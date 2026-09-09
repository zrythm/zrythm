// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/backend/timeline_arranger_objects_model.h"
#include "structure/arrangement/marker.h"
#include "structure/arrangement/tempo_object.h"
#include "structure/arrangement/tempo_object_manager.h"
#include "structure/arrangement/time_signature_object.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/automation_tracklist.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/midi_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/tracklist.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QItemSelectionModel>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::gui
{

namespace arrangement = structure::arrangement;

class TimelineArrangerObjectsModelTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
    tracklist_ = std::make_unique<structure::tracks::Tracklist> (*registry_);
    tempo_object_manager_ =
      std::make_unique<structure::arrangement::TempoObjectManager> (*registry_);
    model_ = std::make_unique<TimelineArrangerObjectsModel> ();
    model_->setTracklist (tracklist_.get ());
    model_->setTempoObjectManager (tempo_object_manager_.get ());
  }

  void TearDown () override
  {
    model_.reset ();
    tempo_object_manager_.reset ();
    tracklist_.reset ();
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

  structure::tracks::TrackUuidReference create_marker_track ()
  {
    structure::tracks::FinalTrackDependencies deps{
      *tempo_map_wrapper_,
      *registry_,
      [] { return false; },
      {},
    };
    return utils::create_object<structure::tracks::MarkerTrack> (
      *registry_, std::move (deps));
  }

  structure::arrangement::ArrangerObjectUuidReference create_midi_clip ()
  {
    return utils::create_object<structure::arrangement::MidiClip> (
      *registry_, *tempo_map_wrapper_, *registry_);
  }

  structure::arrangement::ArrangerObjectUuidReference create_marker ()
  {
    return utils::create_object<structure::arrangement::Marker> (
      *registry_, *tempo_map_wrapper_,
      structure::arrangement::Marker::MarkerType::Custom);
  }

  structure::arrangement::ArrangerObjectUuidReference create_automation_clip ()
  {
    return utils::create_object<structure::arrangement::AutomationClip> (
      *registry_, *tempo_map_wrapper_, *registry_, nullptr);
  }

  structure::tracks::AutomationTrack *
  add_automation_track (structure::tracks::Track * track)
  {
    auto param_id = utils::create_object<dsp::ProcessorParameter> (
      *registry_, *registry_, dsp::ProcessorParameter::UniqueId (u8"param"),
      dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
      u8"Param");
    auto at = utils::make_qobject_unique<structure::tracks::AutomationTrack> (
      *tempo_map_wrapper_, *registry_, std::move (param_id));
    auto * at_ptr = at.get ();
    track->automationTracklist ()->add_automation_track (std::move (at));
    return at_ptr;
  }

  void add_midi_clip_to_first_lane (
    structure::tracks::Track *                                 track,
    const structure::arrangement::ArrangerObjectUuidReference &clip_ref)
  {
    track->lanes ()->getFirstLane ()->arrangement::
      ArrangerObjectOwner<arrangement::MidiClip>::add_object (clip_ref);
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<utils::ObjectRegistry>                registry_;
  std::unique_ptr<dsp::TempoMap>                        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper>                 tempo_map_wrapper_;
  std::unique_ptr<structure::tracks::Tracklist>         tracklist_;
  std::unique_ptr<structure::arrangement::TempoObjectManager>
                                                tempo_object_manager_;
  std::unique_ptr<TimelineArrangerObjectsModel> model_;
};

// A tracklist with a MIDI track (clips in lanes), a marker track and
// automation, plus tempo objects, aggregates all of their objects.
TEST_F (TimelineArrangerObjectsModelTest, InitialWalkCoversAllSourceKinds)
{
  auto   midi_track_ref = create_midi_track ();
  auto * midi_track = midi_track_ref.get ();
  add_midi_clip_to_first_lane (midi_track, create_midi_clip ());
  add_midi_clip_to_first_lane (midi_track, create_midi_clip ());

  auto * at = add_automation_track (midi_track);
  at->arrangement::ArrangerObjectOwner<arrangement::AutomationClip>::add_object (
    create_automation_clip ());

  auto marker_track_ref = create_marker_track ();
  marker_track_ref.get_object_as<structure::tracks::MarkerTrack> ()
    ->arrangement::ArrangerObjectOwner<arrangement::Marker>::add_object (
      create_marker ());
  tracklist_->collection ()->add_track (midi_track_ref);
  tracklist_->collection ()->add_track (marker_track_ref);

  tempo_object_manager_
    ->arrangement::ArrangerObjectOwner<arrangement::TempoObject>::add_object (
      utils::create_object<structure::arrangement::TempoObject> (
        *registry_, *tempo_map_wrapper_));
  tempo_object_manager_->arrangement::
    ArrangerObjectOwner<arrangement::TimeSignatureObject>::add_object (
      utils::create_object<structure::arrangement::TimeSignatureObject> (
        *registry_, *tempo_map_wrapper_));

  // Late attach: the aggregator is pointed at the already-populated
  // tracklist after everything above was created
  auto late_model = std::make_unique<TimelineArrangerObjectsModel> ();
  late_model->setTracklist (tracklist_.get ());
  late_model->setTempoObjectManager (tempo_object_manager_.get ());

  // 2 lane MIDI clips + 1 automation clip + 1 marker + 1 tempo object + 1
  // time signature object
  EXPECT_EQ (late_model->rowCount (), 6);
}

// A track inserted into the collection after the aggregator attached is
// picked up, and its clips are selectable.
TEST_F (TimelineArrangerObjectsModelTest, TrackAddedAfterAttachIsCovered)
{
  auto   track_ref = create_midi_track ();
  auto * track = track_ref.get ();
  tracklist_->collection ()->add_track (track_ref);

  add_midi_clip_to_first_lane (track, create_midi_clip ());
  EXPECT_EQ (model_->rowCount (), 1);
  EXPECT_TRUE (
    model_
      ->mapFromSource (
        track->lanes ()->getFirstLane ()->midiClips ()->index (0, 0))
      .isValid ());

  QItemSelectionModel selection (model_.get ());
  selection.select (model_->index (0, 0), QItemSelectionModel::Select);
  EXPECT_TRUE (selection.hasSelection ());
}

// Removing a track from the collection prunes its rows and the selection
// on them, while selections on other tracks' objects survive.
TEST_F (TimelineArrangerObjectsModelTest, TrackRemovalPrunesRowsAndSelection)
{
  auto track1_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track1_ref);
  add_midi_clip_to_first_lane (track1_ref.get (), create_midi_clip ());

  auto track2_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track2_ref);
  add_midi_clip_to_first_lane (track2_ref.get (), create_midi_clip ());
  ASSERT_EQ (model_->rowCount (), 2);

  QItemSelectionModel selection (model_.get ());
  selection.select (model_->index (1, 0), QItemSelectionModel::Select);
  ASSERT_TRUE (selection.hasSelection ());

  tracklist_->collection ()->remove_track (track1_ref.id ());

  EXPECT_EQ (model_->rowCount (), 1);
  EXPECT_TRUE (selection.hasSelection ());
  EXPECT_TRUE (selection.isSelected (model_->index (0, 0)));

  tracklist_->collection ()->remove_track (track2_ref.id ());
  EXPECT_EQ (model_->rowCount (), 0);
  EXPECT_FALSE (selection.hasSelection ());
}

// Track moves are remove-and-reinsert cycles carried out while the
// collection's moveInProgress flag is set; the unified rows and the
// selection stay stable across them.
TEST_F (TimelineArrangerObjectsModelTest, TrackMoveKeepsRowsAndSelectionStable)
{
  auto track_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track_ref);
  add_midi_clip_to_first_lane (track_ref.get (), create_midi_clip ());
  ASSERT_EQ (model_->rowCount (), 1);

  QItemSelectionModel selection (model_.get ());
  selection.select (model_->index (0, 0), QItemSelectionModel::Select);
  ASSERT_TRUE (selection.hasSelection ());

  auto * collection = tracklist_->collection ();
  collection->setMoveInProgress (true);
  collection->detach_track (track_ref.id ());
  collection->reattach_track (track_ref, 0);
  collection->setMoveInProgress (false);

  EXPECT_EQ (model_->rowCount (), 1);
  EXPECT_TRUE (selection.hasSelection ());
  EXPECT_TRUE (selection.isSelected (model_->index (0, 0)));
}

// Lanes added to a laned track contribute their models; removing a lane
// removes its rows.
TEST_F (TimelineArrangerObjectsModelTest, LanesAddedAndRemovedAreCovered)
{
  auto track_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track_ref);
  auto * track = track_ref.get ();
  ASSERT_GE (track->lanes ()->size (), 1u);

  const auto sources_before = model_->sourceModels ().size ();

  auto * new_lane = track->lanes ()->addLane ();
  EXPECT_EQ (model_->sourceModels ().size (), sources_before + 2);

  const auto clip_ref = create_midi_clip ();
  new_lane->arrangement::ArrangerObjectOwner<arrangement::MidiClip>::add_object (
    clip_ref);
  EXPECT_EQ (model_->rowCount (), 1);

  track->lanes ()->removeLane (track->lanes ()->size () - 1);
  EXPECT_EQ (model_->sourceModels ().size (), sources_before);
  EXPECT_EQ (model_->rowCount (), 0);
}

// Automation tracks contribute their clip models when added and lose them
// when removed; an automation track holder that is removed and re-added
// (the move flow) is registered again.
TEST_F (
  TimelineArrangerObjectsModelTest,
  AutomationTracksAddedAndRemovedAreCovered)
{
  auto track_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track_ref);
  auto * track = track_ref.get ();

  auto *     at = add_automation_track (track);
  const auto sources_with_at = model_->sourceModels ().size ();
  EXPECT_GE (sources_with_at, 1u);

  at->arrangement::ArrangerObjectOwner<arrangement::AutomationClip>::add_object (
    create_automation_clip ());
  EXPECT_EQ (model_->rowCount (), 1);

  auto holder = track->automationTracklist ()->remove_automation_track (*at);
  EXPECT_EQ (model_->sourceModels ().size (), sources_with_at - 1);
  EXPECT_EQ (model_->rowCount (), 0);

  track->automationTracklist ()->add_automation_track (std::move (holder));
  EXPECT_EQ (model_->sourceModels ().size (), sources_with_at);
  EXPECT_EQ (model_->rowCount (), 1);
}

// Automation tracklist reorders reset the model instead of moving rows;
// the source set and the selection stay unchanged across them.
TEST_F (
  TimelineArrangerObjectsModelTest,
  AutomationTracklistReorderKeepsRowsAndSelectionStable)
{
  auto track_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track_ref);
  auto * track = track_ref.get ();
  auto * atl = track->automationTracklist ();

  auto * at1 = add_automation_track (track);
  auto * at2 = add_automation_track (track);
  at1->arrangement::ArrangerObjectOwner<arrangement::AutomationClip>::add_object (
    create_automation_clip ());
  at2->arrangement::ArrangerObjectOwner<arrangement::AutomationClip>::add_object (
    create_automation_clip ());
  ASSERT_EQ (model_->rowCount (), 2);

  QItemSelectionModel selection (model_.get ());
  selection.select (model_->index (0, 0), QItemSelectionModel::Select);
  selection.select (model_->index (1, 0), QItemSelectionModel::Select);
  ASSERT_TRUE (selection.hasSelection ());

  atl->set_automation_track_index (*at1, 1, false);

  EXPECT_EQ (model_->rowCount (), 2);
  EXPECT_TRUE (selection.hasSelection ());
  EXPECT_TRUE (selection.isSelected (model_->index (0, 0)));
  EXPECT_TRUE (selection.isSelected (model_->index (1, 0)));
}

// A source model that is destroyed while registered (destruction without
// a prior structural removal) is dropped from the unified model before
// its destruction begins, so surviving rows stay reachable and the
// selection on them survives.
TEST_F (TimelineArrangerObjectsModelTest, DestroyedSourceIsRemovedEarly)
{
  auto track_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track_ref);
  add_midi_clip_to_first_lane (track_ref.get (), create_midi_clip ());
  ASSERT_EQ (model_->rowCount (), 1);

  tempo_object_manager_
    ->arrangement::ArrangerObjectOwner<arrangement::TempoObject>::add_object (
      utils::create_object<structure::arrangement::TempoObject> (
        *registry_, *tempo_map_wrapper_));
  tempo_object_manager_->arrangement::
    ArrangerObjectOwner<arrangement::TimeSignatureObject>::add_object (
      utils::create_object<structure::arrangement::TimeSignatureObject> (
        *registry_, *tempo_map_wrapper_));
  ASSERT_EQ (model_->rowCount (), 3);

  QItemSelectionModel selection (model_.get ());
  const auto          selected_row = model_->mapFromSource (
    track_ref.get ()->lanes ()->getFirstLane ()->midiClips ()->index (0, 0));
  ASSERT_TRUE (selected_row.isValid ());
  selection.select (selected_row, QItemSelectionModel::Select);
  ASSERT_TRUE (selection.hasSelection ());

  tempo_object_manager_.reset ();

  EXPECT_EQ (model_->rowCount (), 1);
  EXPECT_TRUE (selection.hasSelection ());
  EXPECT_TRUE (selection.isSelected (model_->mapFromSource (
    track_ref.get ()->lanes ()->getFirstLane ()->midiClips ()->index (0, 0))));
}

// Detaching from the tracklist removes all of the tracklist's sources
// while the tempo sources stay.
TEST_F (TimelineArrangerObjectsModelTest, DetachRemovesTracklistSources)
{
  auto track_ref = create_midi_track ();
  tracklist_->collection ()->add_track (track_ref);
  add_midi_clip_to_first_lane (track_ref.get (), create_midi_clip ());

  tempo_object_manager_
    ->arrangement::ArrangerObjectOwner<arrangement::TempoObject>::add_object (
      utils::create_object<structure::arrangement::TempoObject> (
        *registry_, *tempo_map_wrapper_));
  ASSERT_EQ (model_->rowCount (), 2);

  model_->setTracklist (nullptr);
  EXPECT_EQ (model_->rowCount (), 1);

  model_->setTempoObjectManager (nullptr);
  EXPECT_EQ (model_->rowCount (), 0);
}

} // namespace zrythm::gui
