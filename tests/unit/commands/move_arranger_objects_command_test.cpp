// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_arranger_objects_command.h"
#include "structure/arrangement/arranger_object_all.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class MoveArrangerObjectsCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Create test objects
    auto marker_ref =
      object_registry.create_object<structure::arrangement::Marker> (
        *tempo_map, structure::arrangement::Marker::MarkerType::Custom);
    auto note_ref = object_registry.create_object<
      structure::arrangement::MidiNote> (*tempo_map);

    test_objects_.push_back (marker_ref);
    test_objects_.push_back (note_ref);

    // Store original positions
    std::visit (
      [&] (auto &&obj) {
        original_positions_.push_back (
          units::ticks (obj->position ()->ticks ()));
      },
      marker_ref.get_object ());
    std::visit (
      [&] (auto &&obj) {
        original_positions_.push_back (
          units::ticks (obj->position ()->ticks ()));
      },
      note_ref.get_object ());
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  structure::arrangement::ArrangerObjectRegistry object_registry;
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
      if (auto * obj = test_objects_[i].get_object_base ())
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
      if (auto * obj = test_objects_[i].get_object_base ())
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
      if (auto * obj = test_objects_[i].get_object_base ())
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
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta).in (units::ticks));
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
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
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta).in (units::ticks));
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
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
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
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
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              original_pitch = obj->pitch ();
            }
        },
        obj_ref.get_object ());
    }

  command.redo ();

  // Check horizontal movement for all objects
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            (original_positions_[i] + tick_delta).in (units::ticks));
        }
    }

  // Check vertical movement for MIDI notes
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch + vertical_delta);
            }
        },
        obj_ref.get_object ());
    }

  command.undo ();

  // Check horizontal movement restored
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks));
        }
    }

  // Check vertical movement restored for MIDI notes
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch);
            }
        },
        obj_ref.get_object ());
    }
}

TEST_F (MoveArrangerObjectsCommandTest, NegativeTickDelta)
{
  // First, move the objects so that the total ticks are still positive
  // (negative timeline ticks are not supported)
  original_positions_.clear ();
  for (const auto &test_object : test_objects_)
    {
      if (auto * obj = test_object.get_object_base ())
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
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (),
            original_positions_[i].in (units::ticks) + tick_delta);
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
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
      if (auto * obj = test_objects_[i].get_object_base ())
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
  auto different_marker =
    object_registry.create_object<structure::arrangement::Marker> (
      *tempo_map, structure::arrangement::Marker::MarkerType::Custom);
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
