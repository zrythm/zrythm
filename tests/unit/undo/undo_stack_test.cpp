// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/command.h"
#include "undo/undo_stack.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

namespace zrythm::undo
{

class UndoStackTestBase : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Keep the event dispatcher alive during teardown of timer-bearing
    // objects
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    undo_stack_ = std::make_unique<UndoStack> (
      [this] (auto callback, bool recalculate_graph) {
        ++pause_request_count_;
        last_requested_graph_recalculation_ = recalculate_graph;
        callback ();
      });
    undo_stack_->set_journal_callback ([this] (const JournalEntry &entry) {
      journal_entries_.push_back (entry);
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

  // Toggle command with a chosen engine requirement, for testing
  // cooperation refusals with observable state
  class PausingToggleCommand : public commands::Command
  {
  public:
    explicit PausingToggleCommand (bool &flag)
        : Command (
            QStringLiteral ("Pausing Toggle"),
            EngineRequirement::PauseProcessing),
          flag_ (flag)
    {
    }

    void redo () override { flag_ = true; }
    void undo () override { flag_ = false; }

  private:
    bool &flag_;
  };

  // Command that absorbs any MergingCommand pushed right after it, like the
  // drag-gesture commands do
  class MergingCommand : public commands::Command
  {
  public:
    explicit MergingCommand (bool &flag, const QString &text)
        : Command (text, EngineRequirement::None), flag_ (flag)
    {
    }

    bool merge_with (const Command &other) override
    {
      const auto * other_cmd = dynamic_cast<const MergingCommand *> (&other);
      if (other_cmd == nullptr)
        return false;
      // undoing this command must also revert the absorbed commands'
      // effects, so track their state alongside this one's
      absorbed_flags_.push_back (&other_cmd->flag_);
      return true;
    }

    int merge_count () const
    {
      return static_cast<int> (absorbed_flags_.size ());
    }

    void redo () override { flag_ = true; }
    void undo () override
    {
      flag_ = false;
      for (bool * absorbed_flag : absorbed_flags_)
        {
          *absorbed_flag = false;
        }
    }

  private:
    bool               &flag_;
    std::vector<bool *> absorbed_flags_;
  };

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;
  std::unique_ptr<UndoStack>                            undo_stack_;

  // Engine cooperation requested by the stack so far
  int  pause_request_count_ = 0;
  bool last_requested_graph_recalculation_ = false;

  // Journal entries received so far
  std::vector<JournalEntry> journal_entries_;
};

class UndoStackTest : public UndoStackTestBase
{
};

// Fixture for the revision, journal, transaction and discard API
class TransactionTest : public UndoStackTestBase
{
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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Toggle")));
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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->endMacro ();

  EXPECT_TRUE (flag1);
  EXPECT_TRUE (flag2);
  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canUndo ());
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("My Macro"));

  const auto * macro =
    dynamic_cast<const commands::MacroCommand *> (undo_stack_->command (0));
  ASSERT_NE (macro, nullptr);
  EXPECT_EQ (macro->child_count (), 2);

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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  undo_stack_->endMacro ();
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Outer"));

  const auto * outer =
    dynamic_cast<const commands::MacroCommand *> (undo_stack_->command (0));
  ASSERT_EQ (outer->child_count (), 1);
  const auto * inner =
    dynamic_cast<const commands::MacroCommand *> (outer->child_at (0));
  ASSERT_NE (inner, nullptr);
  EXPECT_EQ (inner->text (), QStringLiteral ("Inner"));
  EXPECT_EQ (inner->child_count (), 1);
}

TEST_F (UndoStackTest, NestedEmptyInnerMacroIsDiscarded)
{
  bool flag = false;

  undo_stack_->beginMacro (QStringLiteral ("Outer"));
  undo_stack_->beginMacro (QStringLiteral ("Inner"));
  // No push inside the inner macro - it must be discarded
  undo_stack_->endMacro ();
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  const auto * outer =
    dynamic_cast<const commands::MacroCommand *> (undo_stack_->command (0));
  ASSERT_EQ (outer->child_count (), 1);
  EXPECT_EQ (outer->child_at (0)->text (), QStringLiteral ("Cmd"));
}

TEST_F (UndoStackTest, PendingMacroNestsInsideRealizedMacro)
{
  bool flag1 = false;
  bool flag2 = false;

  undo_stack_->beginMacro (QStringLiteral ("Outer"));
  // Realize the outer macro
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  // Inner macro stays pending until the next push
  undo_stack_->beginMacro (QStringLiteral ("Inner"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->endMacro ();
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Outer"));

  const auto * outer =
    dynamic_cast<const commands::MacroCommand *> (undo_stack_->command (0));
  ASSERT_EQ (outer->child_count (), 2);
  EXPECT_EQ (outer->child_at (0)->text (), QStringLiteral ("Cmd 1"));
  const auto * inner =
    dynamic_cast<const commands::MacroCommand *> (outer->child_at (1));
  ASSERT_NE (inner, nullptr);
  EXPECT_EQ (inner->text (), QStringLiteral ("Inner"));
  ASSERT_EQ (inner->child_count (), 1);
  EXPECT_EQ (inner->child_at (0)->text (), QStringLiteral ("Cmd 2"));

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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  // Pending inner macro that never receives a command must be discarded
  undo_stack_->beginMacro (QStringLiteral ("Inner"));
  undo_stack_->endMacro ();
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  const auto * outer =
    dynamic_cast<const commands::MacroCommand *> (undo_stack_->command (0));
  ASSERT_EQ (outer->child_count (), 1);
  EXPECT_EQ (outer->child_at (0)->text (), QStringLiteral ("Cmd"));
}

TEST_F (UndoStackTest, UndoRedoIgnoredWhileMacroPending)
{
  bool flag = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  ASSERT_TRUE (undo_stack_->canUndo ());

  undo_stack_->beginMacro (QStringLiteral ("Pending"));

  // Undo/redo during an unrealized macro must not touch the stack
  undo_stack_->undo ();
  EXPECT_TRUE (flag);
  // canUndo is false while any macro is open, including a pending one
  EXPECT_FALSE (undo_stack_->canUndo ());
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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
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
    undo_stack_->push (
      std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
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
// the command applies by pushing an echo edit) must be refused. The
// history is not re-entrant: executing the nested push would corrupt the
// command list.
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
      stack_.push (
        std::make_unique<ToggleCommand> (echo_flag_, QStringLiteral ("Echo")));
    }
    void undo () override { }

  private:
    UndoStack &stack_;
    bool      &echo_flag_;
  };

  bool echo_flag = false;
  undo_stack_->push (std::make_unique<EchoingCommand> (*undo_stack_, echo_flag));

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
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  undo_stack_->push (std::make_unique<UndoingCommand> (*undo_stack_));

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
    std::make_unique<RequirementCommand> (
      commands::Command::EngineRequirement::None));
  EXPECT_EQ (pause_request_count_, 0);
}

TEST_F (UndoStackTest, PauseProcessingCommandRequestsPauseWithoutGraphRecalc)
{
  using commands::Command;

  undo_stack_->push (
    std::make_unique<RequirementCommand> (
      Command::EngineRequirement::PauseProcessing));
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
    std::make_unique<RequirementCommand> (
      Command::EngineRequirement::RecalculateGraph));
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
  undo_stack_->push (
    std::make_unique<RequirementCommand> (Command::EngineRequirement::None));
  undo_stack_->push (
    std::make_unique<RequirementCommand> (
      Command::EngineRequirement::PauseProcessing));
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
    std::make_unique<RequirementCommand> (
      Command::EngineRequirement::PauseProcessing));
  undo_stack_->push (
    std::make_unique<RequirementCommand> (
      Command::EngineRequirement::RecalculateGraph));
  undo_stack_->endMacro ();
  EXPECT_EQ (pause_request_count_, 4);

  undo_stack_->undo ();
  EXPECT_EQ (pause_request_count_, 5);
  EXPECT_TRUE (last_requested_graph_recalculation_);
}

TEST_F (UndoStackTest, UndoLimitTrimsOldestCommandsOnPush)
{
  undo_stack_->set_undo_limit (3);
  bool flag1 = false;
  bool flag2 = false;
  bool flag3 = false;
  bool flag4 = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag3, QStringLiteral ("Cmd 3")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag4, QStringLiteral ("Cmd 4")));

  ASSERT_EQ (undo_stack_->count (), 3);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Cmd 2"));
  EXPECT_EQ (undo_stack_->index (), 3);

  undo_stack_->undo ();
  undo_stack_->undo ();
  undo_stack_->undo ();
  EXPECT_EQ (undo_stack_->index (), 0);
  EXPECT_FALSE (flag2);
  EXPECT_FALSE (flag3);
  EXPECT_FALSE (flag4);
  // the evicted command's effect is permanent: its undo never runs again
  EXPECT_TRUE (flag1);
}

TEST_F (UndoStackTest, UndoLimitChangeKeepsExistingEntriesUntilNextPush)
{
  bool flag1 = false;
  bool flag2 = false;
  bool flag3 = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag3, QStringLiteral ("Cmd 3")));

  // lowering the limit deletes nothing on its own
  undo_stack_->set_undo_limit (2);
  EXPECT_EQ (undo_stack_->count (), 3);
  EXPECT_EQ (undo_stack_->index (), 3);

  // the next push trims the history down to the new limit
  bool flag4 = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag4, QStringLiteral ("Cmd 4")));
  EXPECT_EQ (undo_stack_->count (), 2);
  EXPECT_EQ (undo_stack_->index (), 2);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Cmd 3"));
}

TEST_F (UndoStackTest, MergeInsideMacroFoldsIntoLastChild)
{
  bool flag_a = false;
  bool flag_b = false;

  undo_stack_->beginMacro (QStringLiteral ("Macro"));
  auto first =
    std::make_unique<MergingCommand> (flag_a, QStringLiteral ("Merge 1"));
  undo_stack_->push (std::move (first));
  // the second merge folds into the macro's last child (the first merge)
  undo_stack_->push (
    std::make_unique<MergingCommand> (flag_b, QStringLiteral ("Merge 2")));
  undo_stack_->endMacro ();

  ASSERT_EQ (undo_stack_->count (), 1);
  const auto * macro =
    dynamic_cast<const commands::MacroCommand *> (undo_stack_->command (0));
  ASSERT_NE (macro, nullptr);
  EXPECT_EQ (macro->child_count (), 1);
  EXPECT_TRUE (flag_a);
  EXPECT_TRUE (flag_b);

  undo_stack_->undo ();
  EXPECT_FALSE (flag_a);
  EXPECT_FALSE (flag_b);
}

TEST_F (UndoStackTest, PushAfterUndoDiscardsRedoHistoryAndOffersMerge)
{
  bool flag_a = false;
  bool flag_t = false;
  bool flag_b = false;

  auto first =
    std::make_unique<MergingCommand> (flag_a, QStringLiteral ("Merge 1"));
  const auto * first_cmd = first.get ();
  undo_stack_->push (std::move (first));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag_t, QStringLiteral ("Toggle")));
  undo_stack_->undo ();
  ASSERT_FALSE (flag_t);
  ASSERT_TRUE (undo_stack_->canRedo ());

  // the push discards the redoable toggle and merges into the merge below
  undo_stack_->push (
    std::make_unique<MergingCommand> (flag_b, QStringLiteral ("Merge 2")));

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_FALSE (undo_stack_->canRedo ());
  EXPECT_EQ (first_cmd->merge_count (), 1);
  EXPECT_TRUE (flag_a);
  EXPECT_TRUE (flag_b);

  undo_stack_->undo ();
  // the absorbed command's effect folds into the absorber's undo
  EXPECT_FALSE (flag_a);
}

TEST_F (UndoStackTest, CommandDestroyedWhenEngineDropsCallback)
{
  // a requester that never invokes the callback, like an engine refusing
  // to pause during an offline render
  undo::UndoStack stack ([] (auto &&callback, bool) { (void) callback; });

  class DropTrackingCommand : public commands::Command
  {
  public:
    explicit DropTrackingCommand (bool &destroyed)
        : Command (QStringLiteral ("Dropped"), EngineRequirement::PauseProcessing),
          destroyed_ (destroyed)
    {
    }
    ~DropTrackingCommand () override { destroyed_ = true; }

    void redo () override { }
    void undo () override { }

  private:
    bool &destroyed_;
  };

  bool destroyed = false;
  stack.push (std::make_unique<DropTrackingCommand> (destroyed));

  EXPECT_EQ (stack.count (), 0);
  EXPECT_FALSE (stack.canUndo ());
  EXPECT_TRUE (destroyed);
}

// push reports whether the engine accepted the command: a requester that
// drops the request leaves the flag, the history and the revision untouched
TEST_F (UndoStackTest, PushReportsEngineRefusal)
{
  bool            drop = true;
  undo::UndoStack stack ([&drop] (auto &&callback, bool) {
    if (!drop)
      callback ();
  });

  bool flag = false;
  EXPECT_FALSE (stack.push (std::make_unique<PausingToggleCommand> (flag)));
  EXPECT_FALSE (flag);
  EXPECT_EQ (stack.count (), 0);
  EXPECT_EQ (stack.revision (), 0);

  drop = false;
  EXPECT_TRUE (stack.push (std::make_unique<PausingToggleCommand> (flag)));
  EXPECT_TRUE (flag);
  EXPECT_EQ (stack.count (), 1);
  EXPECT_EQ (stack.revision (), 1);
}

TEST_F (UndoStackTest, CanUndoFalseWhileRealizedMacroOpen)
{
  bool flag1 = false;
  bool flag2 = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  ASSERT_TRUE (undo_stack_->canUndo ());

  undo_stack_->beginMacro (QStringLiteral ("Macro"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  EXPECT_FALSE (undo_stack_->canUndo ());
  EXPECT_FALSE (undo_stack_->canRedo ());

  undo_stack_->endMacro ();
  EXPECT_TRUE (undo_stack_->canUndo ());
}

TEST_F (UndoStackTest, EngineRefusedPushInsideMacroLeavesHistoryUntouched)
{
  // a requester that never invokes the callback, like an engine refusing
  // to pause during an offline render
  undo::UndoStack stack ([] (auto &&callback, bool) { (void) callback; });

  bool flag = false;
  stack.push (std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd 1")));
  stack.undo ();
  ASSERT_FALSE (flag);
  ASSERT_TRUE (stack.canRedo ());

  stack.beginMacro (QStringLiteral ("Macro"));
  stack.push (
    std::make_unique<RequirementCommand> (
      commands::Command::EngineRequirement::PauseProcessing));
  stack.endMacro ();

  // the refused push changed nothing: the redo history is intact and no
  // empty macro entry was committed
  EXPECT_EQ (stack.count (), 1);
  EXPECT_EQ (stack.index (), 0);
  EXPECT_TRUE (stack.canRedo ());
  EXPECT_FALSE (stack.canUndo ());
  EXPECT_FALSE (stack.macroActive ());

  stack.redo ();
  EXPECT_TRUE (flag);
}

TEST_F (UndoStackTest, RedoActionsHidesMacroBeingComposed)
{
  bool flag = false;
  undo_stack_->beginMacro (QStringLiteral ("Composing"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Inner")));

  // the macro is realized but still open: it must not appear as redoable
  ASSERT_TRUE (undo_stack_->macroActive ());
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->redoActions ().size (), 0);

  undo_stack_->endMacro ();
  // the closed macro is one undoable step
  ASSERT_EQ (undo_stack_->undoActions ().size (), 1);
  EXPECT_EQ (undo_stack_->undoActions ().at (0), QStringLiteral ("Composing"));

  undo_stack_->undo ();
  ASSERT_EQ (undo_stack_->redoActions ().size (), 1);
  EXPECT_EQ (undo_stack_->redoActions ().at (0), QStringLiteral ("Composing"));
}

TEST_F (UndoStackTest, MacroCompositionEmitsOnceAtClose)
{
  QSignalSpy spy (undo_stack_.get (), &UndoStack::redoActionsChanged);

  bool flag1 = false;
  bool flag2 = false;
  undo_stack_->beginMacro (QStringLiteral ("Macro"));
  // beginMacro emits because canUndo/canRedo change while a macro is open
  EXPECT_EQ (spy.size (), 1);
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  EXPECT_EQ (spy.size (), 1);

  // closing the root macro emits once for the whole composition
  undo_stack_->endMacro ();
  EXPECT_EQ (spy.size (), 2);
}

// ---------------------------------------------------------------------------
// Revisions and journal
// ---------------------------------------------------------------------------

TEST_F (TransactionTest, JournalRecordsEveryMutationWithMonotonicRevisions)
{
  bool flag1 = false;
  bool flag2 = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->undo ();
  undo_stack_->redo ();

  ASSERT_EQ (journal_entries_.size (), 4);
  EXPECT_EQ (journal_entries_[0].revision, 1);
  EXPECT_EQ (journal_entries_[0].kind, JournalEntryKind::Push);
  EXPECT_EQ (journal_entries_[0].command_texts.at (0), "Cmd 1");
  EXPECT_EQ (journal_entries_[1].revision, 2);
  EXPECT_EQ (journal_entries_[1].kind, JournalEntryKind::Push);
  EXPECT_EQ (journal_entries_[2].revision, 3);
  EXPECT_EQ (journal_entries_[2].kind, JournalEntryKind::Undo);
  EXPECT_EQ (journal_entries_[2].command_texts.at (0), "Cmd 2");
  EXPECT_EQ (journal_entries_[3].revision, 4);
  EXPECT_EQ (journal_entries_[3].kind, JournalEntryKind::Redo);
  EXPECT_EQ (undo_stack_->revision (), 4);
}

TEST_F (TransactionTest, RefusedMutationLeavesRevisionAndJournalUntouched)
{
  // a requester that never invokes the callback, like an engine refusing
  // to pause during an offline render
  undo::UndoStack stack ([] (auto &&callback, bool) { (void) callback; });
  std::vector<JournalEntry> entries;
  stack.set_journal_callback ([&entries] (const JournalEntry &entry) {
    entries.push_back (entry);
  });

  stack.push (
    std::make_unique<RequirementCommand> (
      commands::Command::EngineRequirement::PauseProcessing));

  EXPECT_EQ (stack.revision (), 0);
  EXPECT_TRUE (entries.empty ());
}

TEST_F (TransactionTest, MergedPushStillBumpsRevisionAndJournals)
{
  bool flag_a = false;
  bool flag_b = false;
  undo_stack_->push (
    std::make_unique<MergingCommand> (flag_a, QStringLiteral ("Merge 1")));
  undo_stack_->push (
    std::make_unique<MergingCommand> (flag_b, QStringLiteral ("Merge 2")));

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->revision (), 2);
  ASSERT_EQ (journal_entries_.size (), 2);
  EXPECT_EQ (journal_entries_[1].revision, 2);
  EXPECT_EQ (journal_entries_[1].kind, JournalEntryKind::Push);
  EXPECT_EQ (journal_entries_[1].command_texts.at (0), "Merge 2");
}

// ---------------------------------------------------------------------------
// Transactions
// ---------------------------------------------------------------------------

TEST_F (TransactionTest, EmptyTransactionCommitsNothing)
{
  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Empty"));
  ASSERT_TRUE (tx.has_value ());
  EXPECT_TRUE (tx->commit ());

  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
  EXPECT_EQ (undo_stack_->revision (), 0);
  EXPECT_TRUE (journal_entries_.empty ());
}

TEST_F (TransactionTest, EmptyTransactionAbortsCleanly)
{
  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Empty"));
  ASSERT_TRUE (tx.has_value ());
  EXPECT_TRUE (tx->abort ());

  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_FALSE (undo_stack_->macroActive ());
  EXPECT_EQ (undo_stack_->revision (), 0);
  EXPECT_TRUE (journal_entries_.empty ());
}

TEST_F (TransactionTest, CommitMakesTheGroupOneUndoStep)
{
  bool flag1 = false;
  bool flag2 = false;
  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Group"));
  ASSERT_TRUE (tx.has_value ());
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  EXPECT_FALSE (undo_stack_->canUndo ());
  ASSERT_TRUE (tx->commit ());

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Group"));
  const auto undo_actions = undo_stack_->undoActions ();
  ASSERT_EQ (undo_actions.size (), 1);
  EXPECT_EQ (undo_actions.at (0), QStringLiteral ("Group"));

  // the journal records the pushes; closing the group records nothing
  ASSERT_EQ (journal_entries_.size (), 2);
  EXPECT_EQ (undo_stack_->revision (), 2);

  undo_stack_->undo ();
  EXPECT_FALSE (flag1);
  EXPECT_FALSE (flag2);
  undo_stack_->redo ();
  EXPECT_TRUE (flag1);
  EXPECT_TRUE (flag2);
}

TEST_F (TransactionTest, AbortRevertsCommandsAndDropsThem)
{
  bool flag1 = false;
  bool flag2 = false;
  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Group"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  ASSERT_TRUE (tx->abort ());

  EXPECT_FALSE (flag1);
  EXPECT_FALSE (flag2);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (undo_stack_->index (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
  EXPECT_FALSE (undo_stack_->canRedo ());
  EXPECT_FALSE (undo_stack_->macroActive ());

  ASSERT_EQ (journal_entries_.size (), 3);
  EXPECT_EQ (journal_entries_[2].revision, 3);
  EXPECT_EQ (journal_entries_[2].kind, JournalEntryKind::Abort);
  ASSERT_EQ (journal_entries_[2].command_texts.size (), 1);
  EXPECT_EQ (journal_entries_[2].command_texts.at (0), "Group");
}

TEST_F (TransactionTest, RevisionsKeepCountingAfterAnAbort)
{
  bool flag = false;
  {
    auto tx = undo_stack_->begin_transaction (QStringLiteral ("Doomed"));
    undo_stack_->push (
      std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd 1")));
    ASSERT_TRUE (tx->abort ());
  }
  ASSERT_EQ (undo_stack_->revision (), 2);

  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd 2")));
  EXPECT_EQ (undo_stack_->revision (), 3);
  ASSERT_EQ (journal_entries_.size (), 3);
  EXPECT_EQ (journal_entries_[2].revision, 3);
  EXPECT_EQ (journal_entries_[2].kind, JournalEntryKind::Push);
  EXPECT_EQ (journal_entries_[2].command_texts.at (0), "Cmd 2");
}

TEST_F (TransactionTest, DestructorAbortsAnOpenTransaction)
{
  bool flag = false;
  {
    auto tx = undo_stack_->begin_transaction (QStringLiteral ("Scoped"));
    undo_stack_->push (
      std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  }

  EXPECT_FALSE (flag);
  EXPECT_EQ (undo_stack_->count (), 0);
  ASSERT_EQ (journal_entries_.size (), 2);
  EXPECT_EQ (journal_entries_[1].kind, JournalEntryKind::Abort);
}

TEST_F (TransactionTest, AbortingInnerTransactionKeepsOuterCommands)
{
  bool flag_outer = false;
  bool flag_inner = false;
  auto outer = undo_stack_->begin_transaction (QStringLiteral ("Outer"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag_outer, QStringLiteral ("Cmd 1")));
  auto inner = undo_stack_->begin_transaction (QStringLiteral ("Inner"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag_inner, QStringLiteral ("Cmd 2")));
  ASSERT_TRUE (inner->abort ());

  EXPECT_FALSE (flag_inner);
  EXPECT_TRUE (flag_outer);
  EXPECT_TRUE (undo_stack_->macroActive ());
  ASSERT_TRUE (outer->commit ());

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Outer"));
  ASSERT_EQ (journal_entries_.size (), 3);
  EXPECT_EQ (journal_entries_[2].kind, JournalEntryKind::Abort);
  EXPECT_EQ (journal_entries_[2].command_texts.at (0), "Inner");

  undo_stack_->undo ();
  EXPECT_FALSE (flag_outer);
}

TEST_F (TransactionTest, AbortingOuterTransactionRevertsCommittedInner)
{
  bool flag1 = false;
  bool flag2 = false;
  bool flag3 = false;
  auto outer = undo_stack_->begin_transaction (QStringLiteral ("Outer"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  {
    auto inner = undo_stack_->begin_transaction (QStringLiteral ("Inner"));
    undo_stack_->push (
      std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
    ASSERT_TRUE (inner->commit ());
  }
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag3, QStringLiteral ("Cmd 3")));
  ASSERT_TRUE (outer->abort ());

  EXPECT_FALSE (flag1);
  EXPECT_FALSE (flag2);
  EXPECT_FALSE (flag3);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (undo_stack_->revision (), 4);
}

TEST_F (TransactionTest, OuterCommitAndAbortRefusedWhileInnerOpen)
{
  auto outer = undo_stack_->begin_transaction (QStringLiteral ("Outer"));
  auto inner = undo_stack_->begin_transaction (QStringLiteral ("Inner"));

  EXPECT_FALSE (outer->commit ());
  EXPECT_FALSE (outer->abort ());
  EXPECT_TRUE (outer->is_open ());
  EXPECT_TRUE (undo_stack_->macroActive ());

  EXPECT_TRUE (inner->abort ());
  EXPECT_TRUE (outer->commit ());
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TransactionTest, EngineRefusedAbortKeepsTransactionOpen)
{
  bool                      drop = false;
  undo::UndoStack           stack ([&drop] (auto &&callback, bool) {
    if (!drop)
      callback ();
  });
  std::vector<JournalEntry> entries;
  stack.set_journal_callback ([&entries] (const JournalEntry &entry) {
    entries.push_back (entry);
  });

  auto tx = stack.begin_transaction (QStringLiteral ("Tx"));
  // the pause-requiring command makes the abort need an engine pause
  stack.push (
    std::make_unique<RequirementCommand> (
      commands::Command::EngineRequirement::PauseProcessing));
  ASSERT_EQ (stack.revision (), 1);

  drop = true;
  EXPECT_FALSE (tx->abort ());
  EXPECT_TRUE (tx->is_open ());
  EXPECT_TRUE (stack.macroActive ());
  EXPECT_EQ (stack.revision (), 1);

  drop = false;
  EXPECT_TRUE (tx->abort ());
  EXPECT_EQ (stack.count (), 0);
  EXPECT_EQ (stack.revision (), 2);
  ASSERT_EQ (entries.size (), 2);
  EXPECT_EQ (entries[1].kind, JournalEntryKind::Abort);
}

TEST_F (TransactionTest, EmptyRealizedTransactionIsSplicedOnCommit)
{
  // both transactions are realized by the same push; aborting the inner
  // one leaves the outer realized without children
  auto outer = undo_stack_->begin_transaction (QStringLiteral ("Outer"));
  auto inner = undo_stack_->begin_transaction (QStringLiteral ("Inner"));
  undo_stack_->push (
    std::make_unique<RequirementCommand> (
      commands::Command::EngineRequirement::None));
  ASSERT_TRUE (inner->abort ());
  ASSERT_TRUE (outer->commit ());

  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
  EXPECT_EQ (undo_stack_->revision (), 2);
}

TEST_F (TransactionTest, BeginTransactionRefusedWhilePlainMacroOpen)
{
  undo_stack_->beginMacro (QStringLiteral ("Macro"));
  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Tx"));
  EXPECT_FALSE (tx.has_value ());
  undo_stack_->endMacro ();

  EXPECT_FALSE (undo_stack_->macroActive ());
}

TEST_F (TransactionTest, PlainMacrosRefusedWhileTransactionOpen)
{
  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Tx"));
  ASSERT_TRUE (tx.has_value ());

  undo_stack_->beginMacro (QStringLiteral ("Refused"));
  undo_stack_->endMacro ();
  EXPECT_TRUE (undo_stack_->macroActive ());

  bool flag = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  ASSERT_TRUE (tx->commit ());

  ASSERT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->text (0), QStringLiteral ("Tx"));
}

TEST_F (TransactionTest, TransactionCanBeMoved)
{
  auto tx1 = undo_stack_->begin_transaction (QStringLiteral ("Tx"));
  bool flag = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));

  auto tx2 = std::move (*tx1);
  EXPECT_FALSE (tx1->is_open ());
  EXPECT_TRUE (tx2.is_open ());

  EXPECT_FALSE (tx1->abort ());
  ASSERT_TRUE (tx2.abort ());
  EXPECT_FALSE (flag);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// ---------------------------------------------------------------------------
// discard_from()
// ---------------------------------------------------------------------------

TEST_F (TransactionTest, DiscardFromRevertsAndDropsTouchedEntries)
{
  bool flag1 = false;
  bool flag2 = false;
  bool flag3 = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag3, QStringLiteral ("Cmd 3")));

  ASSERT_TRUE (undo_stack_->discard_from (2));

  EXPECT_TRUE (flag1);
  EXPECT_FALSE (flag2);
  EXPECT_FALSE (flag3);
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (undo_stack_->index (), 1);
  EXPECT_FALSE (undo_stack_->canRedo ());

  // the two reverts are journaled as undo steps, the drop as one abort
  ASSERT_EQ (journal_entries_.size (), 6);
  EXPECT_EQ (journal_entries_[3].kind, JournalEntryKind::Undo);
  EXPECT_EQ (journal_entries_[3].command_texts.at (0), "Cmd 3");
  EXPECT_EQ (journal_entries_[4].kind, JournalEntryKind::Undo);
  EXPECT_EQ (journal_entries_[4].command_texts.at (0), "Cmd 2");
  EXPECT_EQ (journal_entries_[5].kind, JournalEntryKind::Abort);
  ASSERT_EQ (journal_entries_[5].command_texts.size (), 2);
  EXPECT_EQ (journal_entries_[5].command_texts.at (0), "Cmd 2");
  EXPECT_EQ (journal_entries_[5].command_texts.at (1), "Cmd 3");
  EXPECT_EQ (undo_stack_->revision (), 6);
}

TEST_F (TransactionTest, DiscardFromDropsRedoEntriesWithoutReverting)
{
  bool flag1 = false;
  bool flag2 = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->undo ();
  ASSERT_FALSE (flag2);
  ASSERT_EQ (undo_stack_->index (), 1);

  ASSERT_TRUE (undo_stack_->discard_from (2));

  // the second command was already undone: it is dropped without running
  // its undo again
  EXPECT_TRUE (flag1);
  EXPECT_FALSE (flag2);
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_FALSE (undo_stack_->canRedo ());
  ASSERT_EQ (journal_entries_.size (), 4);
  EXPECT_EQ (journal_entries_[3].kind, JournalEntryKind::Abort);
}

TEST_F (TransactionTest, DiscardFromMidMacroDropsTheWholeMacro)
{
  bool flag1 = false;
  bool flag2 = false;
  bool flag3 = false;
  undo_stack_->beginMacro (QStringLiteral ("My Macro"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));
  undo_stack_->endMacro ();
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag3, QStringLiteral ("Cmd 3")));

  // revision 2 is a push inside the macro: the macro is dropped whole
  ASSERT_TRUE (undo_stack_->discard_from (2));

  EXPECT_FALSE (flag1);
  EXPECT_FALSE (flag2);
  EXPECT_FALSE (flag3);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
}

TEST_F (TransactionTest, DiscardFromRefusesInvalidRevisions)
{
  EXPECT_FALSE (undo_stack_->discard_from (0));
  EXPECT_FALSE (undo_stack_->discard_from (1));

  bool flag = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));
  undo_stack_->beginMacro (QStringLiteral ("Macro"));
  EXPECT_FALSE (undo_stack_->discard_from (1));
  undo_stack_->endMacro ();

  // a revision only used by undo/redo matches no entry: nothing is
  // dropped and the call succeeds
  undo_stack_->undo ();
  EXPECT_TRUE (undo_stack_->discard_from (2));
  EXPECT_EQ (undo_stack_->count (), 1);
}

// A change-signal slot firing while the stack is still finishing a
// mutation must not be able to edit: the edit would land around the
// mutation's own journal entry
TEST_F (TransactionTest, ReentrantPushDuringAbortEmissionIsRefused)
{
  bool flag = false;
  bool echo_flag = false;
  auto tx = undo_stack_->begin_transaction (QStringLiteral ("Tx"));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag, QStringLiteral ("Cmd")));

  bool slot_ran = false;
  QObject::connect (
    undo_stack_.get (), &UndoStack::canRedoChanged, undo_stack_.get (),
    [this, &slot_ran, &echo_flag] () {
      if (slot_ran)
        return;
      slot_ran = true;
      undo_stack_->push (
        std::make_unique<ToggleCommand> (echo_flag, QStringLiteral ("Echo")));
    });

  ASSERT_TRUE (tx->abort ());
  EXPECT_TRUE (slot_ran);
  EXPECT_FALSE (echo_flag);
  EXPECT_EQ (undo_stack_->count (), 0);
  // the refused echo push produced no journal entry; the abort is last
  ASSERT_EQ (journal_entries_.size (), 2);
  EXPECT_EQ (journal_entries_[1].kind, JournalEntryKind::Abort);
  EXPECT_EQ (undo_stack_->revision (), 2);
}

TEST_F (TransactionTest, ReentrantPushDuringDiscardEmissionIsRefused)
{
  bool flag1 = false;
  bool flag2 = false;
  bool echo_flag = false;
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag1, QStringLiteral ("Cmd 1")));
  undo_stack_->push (
    std::make_unique<ToggleCommand> (flag2, QStringLiteral ("Cmd 2")));

  bool slot_ran = false;
  QObject::connect (
    undo_stack_.get (), &UndoStack::canRedoChanged, undo_stack_.get (),
    [this, &slot_ran, &echo_flag] () {
      if (slot_ran)
        return;
      slot_ran = true;
      undo_stack_->push (
        std::make_unique<ToggleCommand> (echo_flag, QStringLiteral ("Echo")));
    });

  // the first emission fires during the inner undo guard of the revert
  // loop, with the outer discard guard active
  ASSERT_TRUE (undo_stack_->discard_from (2));
  EXPECT_TRUE (slot_ran);
  EXPECT_FALSE (echo_flag);
  EXPECT_EQ (undo_stack_->count (), 1);
  ASSERT_EQ (journal_entries_.size (), 4);
  EXPECT_EQ (journal_entries_[3].kind, JournalEntryKind::Abort);
}

// The revert loop stops at the first engine refusal: the reverted
// entries stay undone and redoable, nothing is erased, and no Abort
// entry is recorded
TEST_F (TransactionTest, DiscardFromStopsWhenEngineRefuses)
{
  bool                      drop = false;
  undo::UndoStack           stack ([&drop] (auto &&callback, bool) {
    if (!drop)
      callback ();
  });
  std::vector<JournalEntry> entries;
  stack.set_journal_callback ([&entries] (const JournalEntry &entry) {
    entries.push_back (entry);
  });

  bool pausing_flag = false;
  bool plain_flag = false;
  stack.push (std::make_unique<PausingToggleCommand> (pausing_flag));
  stack.push (
    std::make_unique<ToggleCommand> (plain_flag, QStringLiteral ("Cmd 2")));

  drop = true;
  EXPECT_FALSE (stack.discard_from (1));

  // the None-requirement entry reverted; the pause-requiring one did not
  EXPECT_TRUE (pausing_flag);
  EXPECT_FALSE (plain_flag);
  EXPECT_EQ (stack.index (), 1);
  EXPECT_TRUE (stack.canRedo ());
  EXPECT_EQ (stack.count (), 2);
  ASSERT_EQ (entries.size (), 3);
  EXPECT_EQ (entries[2].kind, JournalEntryKind::Undo);

  // the discard completes once the engine cooperates again
  drop = false;
  EXPECT_TRUE (stack.discard_from (1));
  EXPECT_EQ (stack.count (), 0);
}

TEST_F (TransactionTest, SetIndexStopsWhenEngineRefuses)
{
  bool            drop = false;
  undo::UndoStack stack ([&drop] (auto &&callback, bool) {
    if (!drop)
      callback ();
  });

  bool flag = false;
  stack.push (std::make_unique<PausingToggleCommand> (flag));

  // the undo direction is refused
  drop = true;
  stack.setIndex (0);
  EXPECT_EQ (stack.index (), 1);
  EXPECT_TRUE (flag);

  drop = false;
  stack.setIndex (0);
  EXPECT_EQ (stack.index (), 0);
  EXPECT_FALSE (flag);

  // the redo direction is refused
  drop = true;
  stack.setIndex (1);
  EXPECT_EQ (stack.index (), 0);
}

} // namespace zrythm::undo
