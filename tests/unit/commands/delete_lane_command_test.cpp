// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_arranger_object_command.h"
#include "commands/delete_lane_command.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/tracks/track_lane_list.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class DeleteLaneCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory_ = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = tempo_map_wrapper_,
        .registry_ = registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });
  }

  dsp::TempoMap                       tempo_map_{ units::sample_rate (44100) };
  dsp::TempoMapWrapper                tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry               registry_;
  std::unique_ptr<utils::AppSettings> app_settings_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory_;
};

// A track always keeps at least one lane
TEST_F (DeleteLaneCommandTest, RefusesLastLane)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane = lane_list.addLane ();

  EXPECT_THROW (
    DeleteLaneCommand (
      lane_list,
      structure::tracks::TrackLaneUuidReference (lane->get_uuid (), registry_)),
    std::invalid_argument);
  EXPECT_EQ (lane_list.size (), 1);
}

// The command operates on the list the lane belongs to
TEST_F (DeleteLaneCommandTest, RefusesLaneFromAnotherList)
{
  structure::tracks::TrackLaneList list_a{ registry_, nullptr };
  structure::tracks::TrackLaneList list_b{ registry_, nullptr };
  auto *                           lane_a = list_a.addLane ();
  list_b.addLane ();
  list_b.addLane ();

  EXPECT_THROW (
    DeleteLaneCommand (
      list_b,
      structure::tracks::TrackLaneUuidReference (lane_a->get_uuid (), registry_)),
    std::invalid_argument);
  EXPECT_EQ (list_b.size (), 2);
}

// Undo reattaches the lane at its original position with its contents
TEST_F (DeleteLaneCommandTest, UndoRestoresLaneWithContents)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane_1 = lane_list.addLane ();
  auto *                           lane_2 = lane_list.addLane ();
  auto                             clip_ref =
    factory_->get_builder<structure::arrangement::MidiClip> ()
      .build_in_registry ();
  AddArrangerObjectCommand<structure::arrangement::MidiClip> attach_clip (
    make_owner_ref (*lane_2, registry_), clip_ref);
  attach_clip.redo ();
  // The clip attach appended a trailing empty lane
  ASSERT_EQ (lane_list.size (), 3);
  auto note_ref =
    factory_->get_builder<structure::arrangement::MidiNote> ()
      .build_in_registry ();
  AddArrangerObjectCommand<structure::arrangement::MidiNote> attach_note (
    make_owner_ref (*clip_ref.get (), registry_), note_ref);
  attach_note.redo ();

  DeleteLaneCommand command (
    lane_list,
    structure::tracks::TrackLaneUuidReference (lane_2->get_uuid (), registry_));
  command.redo ();
  EXPECT_EQ (lane_list.size (), 2);
  EXPECT_EQ (lane_list.at (0), lane_1);

  command.undo ();
  EXPECT_EQ (lane_list.size (), 3);
  EXPECT_EQ (lane_list.at (0), lane_1);
  EXPECT_EQ (lane_list.at (1), lane_2);
  // The lane's clip stayed owned by the lane while it was detached
  EXPECT_EQ (
    lane_2
      ->structure::arrangement::ArrangerObjectOwner<
        structure::arrangement::MidiClip>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (lane_2->owner_list (), &lane_list);

  // A redo cycle removes the lane again at its re-looked-up index
  command.redo ();
  EXPECT_EQ (lane_list.size (), 2);
  EXPECT_EQ (lane_list.at (0), lane_1);
}

// A list always ends with an empty lane: removing the last lane is
// refused when the remaining last lane would have content
TEST_F (DeleteLaneCommandTest, RefusesRemovingLastLaneWithoutEmptyPredecessor)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane_1 = lane_list.addLane ();
  auto *                           lane_2 = lane_list.addLane ();
  auto                             clip_ref =
    factory_->get_builder<structure::arrangement::MidiClip> ()
      .build_in_registry ();
  AddArrangerObjectCommand<structure::arrangement::MidiClip> attach_clip (
    make_owner_ref (*lane_1, registry_), clip_ref);
  attach_clip.redo ();
  // Lane 1 is not the last lane, so no spare was appended
  ASSERT_EQ (lane_list.size (), 2);

  EXPECT_THROW (
    DeleteLaneCommand (
      lane_list,
      structure::tracks::TrackLaneUuidReference (lane_2->get_uuid (), registry_)),
    std::invalid_argument);
  EXPECT_EQ (lane_list.size (), 2);
}

// Removing the last lane is allowed when the remaining last lane is
// empty
TEST_F (DeleteLaneCommandTest, AllowsRemovingLastEmptyLane)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  lane_list.addLane ();
  auto * lane_2 = lane_list.addLane ();

  DeleteLaneCommand command (
    lane_list,
    structure::tracks::TrackLaneUuidReference (lane_2->get_uuid (), registry_));
  command.redo ();
  EXPECT_EQ (lane_list.size (), 1);

  command.undo ();
  EXPECT_EQ (lane_list.size (), 2);
}

// reinsert_lane validates its inputs
TEST_F (DeleteLaneCommandTest, ReinsertLaneValidates)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  auto *                           lane = lane_list.addLane ();
  auto detached_ref = utils::create_object<structure::tracks::TrackLane> (
    registry_,
    structure::tracks::TrackLane::TrackLaneDependencies{
      .registry_ = registry_, .timebase_provider_ = nullptr });

  EXPECT_THROW (lane_list.reinsert_lane (2, detached_ref), std::out_of_range);
  EXPECT_THROW (
    lane_list.reinsert_lane (
      0,
      structure::tracks::TrackLaneUuidReference (lane->get_uuid (), registry_)),
    std::invalid_argument);

  lane_list.reinsert_lane (1, detached_ref);
  EXPECT_EQ (lane_list.size (), 2);
  EXPECT_EQ (lane_list.at (1)->get_uuid (), detached_ref.id ());
}

// A list refuses to remove its last lane
TEST_F (DeleteLaneCommandTest, RemoveLaneKeepsLastLane)
{
  structure::tracks::TrackLaneList lane_list{ registry_, nullptr };
  lane_list.addLane ();

  lane_list.removeLane (0);

  EXPECT_EQ (lane_list.size (), 1);
}

} // namespace zrythm::commands
