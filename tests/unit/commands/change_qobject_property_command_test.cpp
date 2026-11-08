// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
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
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::None);
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (ChangeQObjectPropertyCommandTest, RedoOperation)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::None);

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
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::None);

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
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::None);

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
    *test_obj_, QStringLiteral ("stringValue"), QStringLiteral ("changed"),
    Command::EngineRequirement::None);

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
    *test_obj_, QStringLiteral ("doubleValue"), 2.71,
    Command::EngineRequirement::None);

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
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 42,
    Command::EngineRequirement::None);

  // Initial state
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Redo with same value
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 42);

  // Undo should still restore to original
  cmd.undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

TEST_F (ChangeQObjectPropertyCommandTest, CommandText)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::None);
  QString expected_text =
    QObject::tr ("Change %1").arg (QStringLiteral ("intValue"));
  EXPECT_EQ (cmd.text (), expected_text);
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithSameType)
{
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);
  ChangeQObjectPropertyCommand cmd2 (
    *test_obj_, QStringLiteral ("intValue"), 75,
    Command::EngineRequirement::None);

  // First command
  cmd1.redo ();
  EXPECT_EQ (test_obj_->intValue (), 25);

  // Should merge with second command (within time limit)
  EXPECT_TRUE (cmd1.merge_with (cmd2));

  // Redo should use merged value
  cmd1.redo ();
  EXPECT_EQ (test_obj_->intValue (), 75);
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithDifferentType)
{
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);

  // Create a mock command of a different type
  class MockCommand : public commands::Command
  {
  public:
    MockCommand () : Command (QStringLiteral ("Mock"), EngineRequirement::None)
    {
    }
    void undo () override { }
    void redo () override { }
  } mockCmd;

  // Should not merge with different command type
  EXPECT_FALSE (cmd1.merge_with (mockCmd));
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithDifferentObject)
{
  auto                         test_obj2 = std::make_unique<MockQObject> ();
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);
  ChangeQObjectPropertyCommand cmd2 (
    *test_obj2, QStringLiteral ("intValue"), 75,
    Command::EngineRequirement::None);

  // Should not merge with different object
  EXPECT_FALSE (cmd1.merge_with (cmd2));
}

TEST_F (ChangeQObjectPropertyCommandTest, MergeWithDifferentProperty)
{
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);
  ChangeQObjectPropertyCommand cmd2 (
    *test_obj_, QStringLiteral ("stringValue"), QStringLiteral ("test"),
    Command::EngineRequirement::None);

  // Should not merge with different property
  EXPECT_FALSE (cmd1.merge_with (cmd2));
}

TEST_F (ChangeQObjectPropertyCommandTest, EngineRequirementPassedToConstructor)
{
  ChangeQObjectPropertyCommand plain_cmd (
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::None);
  EXPECT_EQ (plain_cmd.engine_requirement (), Command::EngineRequirement::None);

  ChangeQObjectPropertyCommand tempo_cmd (
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::PauseProcessing);
  EXPECT_EQ (
    tempo_cmd.engine_requirement (),
    Command::EngineRequirement::PauseProcessing);
}

// A tempo-map-affecting change must not merge into a plain one: the merged
// command would lose its engine pause
TEST_F (ChangeQObjectPropertyCommandTest, MergeRefusesDifferingEngineRequirements)
{
  ChangeQObjectPropertyCommand cmd1 (
    *test_obj_, QStringLiteral ("intValue"), 25,
    Command::EngineRequirement::None);
  cmd1.redo ();
  ChangeQObjectPropertyCommand cmd2 (
    *test_obj_, QStringLiteral ("intValue"), 75,
    Command::EngineRequirement::PauseProcessing);
  EXPECT_FALSE (cmd1.merge_with (cmd2));
}

TEST_F (
  ChangeQObjectPropertyCommandTest,
  ChangeTempoMapAffectingBasicFunctionality)
{
  ChangeQObjectPropertyCommand cmd (
    *test_obj_, QStringLiteral ("intValue"), 100,
    Command::EngineRequirement::PauseProcessing);

  // Should work like the plain variant
  cmd.redo ();
  EXPECT_EQ (test_obj_->intValue (), 100);

  cmd.undo ();
  EXPECT_EQ (test_obj_->intValue (), 42);
}

} // namespace zrythm::commands
