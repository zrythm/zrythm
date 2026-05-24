// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_region_to_clip_slot_command.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/scenes/clip_slot.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class AddRegionToClipSlotCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    factory = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map,
        .registry_ = object_registry,
        .musical_mode_getter_ = [] () { return true; },
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); }, [] () { return 120.0; });

    // Create a test AudioRegion
    auto region_builder =
      factory->get_builder<structure::arrangement::AudioRegion> ();
    auto region_ref = region_builder.build_in_registry ();
    test_region_ref = region_ref;

    // Create a clip slot
    clip_slot =
      std::make_unique<structure::scenes::ClipSlot> (object_registry, nullptr);
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  utils::ObjectRegistry          object_registry;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory;
  structure::arrangement::ArrangerObjectUuidReference test_region_ref{
    object_registry
  };
  std::unique_ptr<structure::scenes::ClipSlot> clip_slot;
};

// Test initial state after construction
TEST_F (AddRegionToClipSlotCommandTest, InitialState)
{
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // Initially, the clip slot should not have a region
  EXPECT_EQ (clip_slot->region (), nullptr);
}

// Test redo operation sets region in clip slot
TEST_F (AddRegionToClipSlotCommandTest, RedoSetsRegion)
{
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  command.redo ();

  // After redo, the clip slot should have the region
  EXPECT_NE (clip_slot->region (), nullptr);
  EXPECT_EQ (clip_slot->region (), test_region_ref.get ());
}

// Test undo operation clears region from clip slot
TEST_F (AddRegionToClipSlotCommandTest, UndoClearsRegion)
{
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // First set the region
  command.redo ();
  EXPECT_NE (clip_slot->region (), nullptr);
  EXPECT_EQ (clip_slot->region (), test_region_ref.get ());

  // Then undo should clear it
  command.undo ();
  EXPECT_EQ (clip_slot->region (), nullptr);
}

// Test multiple undo/redo cycles
TEST_F (AddRegionToClipSlotCommandTest, MultipleUndoRedoCycles)
{
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // First cycle
  command.redo ();
  EXPECT_NE (clip_slot->region (), nullptr);
  EXPECT_EQ (clip_slot->region (), test_region_ref.get ());

  command.undo ();
  EXPECT_EQ (clip_slot->region (), nullptr);

  // Second cycle
  command.redo ();
  EXPECT_NE (clip_slot->region (), nullptr);
  EXPECT_EQ (clip_slot->region (), test_region_ref.get ());

  command.undo ();
  EXPECT_EQ (clip_slot->region (), nullptr);
}

// Test command text
TEST_F (AddRegionToClipSlotCommandTest, CommandText)
{
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // The command should have the text "Add Clip"
  EXPECT_EQ (command.text (), QString ("Add Clip"));
}

// Test with different region types
TEST_F (AddRegionToClipSlotCommandTest, DifferentRegionTypes)
{
  // Test with MidiRegion
  auto midi_builder =
    factory->get_builder<structure::arrangement::MidiRegion> ();
  auto midi_region_ref = midi_builder.build_in_registry ();
  structure::scenes::ClipSlot midi_clip_slot (object_registry, nullptr);

  AddRegionToClipSlotCommand midi_command (midi_clip_slot, midi_region_ref);

  midi_command.redo ();
  EXPECT_NE (midi_clip_slot.region (), nullptr);
  EXPECT_EQ (midi_clip_slot.region (), midi_region_ref.get ());

  midi_command.undo ();
  EXPECT_EQ (midi_clip_slot.region (), nullptr);
}

// Test replacing existing region
TEST_F (AddRegionToClipSlotCommandTest, ReplaceExistingRegion)
{
  // Set initial region
  auto initial_builder =
    factory->get_builder<structure::arrangement::AudioRegion> ();
  auto initial_region_ref = initial_builder.build_in_registry ();
  clip_slot->setRegion (initial_region_ref.get ());
  EXPECT_EQ (clip_slot->region (), initial_region_ref.get ());

  // Create command with new region
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // Redo should replace the existing region
  command.redo ();
  EXPECT_EQ (clip_slot->region (), test_region_ref.get ());
  EXPECT_NE (clip_slot->region (), initial_region_ref.get ());

  // Undo should clear the new region (not restore the old one)
  command.undo ();
  EXPECT_EQ (clip_slot->region (), nullptr);
}

// Test with clip slot that already has a region
TEST_F (AddRegionToClipSlotCommandTest, ClipSlotWithExistingRegion)
{
  // First set a region
  clip_slot->setRegion (test_region_ref.get ());
  EXPECT_EQ (clip_slot->region (), test_region_ref.get ());

  // Create another region
  auto new_builder =
    factory->get_builder<structure::arrangement::AudioRegion> ();
  auto new_region_ref = new_builder.build_in_registry ();

  // Create command to add new region
  AddRegionToClipSlotCommand command (*clip_slot, new_region_ref);

  // Redo should replace the existing region
  command.redo ();
  EXPECT_EQ (clip_slot->region (), new_region_ref.get ());
  EXPECT_NE (clip_slot->region (), test_region_ref.get ());

  // Undo should clear the new region
  command.undo ();
  EXPECT_EQ (clip_slot->region (), nullptr);
}

// Test that clearRegion signal is emitted on undo
TEST_F (AddRegionToClipSlotCommandTest, ClearRegionSignalOnUndo)
{
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // Track signal emissions
  bool region_changed_emitted = false;
  QObject::connect (
    clip_slot.get (), &structure::scenes::ClipSlot::regionChanged,
    clip_slot.get (), [&] (structure::arrangement::ArrangerObject * region) {
      region_changed_emitted = true;
    });

  // Set region first
  command.redo ();
  region_changed_emitted = false; // Reset flag

  // Undo should emit signal with nullptr
  command.undo ();
  EXPECT_TRUE (region_changed_emitted);
  EXPECT_EQ (clip_slot->region (), nullptr);
}

} // namespace zrythm::commands
