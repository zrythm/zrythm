// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_clip_to_clip_slot_command.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/scenes/clip_slot.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class AddClipToClipSlotCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    app_settings = std::make_unique<utils::AppSettings> (
      std::make_unique<test_helpers::InMemorySettingsBackend> ());

    factory = std::make_unique<structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map_wrapper,
        .registry_ = object_registry,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    // Create a test AudioClip
    auto clip_builder =
      factory->get_builder<structure::arrangement::AudioClip> ();
    auto clip_ref = clip_builder.build_in_registry ();
    test_clip_ref = clip_ref;

    // Create a clip slot
    clip_slot =
      std::make_unique<structure::scenes::ClipSlot> (object_registry, nullptr);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 object_registry;
  std::unique_ptr<utils::AppSettings>   app_settings;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory;
  structure::arrangement::ArrangerObjectUuidReference            test_clip_ref{
    object_registry
  };
  std::unique_ptr<structure::scenes::ClipSlot> clip_slot;
};

// Test initial state after construction
TEST_F (AddClipToClipSlotCommandTest, InitialState)
{
  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);

  // Initially, the clip slot should not have a clip
  EXPECT_EQ (clip_slot->clip (), nullptr);
}

// Test redo operation sets clip in clip slot
TEST_F (AddClipToClipSlotCommandTest, RedoSetsClip)
{
  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);

  command.redo ();

  // After redo, the clip slot should have the clip
  EXPECT_NE (clip_slot->clip (), nullptr);
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());
}

// Test undo operation clears clip from clip slot
TEST_F (AddClipToClipSlotCommandTest, UndoClearsClip)
{
  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);

  // First set the clip
  command.redo ();
  EXPECT_NE (clip_slot->clip (), nullptr);
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());

  // Then undo should clear it
  command.undo ();
  EXPECT_EQ (clip_slot->clip (), nullptr);
}

// Test multiple undo/redo cycles
TEST_F (AddClipToClipSlotCommandTest, MultipleUndoRedoCycles)
{
  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);

  // First cycle
  command.redo ();
  EXPECT_NE (clip_slot->clip (), nullptr);
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());

  command.undo ();
  EXPECT_EQ (clip_slot->clip (), nullptr);

  // Second cycle
  command.redo ();
  EXPECT_NE (clip_slot->clip (), nullptr);
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());

  command.undo ();
  EXPECT_EQ (clip_slot->clip (), nullptr);
}

// Test command text
TEST_F (AddClipToClipSlotCommandTest, CommandText)
{
  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);

  // The command should have the text "Add Clip"
  EXPECT_EQ (command.text (), QString ("Add Clip"));
}

// Test with different clip types
TEST_F (AddClipToClipSlotCommandTest, DifferentClipTypes)
{
  // Test with MidiClip
  auto midi_builder = factory->get_builder<structure::arrangement::MidiClip> ();
  auto midi_clip_ref = midi_builder.build_in_registry ();
  structure::scenes::ClipSlot midi_clip_slot (object_registry, nullptr);

  AddClipToClipSlotCommand midi_command (midi_clip_slot, midi_clip_ref);

  midi_command.redo ();
  EXPECT_NE (midi_clip_slot.clip (), nullptr);
  EXPECT_EQ (midi_clip_slot.clip (), midi_clip_ref.get ());

  midi_command.undo ();
  EXPECT_EQ (midi_clip_slot.clip (), nullptr);
}

// Test replacing existing clip
TEST_F (AddClipToClipSlotCommandTest, ReplaceExistingClip)
{
  // Set initial clip
  auto initial_builder =
    factory->get_builder<structure::arrangement::AudioClip> ();
  auto initial_clip_ref = initial_builder.build_in_registry ();
  clip_slot->setClip (initial_clip_ref.get ());
  EXPECT_EQ (clip_slot->clip (), initial_clip_ref.get ());

  // Create command with new clip
  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);

  // Redo should replace the existing clip
  command.redo ();
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());
  EXPECT_NE (clip_slot->clip (), initial_clip_ref.get ());

  // Undo should restore the previous clip
  command.undo ();
  EXPECT_EQ (clip_slot->clip (), initial_clip_ref.get ());
}

// Test with clip slot that already has a clip
TEST_F (AddClipToClipSlotCommandTest, ClipSlotWithExistingClip)
{
  // First set a clip
  clip_slot->setClip (
    test_clip_ref.get_object_as<structure::arrangement::AudioClip> ());
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());

  // Create another clip
  auto new_builder = factory->get_builder<structure::arrangement::AudioClip> ();
  auto new_clip_ref = new_builder.build_in_registry ();

  // Create command to add new clip
  AddClipToClipSlotCommand command (*clip_slot, new_clip_ref);

  // Redo should replace the existing clip
  command.redo ();
  EXPECT_EQ (clip_slot->clip (), new_clip_ref.get ());
  EXPECT_NE (clip_slot->clip (), test_clip_ref.get ());

  // Undo should restore the previous clip
  command.undo ();
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());
}

// Test that clearClip signal is emitted on undo
TEST_F (AddClipToClipSlotCommandTest, ClearClipSignalOnUndo)
{
  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);

  // Track signal emissions
  bool clip_changed_emitted = false;
  QObject::connect (
    clip_slot.get (), &structure::scenes::ClipSlot::clipObjectChanged,
    clip_slot.get (), [&] (structure::arrangement::ArrangerObject * clip) {
      clip_changed_emitted = true;
    });

  // Set clip first
  command.redo ();
  clip_changed_emitted = false; // Reset flag

  // Undo should emit signal with nullptr
  command.undo ();
  EXPECT_TRUE (clip_changed_emitted);
  EXPECT_EQ (clip_slot->clip (), nullptr);
}

// Test undo restores the previous clip when the slot already held one.
TEST_F (AddClipToClipSlotCommandTest, UndoRestoresPreviousClip)
{
  auto initial_builder =
    factory->get_builder<structure::arrangement::AudioClip> ();
  auto initial_clip_ref = initial_builder.build_in_registry ();
  clip_slot->setClip (initial_clip_ref.get ());
  ASSERT_EQ (clip_slot->clip (), initial_clip_ref.get ());

  AddClipToClipSlotCommand command (*clip_slot, test_clip_ref);
  command.redo ();
  EXPECT_EQ (clip_slot->clip (), test_clip_ref.get ());

  command.undo ();
  EXPECT_EQ (clip_slot->clip (), initial_clip_ref.get ());
}

} // namespace zrythm::commands
