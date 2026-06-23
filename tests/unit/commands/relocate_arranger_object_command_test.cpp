// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/relocate_arranger_object_command.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/tracks/automation_track.h"
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
        .app_settings_ = *app_settings_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

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
    auto builder = factory_->get_builder<arrangement::AutomationRegion> ();
    automation_region_ref_ = builder.build_in_registry ();

    // Add region to source automation track
    source_at_->add_object (automation_region_ref_);
  }

  dsp::TempoMap                       tempo_map_{ units::sample_rate (44100) };
  dsp::TempoMapWrapper                tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry               registry_;
  std::unique_ptr<utils::AppSettings> app_settings_;
  std::unique_ptr<arrangement::ArrangerObjectFactory> factory_;

  std::unique_ptr<tracks::AutomationTrack> source_at_;
  std::unique_ptr<tracks::AutomationTrack> target_at_;

  arrangement::ArrangerObjectUuidReference automation_region_ref_{ registry_ };
};

// Test initial state after construction
TEST_F (RelocateArrangerObjectCommandTest, InitialState)
{
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command (
    automation_region_ref_, *source_at_, *target_at_);

  // Verify region is still in source automation track
  EXPECT_EQ (source_at_->get_children_vector ().size (), 1);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (
    source_at_->get_children_vector ().at (0).id (),
    automation_region_ref_.id ());
}

// Test redo operation (move automation region from source to target)
TEST_F (RelocateArrangerObjectCommandTest, RedoOperation)
{
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command (
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
TEST_F (RelocateArrangerObjectCommandTest, UndoOperation)
{
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command (
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
TEST_F (RelocateArrangerObjectCommandTest, UndoRedoCycle)
{
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command (
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
TEST_F (RelocateArrangerObjectCommandTest, MoveFromEmptyTrack)
{
  // Remove the test region first
  source_at_->remove_object (automation_region_ref_.id ());

  // This should throw an exception since the region is not in the source
  // automation track
  EXPECT_THROW (
    RelocateArrangerObjectCommand<arrangement::AutomationRegion> command (
      automation_region_ref_, *source_at_, *target_at_),
    std::invalid_argument);
}

// Test move to same automation track (should be no-op)
TEST_F (RelocateArrangerObjectCommandTest, MoveToSameTrack)
{
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command (
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
TEST_F (RelocateArrangerObjectCommandTest, CommandText)
{
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command (
    automation_region_ref_, *source_at_, *target_at_);

  // The command should have the text "Relocate Object" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Relocate Object"));
}

// Test multiple move operations
TEST_F (RelocateArrangerObjectCommandTest, MultipleMoveOperations)
{
  // First move automation region
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command1 (
    automation_region_ref_, *source_at_, *target_at_);
  command1.redo ();
  EXPECT_EQ (source_at_->get_children_vector ().size (), 0);
  EXPECT_EQ (target_at_->get_children_vector ().size (), 1);

  // Move back to source
  RelocateArrangerObjectCommand<arrangement::AutomationRegion> command2 (
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
