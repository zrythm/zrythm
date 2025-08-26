// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_region_to_automation_track_command.h"
#include "dsp/parameter.h"
#include "structure/arrangement/arranger_object.h"
#include "structure/arrangement/automation_region.h"
#include "structure/tracks/automation_track.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{
using namespace zrythm::structure;

class MoveRegionToAutomationTrackCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a test parameter
    auto param_ref = param_registry_.create_object<dsp::ProcessorParameter> (
      port_registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
      dsp::ParameterRange (
        dsp::ParameterRange::Type::Linear, 0.f, 1.f, 0.f, 0.5f),
      utils::Utf8String::from_utf8_encoded_string ("Test Parameter"));

    // Create dependencies for AutomationTrack
    dsp::TempoMap tempo_map{ 44100 };

    // Create source and target automation tracks
    source_at_ = std::make_unique<tracks::AutomationTrack> (
      tempo_map, file_audio_source_registry_, obj_registry_, param_ref);
    target_at_ = std::make_unique<tracks::AutomationTrack> (
      tempo_map, file_audio_source_registry_, obj_registry_, param_ref);

    // Create test automation region
    create_test_region ();
  }

  void TearDown () override
  {
    // Clean up in reverse order of creation
    target_at_.reset ();
    source_at_.reset ();
  }

  void create_test_region ()
  {
    // Create an automation region
    dsp::TempoMap tempo_map{ 44100 };
    automation_region_ref_ =
      obj_registry_.create_object<arrangement::AutomationRegion> (
        tempo_map, obj_registry_, file_audio_source_registry_);

    // Add region to source automation track
    source_at_->add_object (automation_region_ref_);
  }

  arrangement::ArrangerObjectRegistry obj_registry_;
  dsp::FileAudioSourceRegistry        file_audio_source_registry_;
  dsp::PortRegistry                   port_registry_;
  dsp::ProcessorParameterRegistry     param_registry_{ port_registry_ };

  std::unique_ptr<tracks::AutomationTrack> source_at_;
  std::unique_ptr<tracks::AutomationTrack> target_at_;

  arrangement::ArrangerObjectUuidReference automation_region_ref_{
    obj_registry_
  };
};

// Test initial state after construction
TEST_F (MoveRegionToAutomationTrackCommandTest, InitialState)
{
  MoveRegionToAutomationTrackCommand command (
    automation_region_ref_, *source_at_, *target_at_);

  // Verify region is still in source automation track
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (),
    automation_region_ref_.id ());
}

// Test redo operation (move automation region from source to target)
TEST_F (MoveRegionToAutomationTrackCommandTest, RedoOperation)
{
  MoveRegionToAutomationTrackCommand command (
    automation_region_ref_, *source_at_, *target_at_);

  // Initial state
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);

  // Execute redo
  command.redo ();

  // Region should be moved to target automation track
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (
    target_at_->get_children_vector ().at (0).id (),
    automation_region_ref_.id ());
}

// Test undo operation (move automation region back to source)
TEST_F (MoveRegionToAutomationTrackCommandTest, UndoOperation)
{
  MoveRegionToAutomationTrackCommand command (
    automation_region_ref_, *source_at_, *target_at_);

  // First execute redo to move the region
  command.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Then undo
  command.undo ();

  // Region should be back in source automation track
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (),
    automation_region_ref_.id ());
}

// Test undo/redo cycle
TEST_F (MoveRegionToAutomationTrackCommandTest, UndoRedoCycle)
{
  MoveRegionToAutomationTrackCommand command (
    automation_region_ref_, *source_at_, *target_at_);

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
TEST_F (MoveRegionToAutomationTrackCommandTest, MoveFromEmptyTrack)
{
  // Remove the test region first
  source_at_->remove_object (automation_region_ref_.id ());

  // This should throw an exception since the region is not in the source
  // automation track
  EXPECT_THROW (
    MoveRegionToAutomationTrackCommand command (
      automation_region_ref_, *source_at_, *target_at_),
    std::invalid_argument);
}

// Test move to same automation track (should be no-op)
TEST_F (MoveRegionToAutomationTrackCommandTest, MoveToSameTrack)
{
  MoveRegionToAutomationTrackCommand command (
    automation_region_ref_, *source_at_, *source_at_);

  // Initial state
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);

  // Redo should not change anything
  command.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (),
    automation_region_ref_.id ());

  // Undo should also not change anything
  command.undo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (),
    automation_region_ref_.id ());
}

// Test command text
TEST_F (MoveRegionToAutomationTrackCommandTest, CommandText)
{
  MoveRegionToAutomationTrackCommand command (
    automation_region_ref_, *source_at_, *target_at_);

  // The command should have the text "Move Plugin" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Move Plugin"));
}

// Test multiple move operations
TEST_F (MoveRegionToAutomationTrackCommandTest, MultipleMoveOperations)
{
  // First move automation region
  MoveRegionToAutomationTrackCommand command1 (
    automation_region_ref_, *source_at_, *target_at_);
  command1.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Move back to source
  MoveRegionToAutomationTrackCommand command2 (
    automation_region_ref_, *target_at_, *source_at_);
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

} // namespace zrythm::commands
