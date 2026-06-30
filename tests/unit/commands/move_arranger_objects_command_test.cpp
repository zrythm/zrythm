// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_arranger_objects_command.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "utils/app_settings.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/in_memory_settings_backend.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class MoveArrangerObjectsCommandTest : public ::testing::Test
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

    // Create test objects
    auto marker_builder =
      factory->get_builder<structure::arrangement::Marker> ();
    auto marker_ref = marker_builder.build_in_registry ();
    auto note_builder =
      factory->get_builder<structure::arrangement::MidiNote> ();
    auto note_ref = note_builder.build_in_registry ();

    test_objects_.push_back (marker_ref);
    test_objects_.push_back (note_ref);

    // Store original positions
    original_positions_.push_back (
      units::ticks (marker_ref.get ()->position ()->ticks ()));
    original_positions_.push_back (
      units::ticks (note_ref.get ()->position ()->ticks ()));
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  utils::ObjectRegistry                 object_registry;
  std::unique_ptr<utils::AppSettings>   app_settings;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory> factory;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> test_objects_;
  std::vector<units::precise_tick_t> original_positions_;
};

// Test initial state after construction
TEST_F (MoveArrangerObjectsCommandTest, InitialState)
{
  MoveArrangerObjectsCommand command (test_objects_, units::ticks (100.0));

  // Objects should still be at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }
}

// Test redo operation moves objects
TEST_F (MoveArrangerObjectsCommandTest, RedoMovesObjects)
{
  const units::precise_tick_t tick_delta = units::ticks (100.0);
  MoveArrangerObjectsCommand  command (test_objects_, tick_delta);

  command.redo ();

  // Objects should be moved by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta).in (units::ticks));
        }
    }
}

// Test undo operation restores original positions
TEST_F (MoveArrangerObjectsCommandTest, UndoRestoresPositions)
{
  const units::precise_tick_t tick_delta = units::ticks (100.0);
  MoveArrangerObjectsCommand  command (test_objects_, tick_delta);

  // First move objects
  command.redo ();

  // Then undo
  command.undo ();

  // Objects should be back at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }
}

// Test multiple undo/redo cycles
TEST_F (MoveArrangerObjectsCommandTest, MultipleUndoRedoCycles)
{
  const units::precise_tick_t tick_delta = units::ticks (100.0);
  MoveArrangerObjectsCommand  command (test_objects_, tick_delta);

  // First cycle
  command.redo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta).in (units::ticks));
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }

  // Second cycle
  command.redo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta).in (units::ticks));
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }
}

// Test zero tick delta (no-op)
TEST_F (MoveArrangerObjectsCommandTest, ZeroTickDelta)
{
  MoveArrangerObjectsCommand command (test_objects_, units::ticks (0.0));

  command.redo ();

  // Objects should remain at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }
}

// Test vertical movement for MIDI notes
TEST_F (MoveArrangerObjectsCommandTest, VerticalMovementMidiNotes)
{
  const units::precise_tick_t tick_delta = units::ticks (100.0);
  const int                   vertical_delta = 5;

  // Create a command with vertical movement
  MoveArrangerObjectsCommand command (test_objects_, tick_delta, vertical_delta);

  // Store original pitch for MIDI note
  int original_pitch = 0;
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          original_pitch = note->pitch ();
        }
    }

  command.redo ();

  // Check horizontal movement for all objects
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta).in (units::ticks));
        }
    }

  // Check vertical movement for MIDI notes
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          EXPECT_EQ (note->pitch (), original_pitch + vertical_delta);
        }
    }

  command.undo ();

  // Check horizontal movement restored
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }

  // Check vertical movement restored for MIDI notes
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          EXPECT_EQ (note->pitch (), original_pitch);
        }
    }
}

// Test velocity changes for MIDI notes
TEST_F (MoveArrangerObjectsCommandTest, VelocityChangesMidiNotes)
{
  const int velocity_delta = 20;

  // Store original velocity for MIDI note
  int original_velocity = 0;
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          original_velocity = note->velocity ();
        }
    }

  // Create a command with velocity change (no horizontal movement)
  MoveArrangerObjectsCommand command (
    test_objects_, units::ticks (0.0), velocity_delta,
    MoveArrangerObjectsCommand::VerticalChangeType::Velocity);

  command.redo ();

  // Check velocity changed for MIDI notes
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          EXPECT_EQ (note->velocity (), original_velocity + velocity_delta);
        }
    }

  command.undo ();

  // Check velocity restored for MIDI notes
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          EXPECT_EQ (note->velocity (), original_velocity);
        }
    }
}

// Test velocity clamping at boundaries
TEST_F (MoveArrangerObjectsCommandTest, VelocityClampingAtBoundaries)
{
  // First, set velocity to a high value
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          note->setVelocity (120);
        }
    }

  // Try to increase velocity beyond 127
  MoveArrangerObjectsCommand command (
    test_objects_, units::ticks (0.0), 50.0,
    MoveArrangerObjectsCommand::VerticalChangeType::Velocity);

  command.redo ();

  // Velocity should be clamped to 127
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          EXPECT_EQ (note->velocity (), 127);
        }
    }

  command.undo ();

  // Velocity should be restored to 120
  for (const auto &obj_ref : test_objects_)
    {
      if (
        auto * note =
          obj_ref.template get_object_as<structure::arrangement::MidiNote> ())
        {
          EXPECT_EQ (note->velocity (), 120);
        }
    }
}

TEST_F (MoveArrangerObjectsCommandTest, NegativeTickDelta)
{
  // First, move the objects so that the total ticks are still positive
  // (negative timeline ticks are not supported)
  original_positions_.clear ();
  for (const auto &test_object : test_objects_)
    {
      if (auto * obj = test_object.get ())
        {
          obj->position ()->setTicks (100.0);
          original_positions_.push_back (
            units::ticks (obj->position ()->ticks ()));
        }
    }

  const double               tick_delta = -50.0;
  MoveArrangerObjectsCommand command (test_objects_, units::ticks (tick_delta));

  command.redo ();

  // Objects should be moved backward by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks) + tick_delta);
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }
}

// Test command text
TEST_F (MoveArrangerObjectsCommandTest, CommandText)
{
  MoveArrangerObjectsCommand command (test_objects_, units::ticks (100.0));

  // The command should have the text "Move Objects" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Move Objects"));
}

// Test with empty objects vector
TEST_F (MoveArrangerObjectsCommandTest, EmptyObjectsVector)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> empty_objects;
  MoveArrangerObjectsCommand command (empty_objects, units::ticks (100.0));

  // Should not crash with empty objects vector
  command.redo ();
  command.undo ();
}

// Test command merging with same objects
TEST_F (MoveArrangerObjectsCommandTest, CommandMergingSameObjects)
{
  const units::precise_tick_t tick_delta1 = units::ticks (100.0);
  const units::precise_tick_t tick_delta2 = units::ticks (50.0);

  MoveArrangerObjectsCommand command1 (test_objects_, tick_delta1);
  MoveArrangerObjectsCommand command2 (test_objects_, tick_delta2);

  // Execute first command
  command1.redo ();

  // Should be able to merge with second command
  bool can_merge = command1.mergeWith (&command2);
  EXPECT_TRUE (can_merge);

  // After merging, the command1 now has the total delta (delta1 + delta2)
  // Execute it again to apply the total movement
  command1.redo ();

  // Verify objects moved by total delta (original + delta1 + delta2)
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta1 + tick_delta2)
              .in (units::ticks));
        }
    }
}

// Test command merging with different objects (should not merge)
TEST_F (MoveArrangerObjectsCommandTest, CommandMergingDifferentObjects)
{
  const units::precise_tick_t tick_delta = units::ticks (100.0);

  // Create command with different objects
  std::vector<structure::arrangement::ArrangerObjectUuidReference>
       different_objects;
  auto diff_builder = factory->get_builder<structure::arrangement::Marker> ();
  auto different_marker = diff_builder.build_in_registry ();
  different_objects.push_back (different_marker);

  MoveArrangerObjectsCommand command1 (test_objects_, tick_delta);
  MoveArrangerObjectsCommand command2 (different_objects, tick_delta);

  // Execute first command
  command1.redo ();

  // Should not be able to merge with command operating on different objects
  bool can_merge = command1.mergeWith (&command2);
  EXPECT_FALSE (can_merge);
}

// Test command merging with different command ID (should not merge)
TEST_F (MoveArrangerObjectsCommandTest, CommandMergingDifferentCommandType)
{
  const units::precise_tick_t tick_delta = units::ticks (100.0);

  MoveArrangerObjectsCommand command1 (test_objects_, tick_delta);

  // Create a different type of command
  class DifferentCommand : public QUndoCommand
  {
  public:
    int  id () const override { return 99999999; }
    void undo () override { }
    void redo () override { }
  };

  DifferentCommand different_command;

  // Should not be able to merge with different command type
  bool can_merge = command1.mergeWith (&different_command);
  EXPECT_FALSE (can_merge);
}

} // namespace zrythm::commands
