// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/qobject_property_operator.h"
#include "undo/undo_stack.h"

#include "helpers/mock_qobject.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class QObjectPropertyOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create test object
    test_obj_ = std::make_unique<MockQObject> ();

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create operator
    operator_ = std::make_unique<QObjectPropertyOperator> ();
    operator_->setCurrentObject (test_obj_.get ());
    operator_->setUndoStack (undo_stack_.get ());
  }

  void TearDown () override
  {
    operator_.reset ();
    undo_stack_.reset ();
    test_obj_.reset ();
  }

  std::unique_ptr<MockQObject>             test_obj_;
  std::unique_ptr<undo::UndoStack>         undo_stack_;
  std::unique_ptr<QObjectPropertyOperator> operator_;
};

TEST_F (QObjectPropertyOperatorTest, InitialState)
{
  EXPECT_EQ (operator_->currentObject (), test_obj_.get ());
  EXPECT_EQ (operator_->undoStack (), undo_stack_.get ());
}

TEST_F (QObjectPropertyOperatorTest, SetCurrentObject)
{
  EXPECT_THROW (operator_->setCurrentObject (nullptr), std::invalid_argument);

  auto new_obj = std::make_unique<MockQObject> ();
  operator_->setCurrentObject (new_obj.get ());
  EXPECT_EQ (operator_->currentObject (), new_obj.get ());
}

TEST_F (QObjectPropertyOperatorTest, SetUndoStack)
{
  EXPECT_THROW (operator_->setUndoStack (nullptr), std::invalid_argument);

  auto new_stack = create_mock_undo_stack ();
  operator_->setUndoStack (new_stack.get ());
  EXPECT_EQ (operator_->undoStack (), new_stack.get ());
}

TEST_F (QObjectPropertyOperatorTest, SetValue)
{
  // Initial state
  EXPECT_EQ (test_obj_->intValue (), 42);
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value
  operator_->setValue (QStringLiteral ("intValue"), 100);
  EXPECT_EQ (test_obj_->intValue (), 100);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Redo
  undo_stack_->redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);
}

TEST_F (QObjectPropertyOperatorTest, SetValueString)
{
  // Initial state
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("initial"));
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value
  operator_->setValue (
    QStringLiteral ("stringValue"), QStringLiteral ("changed"));
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("changed"));
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("initial"));

  // Redo
  undo_stack_->redo ();
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("changed"));
}

TEST_F (QObjectPropertyOperatorTest, SetValueDouble)
{
  // Initial state
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 3.14);
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value
  operator_->setValue (QStringLiteral ("doubleValue"), 2.71);
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 2.71);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 3.14);

  // Redo
  undo_stack_->redo ();
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 2.71);
}

TEST_F (QObjectPropertyOperatorTest, SetValueSameValue)
{
  // Set same value - should still create command
  operator_->setValue (QStringLiteral ("intValue"), 42);
  EXPECT_EQ (test_obj_->intValue (), 42);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (QObjectPropertyOperatorTest, SetValueBoundaryValues)
{
  // Test minimum boundary
  operator_->setValue (QStringLiteral ("intValue"), 0);
  EXPECT_EQ (test_obj_->intValue (), 0);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo and test maximum boundary
  undo_stack_->undo ();
  operator_->setValue (QStringLiteral ("intValue"), 1000);
  EXPECT_EQ (test_obj_->intValue (), 1000);
  EXPECT_EQ (undo_stack_->count (), 1);
}

TEST_F (QObjectPropertyOperatorTest, SetValueNegative)
{
  operator_->setValue (QStringLiteral ("intValue"), -50);
  EXPECT_EQ (test_obj_->intValue (), -50);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (QObjectPropertyOperatorTest, SetValueAffectingTempoMap)
{
  // Initial state
  EXPECT_EQ (test_obj_->intValue (), 42);
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value affecting tempo map
  operator_->setValueAffectingTempoMap (QStringLiteral ("intValue"), 200);
  EXPECT_EQ (test_obj_->intValue (), 200);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Redo
  undo_stack_->redo ();
  EXPECT_EQ (test_obj_->intValue (), 200);
}

TEST_F (QObjectPropertyOperatorTest, SetValueAffectingTempoMapString)
{
  // Initial state
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("initial"));
  EXPECT_EQ (undo_stack_->count (), 0);

  // Set new value affecting tempo map
  operator_->setValueAffectingTempoMap (
    QStringLiteral ("stringValue"), QStringLiteral ("tempo_changed"));
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("tempo_changed"));
  EXPECT_EQ (undo_stack_->count (), 1);

  // Undo
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("initial"));

  // Redo
  undo_stack_->redo ();
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("tempo_changed"));
}

TEST_F (QObjectPropertyOperatorTest, MultipleSetValueOperations)
{
  // First operation
  operator_->setValue (QStringLiteral ("intValue"), 25);
  EXPECT_EQ (test_obj_->intValue (), 25);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Second operation - should merge with first
  operator_->setValue (QStringLiteral ("intValue"), 75);
  EXPECT_EQ (test_obj_->intValue (), 75);
  EXPECT_EQ (undo_stack_->count (), 1); // Still 1 due to merging

  // Undo should restore to original value (not intermediate)
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (QObjectPropertyOperatorTest, DifferentProperties)
{
  // Set int property
  operator_->setValue (QStringLiteral ("intValue"), 123);
  EXPECT_EQ (test_obj_->intValue (), 123);
  EXPECT_EQ (undo_stack_->count (), 1);

  // Set string property
  operator_->setValue (
    QStringLiteral ("stringValue"), QStringLiteral ("different"));
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("different"));
  EXPECT_EQ (undo_stack_->count (), 2);

  // Set double property
  operator_->setValue (QStringLiteral ("doubleValue"), 1.618);
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 1.618);
  EXPECT_EQ (undo_stack_->count (), 3);

  // Undo all operations in reverse order
  undo_stack_->undo ();
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 3.14);

  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("initial"));

  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (QObjectPropertyOperatorTest, SignalEmissions)
{
  // Test currentObjectChanged signal
  bool object_changed = false;
  QObject::connect (
    operator_.get (), &QObjectPropertyOperator::currentObjectChanged,
    operator_.get (), [&] () { object_changed = true; });

  auto new_obj = std::make_unique<MockQObject> ();
  operator_->setCurrentObject (new_obj.get ());
  EXPECT_TRUE (object_changed);

  // Test undoStackChanged signal
  bool stack_changed = false;
  QObject::connect (
    operator_.get (), &QObjectPropertyOperator::undoStackChanged,
    operator_.get (), [&] () { stack_changed = true; });

  auto new_stack = create_mock_undo_stack ();
  operator_->setUndoStack (new_stack.get ());
  EXPECT_TRUE (stack_changed);
}

TEST_F (QObjectPropertyOperatorTest, RapidValueChanges)
{
  // Test that rapid successive changes may be merged into a single command
  // Note: The actual merging depends on the 1-second timeout in mergeWith()
  operator_->setValue (QStringLiteral ("intValue"), 25);
  operator_->setValue (QStringLiteral ("intValue"), 60);
  operator_->setValue (QStringLiteral ("intValue"), 75);

  // Final value should be the last one set
  EXPECT_EQ (test_obj_->intValue (), 75);
  EXPECT_EQ (undo_stack_->count (), 1);

  // After undo, we should return to the original value
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (QObjectPropertyOperatorTest, DifferentObjects)
{
  // Create second object and operator
  auto test_obj2 = std::make_unique<MockQObject> ();
  auto operator2 = std::make_unique<QObjectPropertyOperator> ();
  operator2->setCurrentObject (test_obj2.get ());
  operator2->setUndoStack (undo_stack_.get ());

  // Set values on both objects
  operator_->setValue (QStringLiteral ("intValue"), 100);
  operator2->setValue (QStringLiteral ("intValue"), 200);

  // Both objects should have their respective values
  EXPECT_EQ (test_obj_->intValue (), 100);
  EXPECT_EQ (test_obj2->intValue (), 200);
  EXPECT_EQ (undo_stack_->count (), 2);

  // Undo last operation (should affect second object)
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 100);
  EXPECT_EQ (test_obj2->intValue (), 42);

  // Undo first operation (should affect first object)
  undo_stack_->undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
  EXPECT_EQ (test_obj2->intValue (), 42);
}

} // namespace zrythm::actions
