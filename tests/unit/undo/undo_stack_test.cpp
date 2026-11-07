// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/command.h"
#include "undo/undo_stack.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::undo
{

class UndoStackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Keep the event dispatcher alive during teardown of timer-bearing
    // objects
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    undo_stack_ = std::make_unique<UndoStack> (
      [this] (const auto &callback, bool recalculate_graph) {
        ++pause_request_count_;
        last_requested_graph_recalculation_ = recalculate_graph;
        callback ();
      });
  }

  // Command that toggles a flag so undo/redo effects are observable
  class ToggleCommand : public commands::Command
  {
  public:
    explicit ToggleCommand (bool &flag, const QString &text)
        : Command (text, EngineRequirement::None), flag_ (flag)
    {
    }

    void redo () override { flag_ = true; }
    void undo () override { flag_ = false; }

  private:
    bool &flag_;
  };

  // Command whose only behavior is its engine requirement
  class RequirementCommand : public commands::Command
  {
  public:
    explicit RequirementCommand (EngineRequirement requirement)
        : Command (QStringLiteral ("Requirement"), requirement)
    {
    }

    void redo () override { }
    void undo () override { }
  };

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<UndoStack>                            undo_stack_;

  // Engine cooperation requested by the stack so far
  int  pause_request_count_ = 0;
  bool last_requested_graph_recalculation_ = false;
};

TEST_F (UndoStackTest, EmptyMacroIsDiscarded)
{
  undo_stack_->beginMacro (QStringLiteral ("Empty Macro"));
  undo_stack_->endMacro ();

  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
  EXPECT_FALSE (undo_stack_->canRedo ());
}

TEST_F (UndoStackTest, EndMacroWithoutBeginMacroIsIgnored)
{
  // Unmatched endMacro: warns and no-ops without touching the stack
  undo_stack_->endMacro ();

  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_FALSE (undo_stack_->macroActive ());

  // A following balanced macro still works
  bool flag = false;
  undo_stack_->beginMacro (QStringLiteral ("Macro"));
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Toggle")));
  undo_stack_->endMacro ();

  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (flag);
  EXPECT_FALSE (undo_stack_->macroActive ());
}

TEST_F (UndoStackTest, MacroContainsPushedCommands)
{
  bool flag1 = false;
  bool flag2 = false;

  undo_stack_->beginMacro (QStringLiteral ("My Macro"));
  undo_stack_->push (new ToggleCommand (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (new ToggleCommand (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->endMacro ();

  EXPECT_TRUE (flag1);
  EXPECT_TRUE (flag2);
  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canUndo ());
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("My Macro"));

  const auto * macro = undo_stack_->command (0);
  ASSERT_NE (macro, nullptr);
  EXPECT_EQ (macro->childCount (), 2);

  // Whole macro is undone/redone as a single step
  undo_stack_->undo ();
  EXPECT_FALSE (flag1);
  EXPECT_FALSE (flag2);
  EXPECT_FALSE (undo_stack_->canUndo ());
  EXPECT_TRUE (undo_stack_->canRedo ());

  undo_stack_->redo ();
  EXPECT_TRUE (flag1);
  EXPECT_TRUE (flag2);
}

TEST_F (UndoStackTest, NestedMacros)
{
  bool flag = false;

  undo_stack_->beginMacro (QStringLiteral ("Outer"));
  undo_stack_->beginMacro (QStringLiteral ("Inner"));
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  undo_stack_->endMacro ();
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Outer"));

  const auto * outer = undo_stack_->command (0);
  ASSERT_EQ (outer->childCount (), 1);
  const auto * inner = outer->child (0);
  EXPECT_EQ (inner->text (), QStringLiteral ("Inner"));
  EXPECT_EQ (inner->childCount (), 1);
}

TEST_F (UndoStackTest, NestedEmptyInnerMacroIsDiscarded)
{
  bool flag = false;

  undo_stack_->beginMacro (QStringLiteral ("Outer"));
  undo_stack_->beginMacro (QStringLiteral ("Inner"));
  // No push inside the inner macro - it must be discarded
  undo_stack_->endMacro ();
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  const auto * outer = undo_stack_->command (0);
  ASSERT_EQ (outer->childCount (), 1);
  EXPECT_EQ (outer->child (0)->text (), QStringLiteral ("Cmd"));
}

TEST_F (UndoStackTest, PendingMacroNestsInsideRealizedMacro)
{
  bool flag1 = false;
  bool flag2 = false;

  undo_stack_->beginMacro (QStringLiteral ("Outer"));
  // Realize the outer macro
  undo_stack_->push (new ToggleCommand (flag1, QStringLiteral ("Cmd 1")));
  // Inner macro stays pending until the next push
  undo_stack_->beginMacro (QStringLiteral ("Inner"));
  undo_stack_->push (new ToggleCommand (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->endMacro ();
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Outer"));

  const auto * outer = undo_stack_->command (0);
  ASSERT_EQ (outer->childCount (), 2);
  EXPECT_EQ (outer->child (0)->text (), QStringLiteral ("Cmd 1"));
  const auto * inner = outer->child (1);
  EXPECT_EQ (inner->text (), QStringLiteral ("Inner"));
  ASSERT_EQ (inner->childCount (), 1);
  EXPECT_EQ (inner->child (0)->text (), QStringLiteral ("Cmd 2"));

  // Whole stack undoes/redoes as a single step
  undo_stack_->undo ();
  EXPECT_FALSE (flag1);
  EXPECT_FALSE (flag2);
  undo_stack_->redo ();
  EXPECT_TRUE (flag1);
  EXPECT_TRUE (flag2);
}

TEST_F (UndoStackTest, PendingEmptyInnerMacroDiscardedInsideRealizedMacro)
{
  bool flag = false;

  undo_stack_->beginMacro (QStringLiteral ("Outer"));
  // Realize the outer macro
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  // Pending inner macro that never receives a command must be discarded
  undo_stack_->beginMacro (QStringLiteral ("Inner"));
  undo_stack_->endMacro ();
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  const auto * outer = undo_stack_->command (0);
  ASSERT_EQ (outer->childCount (), 1);
  EXPECT_EQ (outer->child (0)->text (), QStringLiteral ("Cmd"));
}

TEST_F (UndoStackTest, UndoRedoIgnoredWhileMacroPending)
{
  bool flag = false;
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  ASSERT_TRUE (undo_stack_->canUndo ());

  undo_stack_->beginMacro (QStringLiteral ("Pending"));

  // Undo/redo during an unrealized macro must not touch the stack
  undo_stack_->undo ();
  EXPECT_TRUE (flag);
  EXPECT_TRUE (undo_stack_->canUndo ());
  undo_stack_->redo ();
  EXPECT_TRUE (flag);
  EXPECT_FALSE (undo_stack_->canRedo ());

  // Discard the empty macro - stack state is unchanged
  undo_stack_->endMacro ();
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canUndo ());
}

TEST_F (UndoStackTest, UndoRedoIgnoredWhileMacroRealized)
{
  undo_stack_->beginMacro (QStringLiteral ("Macro"));

  // Realize the macro with a push
  bool flag = false;
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  ASSERT_TRUE (undo_stack_->macroActive ());

  // Undo/redo must remain blocked until the macro is closed
  undo_stack_->undo ();
  EXPECT_TRUE (flag);
  undo_stack_->redo ();
  EXPECT_TRUE (flag);

  undo_stack_->endMacro ();

  // Macro is closed - undo works again and reverts the whole macro
  ASSERT_TRUE (undo_stack_->canUndo ());
  undo_stack_->undo ();
  EXPECT_FALSE (flag);
}

TEST_F (UndoStackTest, MacroActiveTracking)
{
  EXPECT_FALSE (undo_stack_->macroActive ());

  // Pending (unrealized) macro
  undo_stack_->beginMacro (QStringLiteral ("Pending"));
  EXPECT_TRUE (undo_stack_->macroActive ());

  // Realized macro
  bool flag = false;
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  EXPECT_TRUE (undo_stack_->macroActive ());

  undo_stack_->endMacro ();
  EXPECT_FALSE (undo_stack_->macroActive ());

  // Unbalanced endMacro() is a no-op
  undo_stack_->endMacro ();
  EXPECT_FALSE (undo_stack_->macroActive ());
}

TEST_F (UndoStackTest, ScopedMacroWithPush)
{
  bool flag = false;
  {
    UndoStack::ScopedMacro macro (*undo_stack_, QStringLiteral ("Scoped"));
    undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  }

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Scoped"));
  EXPECT_TRUE (flag);
}

TEST_F (UndoStackTest, ScopedMacroWithoutPush)
{
  {
    UndoStack::ScopedMacro macro (*undo_stack_, QStringLiteral ("Scoped"));
  }

  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
}

TEST_F (UndoStackTest, SetIndexClampsOutOfRangeValues)
{
  bool flag1 = false;
  bool flag2 = false;
  undo_stack_->push (new ToggleCommand (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (new ToggleCommand (flag2, QStringLiteral ("Cmd 2")));
  ASSERT_EQ (undo_stack_->index (), 2);

  // negative index clamps to 0 (everything undone)
  undo_stack_->setIndex (-5);
  EXPECT_EQ (undo_stack_->index (), 0);
  EXPECT_FALSE (flag1);
  EXPECT_FALSE (flag2);

  // index past the end clamps to count() (everything redone)
  undo_stack_->setIndex (undo_stack_->count () + 10);
  EXPECT_EQ (undo_stack_->index (), undo_stack_->count ());
  EXPECT_TRUE (flag1);
  EXPECT_TRUE (flag2);
}

TEST_F (UndoStackTest, UndoRedoOnEmptyStackIsIgnored)
{
  // must warn and return instead of dereferencing a null command
  undo_stack_->undo ();
  undo_stack_->redo ();

  EXPECT_EQ (undo_stack_->index (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
  EXPECT_FALSE (undo_stack_->canRedo ());
}

// A push from inside a command's redo() (a UI control reacting to a value
// the command applies by pushing an echo edit) must be refused. QUndoStack
// is not re-entrant: executing the nested push would corrupt the command
// list.
TEST_F (UndoStackTest, PushFromInsideRedoIsRefused)
{
  class EchoingCommand : public commands::Command
  {
  public:
    EchoingCommand (UndoStack &stack, bool &echo_flag)
        : Command (QString (), EngineRequirement::None), stack_ (stack),
          echo_flag_ (echo_flag)
    {
    }

    void redo () override
    {
      stack_.push (new ToggleCommand (echo_flag_, QStringLiteral ("Echo")));
    }
    void undo () override { }

  private:
    UndoStack &stack_;
    bool      &echo_flag_;
  };

  bool echo_flag = false;
  undo_stack_->push (new EchoingCommand (*undo_stack_, echo_flag));

  // Only the outer command is on the stack and the echo never executed
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_FALSE (echo_flag);
}

TEST_F (UndoStackTest, UndoFromInsideRedoIsRefused)
{
  class UndoingCommand : public commands::Command
  {
  public:
    explicit UndoingCommand (UndoStack &stack)
        : Command (QString (), EngineRequirement::None), stack_ (stack)
    {
    }

    void redo () override { stack_.undo (); }
    void undo () override { }

  private:
    UndoStack &stack_;
  };

  bool flag = false;
  undo_stack_->push (new ToggleCommand (flag, QStringLiteral ("Cmd")));
  undo_stack_->push (new UndoingCommand (*undo_stack_));

  // The nested undo() was refused: both commands remain done
  EXPECT_EQ (undo_stack_->count (), 2);
  EXPECT_EQ (undo_stack_->index (), 2);
  EXPECT_TRUE (flag);

  // Regular undo still works afterwards
  undo_stack_->undo ();
  undo_stack_->undo ();
  EXPECT_FALSE (flag);
  EXPECT_EQ (undo_stack_->index (), 0);
}

TEST_F (UndoStackTest, NoneRequirementRunsCommandWithoutEngineCooperation)
{
  undo_stack_->push (
    new RequirementCommand (commands::Command::EngineRequirement::None));
  EXPECT_EQ (pause_request_count_, 0);
}

TEST_F (UndoStackTest, PauseProcessingCommandRequestsPauseWithoutGraphRecalc)
{
  using commands::Command;

  undo_stack_->push (
    new RequirementCommand (Command::EngineRequirement::PauseProcessing));
  EXPECT_EQ (pause_request_count_, 1);
  EXPECT_FALSE (last_requested_graph_recalculation_);

  undo_stack_->undo ();
  EXPECT_EQ (pause_request_count_, 2);
  EXPECT_FALSE (last_requested_graph_recalculation_);

  undo_stack_->redo ();
  EXPECT_EQ (pause_request_count_, 3);
  EXPECT_FALSE (last_requested_graph_recalculation_);
}

TEST_F (UndoStackTest, RecalculateGraphCommandRequestsPauseWithGraphRecalc)
{
  using commands::Command;

  undo_stack_->push (
    new RequirementCommand (Command::EngineRequirement::RecalculateGraph));
  EXPECT_EQ (pause_request_count_, 1);
  EXPECT_TRUE (last_requested_graph_recalculation_);

  undo_stack_->undo ();
  EXPECT_EQ (pause_request_count_, 2);
  EXPECT_TRUE (last_requested_graph_recalculation_);

  undo_stack_->redo ();
  EXPECT_EQ (pause_request_count_, 3);
  EXPECT_TRUE (last_requested_graph_recalculation_);
}

// A macro runs with the strongest requirement any of its children declares
TEST_F (UndoStackTest, MacroAggregatesStrongestChildRequirement)
{
  using commands::Command;

  // Pause-only children: undo/redo of the macro requests a pause without
  // graph recalculation
  undo_stack_->beginMacro (QStringLiteral ("Pause Macro"));
  undo_stack_->push (new RequirementCommand (Command::EngineRequirement::None));
  undo_stack_->push (
    new RequirementCommand (Command::EngineRequirement::PauseProcessing));
  undo_stack_->endMacro ();
  // only the PauseProcessing push cooperated with the engine
  EXPECT_EQ (pause_request_count_, 1);

  undo_stack_->undo ();
  EXPECT_EQ (pause_request_count_, 2);
  EXPECT_FALSE (last_requested_graph_recalculation_);

  // One graph-recalculating child upgrades the whole macro to a graph
  // recalculation
  undo_stack_->beginMacro (QStringLiteral ("Graph Macro"));
  undo_stack_->push (
    new RequirementCommand (Command::EngineRequirement::PauseProcessing));
  undo_stack_->push (
    new RequirementCommand (Command::EngineRequirement::RecalculateGraph));
  undo_stack_->endMacro ();
  EXPECT_EQ (pause_request_count_, 4);

  undo_stack_->undo ();
  EXPECT_EQ (pause_request_count_, 5);
  EXPECT_TRUE (last_requested_graph_recalculation_);
}

} // namespace zrythm::undo
