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
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0 * mp_units::si::hertz);

    // Create test objects
    auto marker_ref =
      object_registry.create_object<structure::arrangement::Marker> (
        *tempo_map, structure::arrangement::Marker::MarkerType::Custom);
    auto note_ref = object_registry.create_object<
      structure::arrangement::MidiNote> (*tempo_map);

    test_objects_.push_back (marker_ref);
    test_objects_.push_back (note_ref);

    // Store original positions
    original_positions_.push_back (
      marker_ref.get_object_base ()->position ()->ticks ());
    original_positions_.push_back (
      note_ref.get_object_base ()->position ()->ticks ());
  }

  std::unique_ptr<dsp::TempoMap>                 tempo_map;
  structure::arrangement::ArrangerObjectRegistry object_registry;
  std::vector<structure::arrangement::ArrangerObjectUuidReference> test_objects_;
  std::vector<double> original_positions_;
};

// Test initial state after construction
TEST_F (MoveArrangerObjectsCommandTest, InitialState)
{
  MoveArrangerObjectsCommand command (test_objects_, 100.0);

  // Objects should still be at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }
}

// Test redo operation moves objects
TEST_F (MoveArrangerObjectsCommandTest, RedoMovesObjects)
{
  const double               tick_delta = 100.0;
  MoveArrangerObjectsCommand command (test_objects_, tick_delta);

  command.redo ();

  // Objects should be moved by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }
}

// Test undo operation restores original positions
TEST_F (MoveArrangerObjectsCommandTest, UndoRestoresPositions)
{
  const double               tick_delta = 100.0;
  MoveArrangerObjectsCommand command (test_objects_, tick_delta);

  // First move objects
  command.redo ();

  // Then undo
  command.undo ();

  // Objects should be back at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }
}

// Test multiple undo/redo cycles
TEST_F (MoveArrangerObjectsCommandTest, MultipleUndoRedoCycles)
{
  const double               tick_delta = 100.0;
  MoveArrangerObjectsCommand command (test_objects_, tick_delta);

  // First cycle
  command.redo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }

  // Second cycle
  command.redo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }
}

// Test zero tick delta (no-op)
TEST_F (MoveArrangerObjectsCommandTest, ZeroTickDelta)
{
  MoveArrangerObjectsCommand command (test_objects_, 0.0);

  command.redo ();

  // Objects should remain at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }
}

// TODO: Test negative tick delta
#if 0
TEST_F (MoveArrangerObjectsCommandTest, NegativeTickDelta)
{
  const double               tick_delta = -50.0;
  MoveArrangerObjectsCommand command (test_objects_, tick_delta);

  command.redo ();

  // Objects should be moved backward by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  command.undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }
}
#endif

// Test command text
TEST_F (MoveArrangerObjectsCommandTest, CommandText)
{
  MoveArrangerObjectsCommand command (test_objects_, 100.0);

  // The command should have the text "Move Objects" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Move Objects"));
}

// Test with empty objects vector
TEST_F (MoveArrangerObjectsCommandTest, EmptyObjectsVector)
{
  std::vector<structure::arrangement::ArrangerObjectUuidReference> empty_objects;
  MoveArrangerObjectsCommand command (empty_objects, 100.0);

  // Should not crash with empty objects vector
  command.redo ();
  command.undo ();
}

// Test command merging with same objects
TEST_F (MoveArrangerObjectsCommandTest, CommandMergingSameObjects)
{
  const double tick_delta1 = 100.0;
  const double tick_delta2 = 50.0;

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
            original_positions_[i] + tick_delta1 + tick_delta2);
        }
    }
}

// Test command merging with different objects (should not merge)
TEST_F (MoveArrangerObjectsCommandTest, CommandMergingDifferentObjects)
{
  const double tick_delta = 100.0;

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
  const double tick_delta = 100.0;

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
