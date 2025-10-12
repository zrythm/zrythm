// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_region_to_clip_slot_command.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/scenes/clip_slot.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class AddRegionToClipSlotCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    musical_mode_getter = [&] () { return global_musical_mode_enabled; };

    // Create a test AudioRegion
    auto region_ref = object_registry.create_object<
      structure::arrangement::AudioRegion> (
      *tempo_map, object_registry, file_audio_source_registry,
      musical_mode_getter);
    test_region_ref = region_ref;

    // Create a clip slot
    clip_slot =
      std::make_unique<structure::scenes::ClipSlot> (object_registry, nullptr);
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  structure::arrangement::ArrangerObjectRegistry object_registry;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;
  structure::arrangement::ArrangerObjectUuidReference test_region_ref{
    object_registry
  };
  std::unique_ptr<structure::scenes::ClipSlot> clip_slot;
  bool global_musical_mode_enabled{ true };
  structure::arrangement::AudioRegion::GlobalMusicalModeGetter
    musical_mode_getter;
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
  EXPECT_EQ (clip_slot->region (), test_region_ref.get_object_base ());
}

// Test undo operation clears region from clip slot
TEST_F (AddRegionToClipSlotCommandTest, UndoClearsRegion)
{
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // First set the region
  command.redo ();
  EXPECT_NE (clip_slot->region (), nullptr);
  EXPECT_EQ (clip_slot->region (), test_region_ref.get_object_base ());

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
  EXPECT_EQ (clip_slot->region (), test_region_ref.get_object_base ());

  command.undo ();
  EXPECT_EQ (clip_slot->region (), nullptr);

  // Second cycle
  command.redo ();
  EXPECT_NE (clip_slot->region (), nullptr);
  EXPECT_EQ (clip_slot->region (), test_region_ref.get_object_base ());

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
  auto midi_region_ref =
    object_registry.create_object<structure::arrangement::MidiRegion> (
      *tempo_map, object_registry, file_audio_source_registry);
  structure::scenes::ClipSlot midi_clip_slot (object_registry, nullptr);

  AddRegionToClipSlotCommand midi_command (midi_clip_slot, midi_region_ref);

  midi_command.redo ();
  EXPECT_NE (midi_clip_slot.region (), nullptr);
  EXPECT_EQ (midi_clip_slot.region (), midi_region_ref.get_object_base ());

  midi_command.undo ();
  EXPECT_EQ (midi_clip_slot.region (), nullptr);
}

// Test replacing existing region
TEST_F (AddRegionToClipSlotCommandTest, ReplaceExistingRegion)
{
  // Set initial region
  auto initial_region_ref = object_registry.create_object<
    structure::arrangement::AudioRegion> (
    *tempo_map, object_registry, file_audio_source_registry,
    musical_mode_getter);
  clip_slot->setRegion (initial_region_ref.get_object_base ());
  EXPECT_EQ (clip_slot->region (), initial_region_ref.get_object_base ());

  // Create command with new region
  AddRegionToClipSlotCommand command (*clip_slot, test_region_ref);

  // Redo should replace the existing region
  command.redo ();
  EXPECT_EQ (clip_slot->region (), test_region_ref.get_object_base ());
  EXPECT_NE (clip_slot->region (), initial_region_ref.get_object_base ());

  // Undo should clear the new region (not restore the old one)
  command.undo ();
  EXPECT_EQ (clip_slot->region (), nullptr);
}

// Test with clip slot that already has a region
TEST_F (AddRegionToClipSlotCommandTest, ClipSlotWithExistingRegion)
{
  // First set a region
  clip_slot->setRegion (test_region_ref.get_object_base ());
  EXPECT_EQ (clip_slot->region (), test_region_ref.get_object_base ());

  // Create another region
  auto new_region_ref = object_registry.create_object<
    structure::arrangement::AudioRegion> (
    *tempo_map, object_registry, file_audio_source_registry,
    musical_mode_getter);

  // Create command to add new region
  AddRegionToClipSlotCommand command (*clip_slot, new_region_ref);

  // Redo should replace the existing region
  command.redo ();
  EXPECT_EQ (clip_slot->region (), new_region_ref.get_object_base ());
  EXPECT_NE (clip_slot->region (), test_region_ref.get_object_base ());

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
