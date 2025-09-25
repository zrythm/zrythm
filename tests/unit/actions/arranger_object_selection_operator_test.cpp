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

// TODO: Test moveByTicks with negative delta
#if 0
TEST_F (ArrangerObjectSelectionOperatorTest, MoveByTicksNegativeDelta)
{
  const double tick_delta = -50.0;

  bool result = operator_->moveByTicks (tick_delta);
  EXPECT_TRUE (result);

  // Objects should be moved backward by tick_delta
  for (size_t i = 0; i < test_objects_.size (); ++i)
    {
      if (auto * obj = test_objects_[i].get_object_base ())
        {
          EXPECT_DOUBLE_EQ (
            obj->position ()->ticks (), original_positions_[i] + tick_delta);
        }
    }

  // Command should be pushed to undo stack
  EXPECT_EQ (undo_stack_->count (), 1);
}
#endif

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
