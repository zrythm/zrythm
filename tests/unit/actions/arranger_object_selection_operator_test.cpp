// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_selection_operator.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/arranger_object_list_model.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class ArrangerObjectSelectionOperatorTest : public ::testing::Test
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

    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (marker_ref);
    test_objects_.get<structure::arrangement::random_access_index> ()
      .push_back (note_ref);

    // Store original positions
    original_positions_.push_back (
      marker_ref.get_object_base ()->position ()->ticks ());
    original_positions_.push_back (
      note_ref.get_object_base ()->position ()->ticks ());

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create operator
    operator_ = std::make_unique<ArrangerObjectSelectionOperator> ();
    operator_->setUndoStack (undo_stack_.get ());

    // Create selection model and set up with test objects
    selection_model_ = std::make_unique<QItemSelectionModel> (&list_model_);
    operator_->setSelectionModel (selection_model_.get ());

    // Select all objects
    selection_model_->select (
      list_model_.index (0, 0), QItemSelectionModel::Select);
    selection_model_->select (
      list_model_.index (1, 0), QItemSelectionModel::Select);
  }

  std::unique_ptr<dsp::TempoMap>                               tempo_map;
  structure::arrangement::ArrangerObjectRegistry               object_registry;
  structure::arrangement::ArrangerObjectRefMultiIndexContainer test_objects_;
  std::vector<double>                              original_positions_;
  std::unique_ptr<undo::UndoStack>                 undo_stack_;
  std::unique_ptr<ArrangerObjectSelectionOperator> operator_;
  structure::arrangement::ArrangerObjectListModel  list_model_{ test_objects_ };
  std::unique_ptr<QItemSelectionModel>             selection_model_;
};

// Test initial state after construction
TEST_F (ArrangerObjectSelectionOperatorTest, InitialState)
{
  EXPECT_EQ (operator_->undoStack (), undo_stack_.get ());
  EXPECT_EQ (operator_->selectionModel (), selection_model_.get ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveByTicks with positive delta
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksPositiveDelta)
{
  const double tick_delta = 100.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Objects should be moved by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->count (), 1);
}

// Test moveByTicks with negative delta
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksNegativeDelta)
{
  // Move objects to position 100 first to allow negative movement
  for (const auto &obj_ref : test_objects_)
    {
      if (auto * obj = obj_ref.get_object_base ())
        {
          obj->position ()->setTicks (100.0);
        }
    }

  const double tick_delta = -50.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Objects should be moved backward by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), 50);
        }
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->count (), 1);
}

// Test moveByTicks with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksZeroDelta)
{
  bool result = operator_->moveByTicks (0.0);
  EXPECT_TRUE (result);

  // Objects should remain at original positions
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveByTicks with no selection (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  bool result = operator_->moveByTicks (100.0);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveByTicks with invalid movement (before timeline start)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksInvalidMovement)
{
  // Move objects to position 0 first
  for (auto &obj_ref : test_objects_)
    {
      if (auto * obj = obj_ref.get_object_base ())
        {
          obj->position ()->setTicks (0.0);
        }
    }

  const double tick_delta = -50.0; // Would put objects at -50 ticks

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_FALSE (result);

  // Objects should remain at position 0 (no movement)
  for (auto &obj_ref : test_objects_)
    {
      if (auto * obj = obj_ref.get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), 0.0);
        }
    }

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test undo/redo functionality
TEST_F (ArrangerObjectSelectionOperatorTest, UndoRedoFunctionality)
{
  const double tick_delta = 100.0;

  // Move objects
  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Verify objects moved
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  // Undo
  undo_stack_->undo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (obj->position ()->ticks (), original_positions_[i]);
        }
    }

  // Redo
  undo_stack_->redo ();
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (
        auto * obj =
          test_objects_.get<structure::arrangement::random_access_index> ()[i]
            .get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }
}

// Test moveNotesByPitch functionality
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitch)
{
  const int pitch_delta = 5;

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

  bool result = operator_->moveNotesByPitch (pitch_delta);
  EXPECT_TRUE (result);

  // MIDI notes should be moved by pitch_delta
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch + pitch_delta);
            }
        },
        obj_ref.get_object ());
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->count (), 1);

  // Test undo/redo
  undo_stack_->undo ();
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

  undo_stack_->redo ();
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote *>)
            {
              EXPECT_EQ (obj->pitch (), original_pitch + pitch_delta);
            }
        },
        obj_ref.get_object ());
    }
}

// Test moveAutomationPointsByDelta functionality
TEST_F (ArrangerObjectSelectionOperatorTest, MoveAutomationPointsByDelta)
{
  // Create an automation point for testing
  auto automation_point_ref = object_registry.create_object<
    structure::arrangement::AutomationPoint> (*tempo_map);

  // Set initial value
  automation_point_ref
    .get_object_as<structure::arrangement::AutomationPoint> ()
    ->setValue (0.5f);

  // Add to test objects and select it
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    automation_point_ref);

  // Update list model and select the automation point
  selection_model_->select (
    list_model_.index (2, 0), QItemSelectionModel::Select);

  const double delta = 0.2;

  bool result = operator_->moveAutomationPointsByDelta (delta);
  EXPECT_TRUE (result);

  // Automation point should be moved by delta
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint *>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.7f); // 0.5 + 0.2
            }
        },
        obj_ref.get_object ());
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->count (), 1);

  // Test undo/redo
  undo_stack_->undo ();
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint *>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.5f);
            }
        },
        obj_ref.get_object ());
    }

  undo_stack_->redo ();
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint *>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.7f);
            }
        },
        obj_ref.get_object ());
    }
}

// Test moveNotesByPitch with no selection (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  bool result = operator_->moveNotesByPitch (5);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveNotesByPitch with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchZeroDelta)
{
  bool result = operator_->moveNotesByPitch (0);
  EXPECT_TRUE (result);

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveNotesByPitch with invalid pitch (out of range)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchInvalidPitch)
{
  // Find the MIDI note and set its pitch to 125 (close to max)
  int original_pitch = 0;
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::MidiNote>)
            {
              obj->setPitch (125);
              original_pitch = 125;
            }
        },
        obj_ref.get_object ());
    }

  // Try to move by 5 (would result in pitch 130, which is out of range)
  bool result = operator_->moveNotesByPitch (5);
  EXPECT_FALSE (result);

  // Pitch should remain unchanged
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

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveAutomationPointsByDelta with no selection (no-op)
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveAutomationPointsByDeltaNoSelection)
{
  // Clear selection
  selection_model_->clear ();

  bool result = operator_->moveAutomationPointsByDelta (0.1);
  EXPECT_FALSE (result);

  // No command should be pushed for no selection
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveAutomationPointsByDelta with zero delta (no-op)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveAutomationPointsByDeltaZeroDelta)
{
  bool result = operator_->moveAutomationPointsByDelta (0.0);
  EXPECT_TRUE (result);

  // No command should be pushed for zero delta
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveAutomationPointsByDelta with invalid value (out of range)
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveAutomationPointsByDeltaInvalidValue)
{
  // Create an automation point with value 0.9
  auto automation_point_ref = object_registry.create_object<
    structure::arrangement::AutomationPoint> (*tempo_map);

  automation_point_ref
    .get_object_as<structure::arrangement::AutomationPoint> ()
    ->setValue (0.9f);

  // Add to test objects and select it
  test_objects_.get<structure::arrangement::random_access_index> ().push_back (
    automation_point_ref);

  // Update list model and select the automation point
  selection_model_->select (
    list_model_.index (2, 0), QItemSelectionModel::Select);

  // Try to move by 0.2 (would result in value 1.1, which is out of range)
  bool result = operator_->moveAutomationPointsByDelta (0.2);
  EXPECT_FALSE (result);

  // Value should remain unchanged
  for (const auto &obj_ref : test_objects_)
    {
      std::visit (
        [&] (auto &&obj) {
          using ObjectT = base_type<decltype (obj)>;
          if constexpr (
            std::is_same_v<ObjectT, structure::arrangement::AutomationPoint>)
            {
              EXPECT_FLOAT_EQ (obj->value (), 0.9f);
            }
        },
        obj_ref.get_object ());
    }

  // No command should be pushed for invalid movement
  EXPECT_EQ (undo_stack_->count (), 0);
}

// Test moveByTicks without undo stack set (should return false)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksWithoutUndoStack)
{
  auto operator_no_stack = std::make_unique<ArrangerObjectSelectionOperator> ();
  operator_no_stack->setSelectionModel (selection_model_.get ());

  bool result = operator_no_stack->moveByTicks (100.0);
  EXPECT_FALSE (result);
}

// Test moveByTicks without selection model set (should return false)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksWithoutSelectionModel)
{
  auto operator_no_selection =
    std::make_unique<ArrangerObjectSelectionOperator> ();
  operator_no_selection->setUndoStack (undo_stack_.get ());

  bool result = operator_no_selection->moveByTicks (100.0);
  EXPECT_FALSE (result);
}

// Test moveNotesByPitch without undo stack set (should return false)
TEST_F (ArrangerObjectSelectionOperatorTest, MoveNotesByPitchWithoutUndoStack)
{
  auto operator_no_stack = std::make_unique<ArrangerObjectSelectionOperator> ();
  operator_no_stack->setSelectionModel (selection_model_.get ());

  bool result = operator_no_stack->moveNotesByPitch (5);
  EXPECT_FALSE (result);
}

// Test moveAutomationPointsByDelta without undo stack set (should return false)
TEST_F (
  ArrangerObjectSelectionOperatorTest,
  MoveAutomationPointsByDeltaWithoutUndoStack)
{
  auto operator_no_stack = std::make_unique<ArrangerObjectSelectionOperator> ();
  operator_no_stack->setSelectionModel (selection_model_.get ());

  bool result = operator_no_stack->moveAutomationPointsByDelta (0.1);
  EXPECT_FALSE (result);
}

// Test setUndoStack with null (should throw)
TEST_F (ArrangerObjectSelectionOperatorTest, SetNullUndoStackThrows)
{
  EXPECT_THROW (operator_->setUndoStack (nullptr), std::invalid_argument);
}

// Test setSelectionModel with null (should throw)
TEST_F (ArrangerObjectSelectionOperatorTest, SetNullSelectionModelThrows)
{
  EXPECT_THROW (operator_->setSelectionModel (nullptr), std::invalid_argument);
}

// Test signal emissions
TEST_F (ArrangerObjectSelectionOperatorTest, SignalEmissions)
{
  // Test undoStackChanged signal
  bool stack_changed = false;
  QObject::connect (
    operator_.get (), &ArrangerObjectSelectionOperator::undoStackChanged,
    operator_.get (), [&] () { stack_changed = true; });

  auto new_stack = create_mock_undo_stack ();
  operator_->setUndoStack (new_stack.get ());
  EXPECT_TRUE (stack_changed);

  // Test selectionModelChanged signal
  bool selection_changed = false;
  QObject::connect (
    operator_.get (), &ArrangerObjectSelectionOperator::selectionModelChanged,
    operator_.get (), [&] () { selection_changed = true; });

  auto new_selection_model =
    std::make_unique<QItemSelectionModel> (&list_model_);
  operator_->setSelectionModel (new_selection_model.get ());
  EXPECT_TRUE (selection_changed);
}

} // namespace zrythm::actions
