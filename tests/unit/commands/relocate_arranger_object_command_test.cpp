// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/relocate_arranger_object_command.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/tracks/automation_track.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_lane_list.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{
using namespace zrythm::structure;

class RelocateArrangerObjectCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a test parameter
    auto param_ref = utils::create_object<dsp::ProcessorParameter> (
      registry_, registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String::from_utf8_encoded_string ("Test Parameter"));

    // Create source and target automation tracks
    source_at_ = std::make_unique<tracks::AutomationTrack> (
      tempo_map_wrapper_, registry_, param_ref);
    target_at_ = std::make_unique<tracks::AutomationTrack> (
      tempo_map_wrapper_, registry_, param_ref);

    app_settings_ = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory_ = std::make_unique<arrangement::ArrangerObjectFactory> (
      arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = tempo_map_wrapper_,
        .registry_ = registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    // Create test automation clip
    create_test_clip ();
  }

  void TearDown () override
  {
    // Clean up in reverse order of creation
    target_at_.reset ();
    source_at_.reset ();
  }

  void create_test_clip ()
  {
    // Create an automation clip
    auto builder = factory_->get_builder<arrangement::AutomationClip> ();
    automation_clip_ref_ = builder.build_in_registry ();

    // Add clip to source automation track
    source_at_->add_object (automation_clip_ref_);
  }

  dsp::TempoMap                       tempo_map_{ units::sample_rate (44100) };
  dsp::TempoMapWrapper                tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry               registry_;
  std::unique_ptr<utils::AppSettings> app_settings_;
  std::unique_ptr<arrangement::ArrangerObjectFactory> factory_;

  std::unique_ptr<tracks::AutomationTrack> source_at_;
  std::unique_ptr<tracks::AutomationTrack> target_at_;

  arrangement::ArrangerObjectUuidReference automation_clip_ref_{ registry_ };
};

// Test initial state after construction
TEST_F (RelocateArrangerObjectCommandTest, InitialState)
{
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command (
    automation_clip_ref_, *source_at_, *target_at_);

  // Verify clip is still in source automation track
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (), automation_clip_ref_.id ());
}

// Test redo operation (move automation clip from source to target)
TEST_F (RelocateArrangerObjectCommandTest, RedoOperation)
{
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command (
    automation_clip_ref_, *source_at_, *target_at_);

  // Initial state
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);

  // Execute redo
  command.redo ();

  // Clip should be moved to target automation track
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (
    target_at_->get_children_vector ().at (0).id (), automation_clip_ref_.id ());
}

// Test undo operation (move automation clip back to source)
TEST_F (RelocateArrangerObjectCommandTest, UndoOperation)
{
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command (
    automation_clip_ref_, *source_at_, *target_at_);

  // First execute redo to move the clip
  command.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Then undo
  command.undo ();

  // Clip should be back in source automation track
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (), automation_clip_ref_.id ());
}

// Test undo/redo cycle
TEST_F (RelocateArrangerObjectCommandTest, UndoRedoCycle)
{
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command (
    automation_clip_ref_, *source_at_, *target_at_);

  // Initial state
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);

  // Redo
  command.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Undo
  command.undo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);

  // Redo again
  command.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Undo again
  command.undo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);
}

// Test move from empty source automation track (should fail gracefully)
TEST_F (RelocateArrangerObjectCommandTest, MoveFromEmptyTrack)
{
  // Remove the test clip first
  source_at_->remove_object (automation_clip_ref_.id ());

  // This should throw an exception since the clip is not in the source
  // automation track
  EXPECT_THROW (
    RelocateArrangerObjectCommand<arrangement::AutomationClip> command (
      automation_clip_ref_, *source_at_, *target_at_),
    std::invalid_argument);
}

// Test move to same automation track (should be no-op)
TEST_F (RelocateArrangerObjectCommandTest, MoveToSameTrack)
{
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command (
    automation_clip_ref_, *source_at_, *source_at_);

  // Initial state
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);

  // Redo should not change anything
  command.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (), automation_clip_ref_.id ());

  // Undo should also not change anything
  command.undo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (), automation_clip_ref_.id ());
}

// Test command text
TEST_F (RelocateArrangerObjectCommandTest, CommandText)
{
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command (
    automation_clip_ref_, *source_at_, *target_at_);

  // The command should have the text "Relocate Object" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Relocate Object"));
}

// Test multiple move operations
TEST_F (RelocateArrangerObjectCommandTest, MultipleMoveOperations)
{
  // First move automation clip
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command1 (
    automation_clip_ref_, *source_at_, *target_at_);
  command1.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Move back to source
  RelocateArrangerObjectCommand<arrangement::AutomationClip> command2 (
    automation_clip_ref_, *target_at_, *source_at_);
  command2.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);

  // Undo second move
  command2.undo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Undo first move
  command1.undo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);
}

// Creates a MIDI track with four lanes
class RelocateArrangerObjectLaneTest : public RelocateArrangerObjectCommandTest
{
protected:
  void SetUp () override
  {
    RelocateArrangerObjectCommandTest::SetUp ();

    structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter{ [] {
      return false;
    } };
    structure::tracks::FinalTrackDependencies dependencies{
      tempo_map_wrapper_, registry_, soloed_tracks_exist_getter, {}
    };
    structure::tracks::TrackFactory track_factory{
      [dependencies] () -> structure::tracks::FinalTrackDependencies {
        return dependencies;
      }
    };
    track_ref_ =
      track_factory.create_empty_track<structure::tracks::MidiTrack> ();
    auto * midi_track =
      track_ref_.get_object_as<structure::tracks::MidiTrack> ();

    lane_list_ = midi_track->lanes ();
    lane_list_->create_missing_lanes (3);
    lane_1_ = lane_list_->at (1);
    lane_3_ = lane_list_->at (3);
  }

  size_t clips_in_lane (structure::tracks::TrackLane * lane) const
  {
    return lane
      ->structure::arrangement::ArrangerObjectOwner<
        arrangement::MidiClip>::get_children_vector ()
      .size ();
  }

  structure::tracks::TrackUuidReference track_ref_{ registry_ };
  structure::tracks::TrackLaneList *    lane_list_ = nullptr;
  structure::tracks::TrackLane *        lane_1_ = nullptr;
  structure::tracks::TrackLane *        lane_3_ = nullptr;
};

// Redo reattaches the target lane after undo's trim removed it, so the
// moved object lands in a visible lane
TEST_F (RelocateArrangerObjectLaneTest, RedoReattachesTargetLaneTrimmedByUndo)
{
  auto clip_ref =
    factory_->get_builder<arrangement::MidiClip> ().build_in_registry ();
  lane_1_->structure::arrangement::ArrangerObjectOwner<
    arrangement::MidiClip>::add_object (clip_ref);
  RelocateArrangerObjectCommand<arrangement::MidiClip> command (
    clip_ref, make_owner_ref (*lane_1_, registry_),
    make_owner_ref (*lane_3_, registry_));

  command.redo ();
  command.undo ();
  // Undo's trailing-lane trim removed the emptied target lane
  EXPECT_EQ (lane_3_->owner_list (), nullptr);

  command.redo ();
  EXPECT_EQ (lane_3_->owner_list (), lane_list_);
  const auto lane_3_index = lane_list_->indexOfLane (lane_3_);
  ASSERT_TRUE (lane_3_index.has_value ());
  EXPECT_EQ (*lane_3_index, 3);
  EXPECT_EQ (clips_in_lane (lane_3_), 1);
  EXPECT_EQ (clips_in_lane (lane_1_), 0);
}

// Undo reattaches the source lane after redo's trim removed it, so the
// moved-back object lands in a visible lane
TEST_F (RelocateArrangerObjectLaneTest, UndoReattachesSourceLaneTrimmedByRedo)
{
  auto clip_ref =
    factory_->get_builder<arrangement::MidiClip> ().build_in_registry ();
  lane_3_->structure::arrangement::ArrangerObjectOwner<
    arrangement::MidiClip>::add_object (clip_ref);
  RelocateArrangerObjectCommand<arrangement::MidiClip> command (
    clip_ref, make_owner_ref (*lane_3_, registry_),
    make_owner_ref (*lane_1_, registry_));

  command.redo ();
  // Redo's trailing-lane trim removed the emptied source lane
  EXPECT_EQ (lane_3_->owner_list (), nullptr);

  command.undo ();
  EXPECT_EQ (lane_3_->owner_list (), lane_list_);
  const auto lane_3_index = lane_list_->indexOfLane (lane_3_);
  ASSERT_TRUE (lane_3_index.has_value ());
  EXPECT_EQ (*lane_3_index, 3);
  EXPECT_EQ (clips_in_lane (lane_3_), 1);
  EXPECT_EQ (clips_in_lane (lane_1_), 0);
}

} // namespace zrythm::commands
