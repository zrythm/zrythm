// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/change_qobject_property_command.h"

#include <QObject>
#include <QProperty>
#include <QTimer>

#include "helpers/mock_qobject.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ChangeQObjectPropertyCommandTest : public ::testing::Test
{
protected:
  void SetUp () override { test_obj_ = std::make_unique<MockQObject> (); }

  void TearDown () override { }

  std::unique_ptr<MockQObject> test_obj_;
};

TEST_F (ChangeQObjectPropertyCommandTest, InitialState)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (ChangeQObjectPropertyCommandTest, RedoOperation)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);

  // Initial state
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Execute redo (first time)
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);

  // Execute redo again (should still work)
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);
}

TEST_F (ChangeQObjectPropertyCommandTest, UndoOperation)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);

  // First execute redo to set the value
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);

  // Then undo
  cmd.undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (ChangeQObjectPropertyCommandTest, UndoRedoCycle)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);

  // Initial state
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Redo
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);

  // Undo
  cmd.undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Redo again
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);

  // Undo again
  cmd.undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (ChangeQObjectPropertyCommandTest, StringProperty)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("stringValue"), QStringLiteral ("changed"));

  // Initial state
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("initial"));

  // Redo
  cmd.redo ();
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("changed"));

  // Undo
  cmd.undo ();
  EXPECT_EQ (test_obj_->stringValue (), QStringLiteral ("initial"));
}

TEST_F (ChangeQObjectPropertyCommandTest, DoubleProperty)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("doubleValue"), 2.71);

  // Initial state
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 3.14);

  // Redo
  cmd.redo ();
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 2.71);

  // Undo
  cmd.undo ();
  EXPECT_DOUBLE_EQ (test_obj_->doubleValue (), 3.14);
}

TEST_F (ChangeQObjectPropertyCommandTest, SameValueNoChange)
{
  ChangeQObjectPropertyCommand cmd (*test_obj_, QStringLiteral ("intValue"), 42);

  // Initial state
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Redo with same value
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Undo should still restore to original
  cmd.undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (ChangeQObjectPropertyCommandTest, CommandId)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);
  EXPECT_EQ (cmd.id (), 1762957285);
}

TEST_F (ChangeQObjectPropertyCommandTest, CommandText)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);
  QString expected_text =
    QObject::tr ("Change %1").arg (QStringLiteral ("intValue"));
  EXPECT_EQ (cmd.text (), expected_text);
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithSameType)
{
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25);
  ChangeQObjectPropertyCommand cmd2 (
    *test_obj_, QStringLiteral ("intValue"), 75);

  // First command
  cmd1.redo ();
  EXPECT_EQ (test_obj_->intValue (), 25);

  // Should merge with second command (within time limit)
  EXPECT_TRUE (cmd1.mergeWith (&cmd2));

  // Redo should use merged value
  cmd1.redo ();
  EXPECT_EQ (test_obj_->intValue (), 75);
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithDifferentType)
{
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25);

  // Create a mock command with different ID
  class MockCommand : public QUndoCommand
  {
  public:
    int id () const override { return 12345; }
  } mockCmd;

  // Should not merge with different command type
  EXPECT_FALSE (cmd1.mergeWith (&mockCmd));
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithDifferentObject)
{
  auto                         test_obj2 = std::make_unique<MockQObject> ();
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25);
  ChangeQObjectPropertyCommand cmd2 (
    *test_obj2, QStringLiteral ("intValue"), 75);

  // Should not merge with different object
  EXPECT_FALSE (cmd1.mergeWith (&cmd2));
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithDifferentProperty)
{
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25);
  ChangeQObjectPropertyCommand cmd2 (
    *test_obj_, QStringLiteral ("stringValue"), QStringLiteral ("test"));

  // Should not merge with different property
  EXPECT_FALSE (cmd1.mergeWith (&cmd2));
}

TEST_F (ChangeQObjectPropertyCommandTest, ChangeTempoMapAffectingCommandId)
{
  ChangeTempoMapAffectingQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);
  EXPECT_EQ (cmd.id (), 1762957664);
}

TEST_F (
  ChangeQObjectPropertyCommandTest,
  ChangeTempoMapAffectingBasicFunctionality)
{
  ChangeTempoMapAffectingQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100);

  // Should work like the base class
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);

  cmd.undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

} // namespace zrythm::commands
