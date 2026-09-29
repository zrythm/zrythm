// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/editor_arranger_objects_model.h"
#include "structure/arrangement/automation_clip.h"
#include "structure/arrangement/midi_clip.h"
#include "structure/arrangement/midi_note.h"
#include "structure/project/clip_editor.h"
#include "structure/tracks/midi_track.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QItemSelectionModel>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::gui
{

namespace arrangement = structure::arrangement;

class EditorArrangerObjectsModelTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    clip_editor_ = std::make_unique<structure::project::ClipEditor> (*registry_);
    model_ = std::make_unique<EditorArrangerObjectsModel> ();
    model_->setClipEditor (clip_editor_.get ());
  }

  void TearDown () override
  {
    model_.reset ();
    clip_editor_.reset ();
    registry_.reset ();
    app_.reset ();
  }

  structure::tracks::TrackUuidReference create_midi_track ()
  {
    structure::tracks::FinalTrackDependencies deps{
      *tempo_map_wrapper_,
      *registry_,
      [] { return false; },
      {},
    };
    return utils::create_object<structure::tracks::MidiTrack> (
      *registry_, std::move (deps));
  }

  arrangement::ArrangerObjectUuidReference
  create_midi_clip_with_notes (int note_count)
  {
    auto clip_ref = utils::create_object<arrangement::MidiClip> (
      *registry_, *tempo_map_wrapper_, *registry_);
    auto * clip = clip_ref.get_object_as<arrangement::MidiClip> ();
    for (int i = 0; i < note_count; ++i)
      {
        clip->arrangement::ArrangerObjectOwner<arrangement::MidiNote>::add_object (
          utils::create_object<arrangement::MidiNote> (
            *registry_, *tempo_map_wrapper_, clip));
      }
    return clip_ref;
  }

  arrangement::ArrangerObjectUuidReference
  create_automation_clip_with_points (int point_count)
  {
    auto clip_ref = utils::create_object<arrangement::AutomationClip> (
      *registry_, *tempo_map_wrapper_, *registry_, nullptr);
    auto * clip = clip_ref.get_object_as<arrangement::AutomationClip> ();
    for (int i = 0; i < point_count; ++i)
      {
        clip->arrangement::ArrangerObjectOwner<arrangement::AutomationPoint>::
          add_object (
            utils::create_object<arrangement::AutomationPoint> (
              *registry_, *tempo_map_wrapper_));
      }
    return clip_ref;
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<utils::ObjectRegistry>                registry_;
  std::unique_ptr<dsp::TempoMap>                        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper_;
  std::unique_ptr<structure::project::ClipEditor>       clip_editor_;
  std::unique_ptr<EditorArrangerObjectsModel>           model_;
};

// Opening a clip registers the model of the clip's arranger objects.
TEST_F (EditorArrangerObjectsModelTest, OpenedClipRegistersItsModel)
{
  auto track_ref = create_midi_track ();
  auto clip_ref = create_midi_clip_with_notes (3);

  clip_editor_->set_clip (clip_ref.id (), track_ref.id ());

  EXPECT_EQ (model_->rowCount (), 3);
  EXPECT_TRUE (
    model_
      ->mapFromSource (
        clip_ref.get_object_as<arrangement::MidiClip> ()->midiNotes ()->index (
          1, 0))
      .isValid ());
}

// Switching to another clip swaps the source; the rows of the previous
// clip are replaced by the new clip's rows.
TEST_F (EditorArrangerObjectsModelTest, ClipSwitchSwapsSources)
{
  auto track_ref = create_midi_track ();
  auto midi_clip_ref = create_midi_clip_with_notes (2);
  clip_editor_->set_clip (midi_clip_ref.id (), track_ref.id ());
  ASSERT_EQ (model_->rowCount (), 2);

  auto automation_clip_ref = create_automation_clip_with_points (5);
  clip_editor_->set_clip (automation_clip_ref.id (), track_ref.id ());

  EXPECT_EQ (model_->rowCount (), 5);
  EXPECT_TRUE (
    model_
      ->mapFromSource (
        automation_clip_ref.get_object_as<arrangement::AutomationClip> ()
          ->automationPoints ()
          ->index (4, 0))
      .isValid ());
}

// Closing the clip clears the unified model.
TEST_F (EditorArrangerObjectsModelTest, ClosedClipClearsSources)
{
  auto track_ref = create_midi_track ();
  auto clip_ref = create_midi_clip_with_notes (2);
  clip_editor_->set_clip (clip_ref.id (), track_ref.id ());
  ASSERT_EQ (model_->rowCount (), 2);

  clip_editor_->unsetClip ();

  EXPECT_EQ (model_->rowCount (), 0);
}

// A clip destroyed while open (its last reference dropped) is removed
// from the unified model before its destruction begins.
TEST_F (EditorArrangerObjectsModelTest, ClipDeathWhileOpenRemovesSource)
{
  auto track_ref = create_midi_track ();
  {
    auto clip_ref = create_midi_clip_with_notes (2);
    clip_editor_->set_clip (clip_ref.id (), track_ref.id ());
    ASSERT_EQ (model_->rowCount (), 2);

    QItemSelectionModel selection (model_.get ());
    selection.select (model_->index (0, 0), QItemSelectionModel::Select);
    ASSERT_TRUE (selection.hasSelection ());
  }

  EXPECT_EQ (model_->rowCount (), 0);
}

// A clip editor that already has a clip open is synced when the model is
// attached to it.
TEST_F (EditorArrangerObjectsModelTest, LateAttachSyncsOpenClip)
{
  auto track_ref = create_midi_track ();
  auto clip_ref = create_midi_clip_with_notes (4);
  clip_editor_->set_clip (clip_ref.id (), track_ref.id ());

  auto late_model = std::make_unique<EditorArrangerObjectsModel> ();
  late_model->setClipEditor (clip_editor_.get ());

  EXPECT_EQ (late_model->rowCount (), 4);
}

} // namespace zrythm::gui
