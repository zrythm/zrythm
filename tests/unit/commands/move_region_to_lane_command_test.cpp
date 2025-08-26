// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_region_to_lane_command.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/midi_region.h"
#include "structure/tracks/track_lane.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{
using namespace zrythm::structure;

class MoveRegionToLaneCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create dependencies for TrackLane
    structure::tracks::TrackLane::TrackLaneDependencies lane_deps{
      .obj_registry_ = obj_registry_,
      .file_audio_source_registry_ = file_audio_source_registry_,
      .soloed_lanes_exist_func_ = [] () { return false; }
    };

    // Create source and target lanes
    source_lane_ = std::make_unique<structure::tracks::TrackLane> (lane_deps);
    target_lane_ = std::make_unique<structure::tracks::TrackLane> (lane_deps);

    // Create test regions
    create_test_regions ();
  }

  void TearDown () override
  {
    // Clean up in reverse order of creation
    target_lane_.reset ();
    source_lane_.reset ();
  }

  void create_test_regions ()
  {
    // Create a MIDI region
    dsp::TempoMap tempo_map{ 44100 };
    midi_region_ref_ = obj_registry_.create_object<arrangement::MidiRegion> (
      tempo_map, obj_registry_, file_audio_source_registry_);

    // Create an audio region
    auto musical_mode_getter = [] { return false; };
    audio_region_ref_ = obj_registry_.create_object<arrangement::AudioRegion> (
      tempo_map, obj_registry_, file_audio_source_registry_,
      musical_mode_getter);

    // Add regions to source lane
    source_lane_->arrangement::ArrangerObjectOwner<
      arrangement::MidiRegion>::add_object (midi_region_ref_);
    source_lane_->arrangement::ArrangerObjectOwner<
      arrangement::AudioRegion>::add_object (audio_region_ref_);
  }

  arrangement::ArrangerObjectRegistry obj_registry_;
  dsp::FileAudioSourceRegistry        file_audio_source_registry_;

  std::unique_ptr<structure::tracks::TrackLane> source_lane_;
  std::unique_ptr<structure::tracks::TrackLane> target_lane_;

  arrangement::ArrangerObjectUuidReference midi_region_ref_{ obj_registry_ };
  arrangement::ArrangerObjectUuidReference audio_region_ref_{ obj_registry_ };
};

// Test initial state after construction
TEST_F (MoveRegionToLaneCommandTest, InitialState)
{
  MoveRegionToLaneCommand command (
    midi_region_ref_, *source_lane_, *target_lane_);

  // Verify region is still in source lane
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .at (0)
      .id (),
    midi_region_ref_.id ());
}

// Test redo operation (move MIDI region from source to target)
TEST_F (MoveRegionToLaneCommandTest, RedoOperationMidiRegion)
{
  MoveRegionToLaneCommand command (
    midi_region_ref_, *source_lane_, *target_lane_);

  // Initial state
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);

  // Execute redo
  command.redo ();

  // Region should be moved to target lane
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .at (0)
      .id (),
    midi_region_ref_.id ());
}

// Test redo operation (move audio region from source to target)
TEST_F (MoveRegionToLaneCommandTest, RedoOperationAudioRegion)
{
  MoveRegionToLaneCommand command (
    audio_region_ref_, *source_lane_, *target_lane_);

  // Initial state
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);

  // Execute redo
  command.redo ();

  // Region should be moved to target lane
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .at (0)
      .id (),
    audio_region_ref_.id ());
}

// Test undo operation (move MIDI region back to source)
TEST_F (MoveRegionToLaneCommandTest, UndoOperationMidiRegion)
{
  MoveRegionToLaneCommand command (
    midi_region_ref_, *source_lane_, *target_lane_);

  // First execute redo to move the region
  command.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);

  // Then undo
  command.undo ();

  // Region should be back in source lane
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .at (0)
      .id (),
    midi_region_ref_.id ());
}

// Test undo operation (move audio region back to source)
TEST_F (MoveRegionToLaneCommandTest, UndoOperationAudioRegion)
{
  MoveRegionToLaneCommand command (
    audio_region_ref_, *source_lane_, *target_lane_);

  // First execute redo to move the region
  command.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);

  // Then undo
  command.undo ();

  // Region should be back in source lane
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .at (0)
      .id (),
    audio_region_ref_.id ());
}

// Test undo/redo cycle for MIDI region
TEST_F (MoveRegionToLaneCommandTest, UndoRedoCycleMidiRegion)
{
  MoveRegionToLaneCommand command (
    midi_region_ref_, *source_lane_, *target_lane_);

  // Initial state
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);

  // Redo
  command.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);

  // Undo
  command.undo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);

  // Redo again
  command.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);

  // Undo again
  command.undo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
}

// Test undo/redo cycle for audio region
TEST_F (MoveRegionToLaneCommandTest, UndoRedoCycleAudioRegion)
{
  MoveRegionToLaneCommand command (
    audio_region_ref_, *source_lane_, *target_lane_);

  // Initial state
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);

  // Redo
  command.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);

  // Undo
  command.undo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);

  // Redo again
  command.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);

  // Undo again
  command.undo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::AudioRegion>::get_children_vector ()
      .size (),
    0);
}

// Test move from empty source lane (should fail gracefully)
TEST_F (MoveRegionToLaneCommandTest, MoveFromEmptyLane)
{
  // Remove the test regions first
  source_lane_->arrangement::ArrangerObjectOwner<
    arrangement::MidiRegion>::remove_object (midi_region_ref_.id ());
  source_lane_->arrangement::ArrangerObjectOwner<
    arrangement::AudioRegion>::remove_object (audio_region_ref_.id ());

  // This should throw an exception since the region is not in the source lane
  EXPECT_THROW (
    MoveRegionToLaneCommand command (
      midi_region_ref_, *source_lane_, *target_lane_),
    std::invalid_argument);
}

// Test move to same lane (should be no-op)
TEST_F (MoveRegionToLaneCommandTest, MoveToSameLane)
{
  MoveRegionToLaneCommand command (
    midi_region_ref_, *source_lane_, *source_lane_);

  // Initial state
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);

  // Redo should not change anything
  command.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .at (0)
      .id (),
    midi_region_ref_.id ());

  // Undo should also not change anything
  command.undo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .at (0)
      .id (),
    midi_region_ref_.id ());
}

// Test command text
TEST_F (MoveRegionToLaneCommandTest, CommandText)
{
  MoveRegionToLaneCommand command (
    midi_region_ref_, *source_lane_, *target_lane_);

  // The command should have the text "Move Plugin" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Move Plugin"));
}

// Test multiple move operations
TEST_F (MoveRegionToLaneCommandTest, MultipleMoveOperations)
{
  // First move MIDI region
  MoveRegionToLaneCommand command1 (
    midi_region_ref_, *source_lane_, *target_lane_);
  command1.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);

  // Move back to source
  MoveRegionToLaneCommand command2 (
    midi_region_ref_, *target_lane_, *source_lane_);
  command2.redo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);

  // Undo second move
  command2.undo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);

  // Undo first move
  command1.undo ();
  EXPECT_EQ (
    source_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    1);
  EXPECT_EQ (
    target_lane_
      ->arrangement::ArrangerObjectOwner<
        arrangement::MidiRegion>::get_children_vector ()
      .size (),
    0);
}

} // namespace zrythm::commands
