// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <utility>

#include "utils/format_qt.h"

#include "commands/command.h"
#include "undo/undo_stack.h"
#include "utils/logger.h"
#include "utils/serialization.h"
#include "utils/utf8_string.h"

namespace zrythm::undo
{
namespace
{
// Marks the stack as executing for the lifetime of the guard, covering
// the synchronous execution of a push, undo or redo and the bookkeeping
// that follows it. Guards nest: the depth returns to zero when the last
// guard closes.
class ScopedExecution
{
public:
  explicit ScopedExecution (int &depth) : depth_ (depth) { ++depth_; }
  ~ScopedExecution () { --depth_; }
  Q_DISABLE_COPY_MOVE (ScopedExecution)

private:
  int &depth_;
};

std::string
to_utf8 (const QString &text)
{
  return utils::Utf8String::from_qstring (text).str ();
}
} // namespace

UndoStack::UndoStack (
  CallbackWithPausedEngineRequester callback_with_paused_engine_requester,
  QObject *                         parent)
    : QObject (parent),
      callback_with_paused_engine_requester_ (
        std::move (callback_with_paused_engine_requester))
{
}

void
UndoStack::dispatch_with_engine_cooperation (
  commands::Command::EngineRequirement requirement,
  utils::InplaceCallback               action)
{
  if (requirement == commands::Command::EngineRequirement::None)
    {
      action ();
    }
  else
    {
      callback_with_paused_engine_requester_ (
        std::move (action),
        requirement == commands::Command::EngineRequirement::RecalculateGraph);
    }
}

void
UndoStack::execute_with_engine_pause_if_needed (
  const commands::Command &cmd,
  utils::InplaceCallback   action)
{
  dispatch_with_engine_cooperation (
    cmd.engine_requirement (), std::move (action));
}

void
UndoStack::beginMacro (const QString &text)
{
  if (transaction_depth_ > 0)
    {
      z_warning (
        "Refusing beginMacro() while a transaction is open: group commands inside the transaction instead");
      return;
    }
  ++open_macro_count_;
  pending_macros_.emplace_back (text);
  // opening a macro disables undo/redo while it is open
  emit_changed ();
}

void
UndoStack::endMacro ()
{
  if (open_macro_count_ <= 0)
    {
      z_warning ("endMacro() called without a matching beginMacro()");
      return;
    }
  if (transaction_depth_ > 0)
    {
      z_warning (
        "Refusing endMacro() while a transaction is open: use commit() or abort() on the transaction");
      return;
    }
  ScopedExecution executing (executing_depth_);
  close_one_open_macro ();
}

void
UndoStack::close_one_open_macro ()
{
  --open_macro_count_;
  if (!pending_macros_.empty ())
    {
      // the macro never received a command - discard it
      pending_macros_.pop_back ();
      emit_changed ();
      return;
    }
  // an aborted inner transaction can leave this macro without children;
  // an empty macro must not become a history entry
  if (macro_stack_.back ()->child_count () == 0)
    {
      splice_out_open_macro ();
      emit_changed ();
      return;
    }
  end_macro_core ();
}

void
UndoStack::splice_out_open_macro ()
{
  macro_stack_.pop_back ();
  if (macro_stack_.empty ())
    {
      entries_.pop_back ();
    }
  else
    {
      macro_stack_.back ()->remove_last_child ();
    }
}

void
UndoStack::begin_macro_core (const QString &text)
{
  auto macro = std::make_unique<commands::MacroCommand> (text);
  commands::MacroCommand * macro_ptr = macro.get ();
  if (macro_stack_.empty ())
    {
      // opening a root macro discards the redo history
      entries_.resize (index_);
      entries_.push_back ({ std::move (macro), 0 });
    }
  else
    {
      macro_stack_.back ()->add_child (std::move (macro));
    }
  macro_stack_.push_back (macro_ptr);
}

void
UndoStack::end_macro_core ()
{
  macro_stack_.pop_back ();
  if (macro_stack_.empty ())
    {
      // closing the root macro completes it as one history entry
      check_undo_limit ();
      ++index_;
      emit_changed ();
    }
}

void
UndoStack::push (std::unique_ptr<commands::Command> cmd)
{
  if (executing_depth_ > 0)
    {
      z_critical (
        "Refusing nested push of '{}' while the undo stack is executing another command: undo/redo must not trigger new edits",
        cmd->text ());
      return;
    }
  const auto cmd_text = cmd->text ();
  z_debug ("Performing action '{}'", cmd_text);
  // Read the requirement before the command moves into the callback
  const auto requirement = cmd->engine_requirement ();
  bool       invoked = false;
  // The callback owns the command: a pause requester that drops it without
  // invoking it destroys the command instead of leaking it. Pending macros
  // are realized inside the callback, so a dropped callback also leaves the
  // history untouched.
  auto action = [this, &invoked, owned_cmd = std::move (cmd)] () mutable {
    invoked = true;
    ScopedExecution executing (executing_depth_);
    for (const auto &macro_text : pending_macros_)
      {
        begin_macro_core (macro_text);
      }
    pending_macros_.clear ();
    push_core (std::move (owned_cmd));
  };
  dispatch_with_engine_cooperation (requirement, std::move (action));
  if (!invoked)
    {
      z_warning (
        "The engine refused to run with processing paused; edit '{}' was discarded",
        cmd_text);
    }
}

void
UndoStack::push_core (std::unique_ptr<commands::Command> cmd)
{
  const auto cmd_text = to_utf8 (cmd->text ());
  cmd->redo ();

  const bool inside_macro = !macro_stack_.empty ();

  commands::Command * merge_candidate = nullptr;
  if (inside_macro)
    {
      merge_candidate = macro_stack_.back ()->last_child ();
    }
  else
    {
      if (index_ > 0)
        {
          merge_candidate = entries_[index_ - 1].command.get ();
        }
      // pushing discards the redo history above the current index
      entries_.resize (index_);
    }

  // on a successful merge the command is destroyed at the end of this scope
  const auto merged =
    merge_candidate != nullptr && merge_candidate->merge_with (*cmd);
  if (!merged)
    {
      if (inside_macro)
        {
          macro_stack_.back ()->add_child (std::move (cmd));
        }
      else
        {
          entries_.push_back ({ std::move (cmd), 0 });
          check_undo_limit ();
          ++index_;
        }
    }
  record_mutation (JournalEntryKind::Push, { std::move (cmd_text) });
  // the push touches one root-level history entry: a plain entry
  // (appended or merged into) or the open root macro
  if (macro_stack_.empty ())
    {
      entries_[index_ - 1].last_push_revision = revision_;
    }
  else
    {
      entries_.back ().last_push_revision = revision_;
    }
  // while a macro is being composed, closing the root macro emits once for
  // the whole macro
  if (macro_stack_.empty ())
    {
      emit_changed ();
    }
}

bool
UndoStack::revert_top_entry ()
{
  const auto &entry = entries_[index_ - 1];
  const auto  undone_text = to_utf8 (entry.command->text ());
  bool        invoked = false;
  execute_with_engine_pause_if_needed (
    *entry.command, [this, &undone_text, &invoked] () {
      invoked = true;
      ScopedExecution executing (executing_depth_);
      entries_[index_ - 1].command->undo ();
      --index_;
      record_mutation (JournalEntryKind::Undo, { undone_text });
      emit_changed ();
    });
  return invoked;
}

void
UndoStack::undo ()
{
  if (open_macro_count_ > 0)
    {
      z_warning ("Ignoring undo() while a macro is open");
      return;
    }
  if (index_ == 0)
    {
      z_warning ("Ignoring undo() with nothing to undo");
      return;
    }
  if (executing_depth_ > 0)
    {
      z_critical (
        "Refusing nested undo() while the undo stack is executing another command: undo/redo must not trigger further undos");
      return;
    }
  z_debug ("Undoing");
  if (!revert_top_entry ())
    {
      z_warning (
        "The engine refused to run with processing paused; the undo did not run");
    }
}

bool
UndoStack::apply_next_entry ()
{
  const auto &entry = entries_[index_];
  const auto  redone_text = to_utf8 (entry.command->text ());
  bool        invoked = false;
  execute_with_engine_pause_if_needed (
    *entry.command, [this, &redone_text, &invoked] () {
      invoked = true;
      ScopedExecution executing (executing_depth_);
      entries_[index_].command->redo ();
      ++index_;
      record_mutation (JournalEntryKind::Redo, { redone_text });
      emit_changed ();
    });
  return invoked;
}

void
UndoStack::redo ()
{
  if (open_macro_count_ > 0)
    {
      z_warning ("Ignoring redo() while a macro is open");
      return;
    }
  if (index_ >= count ())
    {
      z_warning ("Ignoring redo() with nothing to redo");
      return;
    }
  if (executing_depth_ > 0)
    {
      z_critical (
        "Refusing nested redo() while the undo stack is executing another command: undo/redo must not trigger further undos");
      return;
    }
  z_debug ("Redoing");
  if (!apply_next_entry ())
    {
      z_warning (
        "The engine refused to run with processing paused; the redo did not run");
    }
}

void
UndoStack::set_undo_limit (int limit)
{
  undo_limit_ = limit;
  emit_changed ();
}

void
UndoStack::check_undo_limit ()
{
  if (undo_limit_ <= 0 || !macro_stack_.empty ())
    {
      return;
    }
  const auto command_count = static_cast<int> (entries_.size ());
  if (undo_limit_ >= command_count)
    {
      return;
    }
  const auto excess = command_count - undo_limit_;
  entries_.erase (
    entries_.begin (), std::ranges::next (entries_.begin (), excess));
  index_ -= excess;
}

void
UndoStack::emit_changed ()
{
  Q_EMIT undoActionsChanged ();
  Q_EMIT redoActionsChanged ();
  Q_EMIT indexChanged ();
  Q_EMIT canUndoChanged ();
  Q_EMIT canRedoChanged ();
}

void
UndoStack::set_journal_callback (JournalCallback callback)
{
  journal_callback_ = std::move (callback);
}

void
UndoStack::record_mutation (
  JournalEntryKind         kind,
  std::vector<std::string> command_texts)
{
  ++revision_;
  if (journal_callback_)
    {
      journal_callback_ (
        JournalEntry{ revision_, kind, std::move (command_texts) });
    }
}

const commands::Command *
UndoStack::command (int index) const
{
  if (index < 0 || index >= count ())
    {
      return nullptr;
    }
  return entries_[index].command.get ();
}

QString
UndoStack::text (int idx) const
{
  if (idx < 0 || idx >= count ())
    {
      return {};
    }
  return entries_[idx].command->text ();
}

QStringList
UndoStack::undoActions ()
{
  QStringList actions;

  for (int i = 0; i < index_; ++i)
    {
      actions.prepend (entries_[i].command->text ());
    }

  return actions;
}

QStringList
UndoStack::redoActions ()
{
  QStringList actions;

  // the open root macro is the last entry; it is still being composed and
  // is not redoable
  const auto effective_count = macro_stack_.empty () ? count () : count () - 1;

  for (int i = index_; i < effective_count; ++i)
    {
      actions.append (entries_[i].command->text ());
    }

  return actions;
}

void
UndoStack::setIndex (int idx)
{
  if (idx < 0 || idx > count ())
    {
      z_warning (
        "Clamping undo stack index {} to valid range 0..{}", idx, count ());
      idx = std::clamp (idx, 0, count ());
    }
  if (idx == index ())
    return;

  // stop at the first step the engine refuses instead of issuing the
  // remaining doomed requests
  const auto step_towards = [this, idx] () {
    const auto index_before = index_;
    if (idx > index_)
      {
        redo ();
      }
    else
      {
        undo ();
      }
    return index_ != index_before;
  };
  while (index_ != idx)
    {
      if (!step_towards ())
        {
          z_warning (
            "setIndex({}) stopped at index {}: the engine refused to run with processing paused",
            idx, index_);
          return;
        }
    }
}

std::optional<UndoStack::Transaction>
UndoStack::begin_transaction (const QString &label)
{
  if (executing_depth_ > 0)
    {
      z_critical (
        "Refusing begin_transaction() while the undo stack is executing another command");
      return std::nullopt;
    }
  if (open_macro_count_ > transaction_depth_)
    {
      z_warning (
        "Refusing begin_transaction() while a plain macro is open: end the macro before starting a transaction");
      return std::nullopt;
    }
  const auto pending_size = static_cast<int> (pending_macros_.size ());
  const auto open_count = open_macro_count_;
  const auto tx_depth = transaction_depth_;
  ++open_macro_count_;
  ++transaction_depth_;
  pending_macros_.emplace_back (label);
  emit_changed ();
  // constructed here (the constructor is private) and moved into the
  // optional, whose construction context is not a friend
  Transaction transaction (
    *this, label, pending_size, open_count, tx_depth, revision_);
  return std::optional<Transaction> (std::move (transaction));
}

bool
UndoStack::discard_from (std::uint64_t from_revision)
{
  if (from_revision == 0 || from_revision > revision_)
    {
      z_warning (
        "Ignoring discard_from(): revision {} is outside the journal range 1..{}",
        from_revision, revision_);
      return false;
    }
  if (open_macro_count_ > 0)
    {
      z_warning ("Ignoring discard_from() while a macro or transaction is open");
      return false;
    }
  if (executing_depth_ > 0)
    {
      z_critical (
        "Refusing discard_from() while the undo stack is executing another command");
      return false;
    }

  // the guard spans the reverts and the bookkeeping tail, so change
  // signals firing during the emissions cannot edit mid-discard
  ScopedExecution executing (executing_depth_);

  // entries complete in order, so the entries touched at from_revision or
  // later form a suffix
  const auto first = std::ranges::find_if (
    entries_, [from_revision] (const HistoryEntry &entry) {
      return entry.last_push_revision >= from_revision;
    });
  if (first == entries_.end ())
    {
      // no history entry was touched at from_revision or later
      return true;
    }
  const auto first_index =
    static_cast<int> (std::ranges::distance (entries_.begin (), first));

  // revert the still-applied entries being dropped, newest first; each
  // revert is journaled as an undo step
  while (index_ > first_index)
    {
      if (!revert_top_entry ())
        {
          z_warning (
            "discard_from() stopped: the engine refused an undo; the reverted entries stay undone and redoable");
          return false;
        }
    }

  // drop the entries: they can no longer be redone
  auto dropped_texts =
    entries_ | std::views::drop (first_index)
    | std::views::transform ([] (const HistoryEntry &entry) {
        return to_utf8 (entry.command->text ());
      })
    | std::ranges::to<std::vector> ();
  entries_.erase (first, entries_.end ());
  record_mutation (JournalEntryKind::Abort, std::move (dropped_texts));
  emit_changed ();
  return true;
}

UndoStack::Transaction::Transaction (
  UndoStack    &stack,
  QString       label,
  int           pending_size,
  int           open_macro_count_at_begin,
  int           transaction_depth_at_begin,
  std::uint64_t revision_at_begin)
    : stack_ (stack), label_ (std::move (label)), pending_size_ (pending_size),
      open_macro_count_at_begin_ (open_macro_count_at_begin),
      transaction_depth_at_begin_ (transaction_depth_at_begin),
      revision_at_begin_ (revision_at_begin)
{
}

UndoStack::Transaction::Transaction (Transaction &&other) noexcept
    : stack_ (other.stack_), label_ (std::move (other.label_)),
      pending_size_ (other.pending_size_),
      open_macro_count_at_begin_ (other.open_macro_count_at_begin_),
      transaction_depth_at_begin_ (other.transaction_depth_at_begin_),
      revision_at_begin_ (other.revision_at_begin_), state_ (other.state_)
{
  other.state_ = State::MovedFrom;
}

UndoStack::Transaction::~Transaction ()
{
  if (state_ == State::Open && !abort ())
    {
      z_critical (
        "Transaction '{}' could not be aborted on destruction; the stack is left with an open transaction",
        label_);
    }
}

bool
UndoStack::Transaction::commit ()
{
  if (state_ != State::Open)
    {
      z_warning ("Ignoring commit(): the transaction already ended");
      return false;
    }
  if (stack_.transaction_depth_ != transaction_depth_at_begin_ + 1)
    {
      z_warning (
        "Refusing to commit transaction '{}': an inner transaction is still open",
        label_);
      return false;
    }
  state_ = State::Committed;
  --stack_.transaction_depth_;
  ScopedExecution executing (stack_.executing_depth_);
  stack_.close_one_open_macro ();
  return true;
}

bool
UndoStack::Transaction::abort ()
{
  if (state_ != State::Open)
    {
      z_warning ("Ignoring abort(): the transaction already ended");
      return false;
    }
  if (stack_.transaction_depth_ != transaction_depth_at_begin_ + 1)
    {
      z_warning (
        "Refusing to abort transaction '{}': an inner transaction is still open",
        label_);
      return false;
    }
  // the guard spans the reverts and the bookkeeping tail, so change
  // signals firing during the final emission cannot edit mid-abort
  ScopedExecution executing (stack_.executing_depth_);

  // no command ran inside the transaction: discard its pending macro and
  // record nothing. The transaction's macro is realized once the
  // open-macro stack grows past its slot (open_macro_count_at_begin_).
  if (
    static_cast<int> (stack_.macro_stack_.size ()) <= open_macro_count_at_begin_)
    {
      state_ = State::Aborted;
      --stack_.transaction_depth_;
      stack_.pending_macros_.resize (pending_size_);
      stack_.open_macro_count_ = open_macro_count_at_begin_;
      stack_.emit_changed ();
      return true;
    }

  // revert every command pushed since this transaction began, then
  // splice the transaction's macro tree out of the history
  auto * tx_macro = stack_.macro_stack_[open_macro_count_at_begin_];
  bool   invoked = false;
  stack_.execute_with_engine_pause_if_needed (*tx_macro, [tx_macro, &invoked] () {
    invoked = true;
    tx_macro->undo ();
  });
  if (!invoked)
    {
      z_warning (
        "The engine refused to run with processing paused; transaction '{}' stays open and can be aborted again",
        label_);
      return false;
    }

  state_ = State::Aborted;
  --stack_.transaction_depth_;
  if (open_macro_count_at_begin_ > 0)
    {
      // the transaction's macro is the last child of its parent macro;
      // the root entry's last push revision reverts to before this
      // transaction began
      stack_.entries_.back ().last_push_revision = revision_at_begin_;
    }
  stack_.splice_out_open_macro ();
  stack_.open_macro_count_ = open_macro_count_at_begin_;
  stack_.record_mutation (JournalEntryKind::Abort, { to_utf8 (label_) });
  stack_.emit_changed ();
  return true;
}

void
to_json (nlohmann::json &j, const UndoStack &u)
{
  j = nlohmann::json::object ();
  // TODO: serialize undo stack commands
}

void
from_json (const nlohmann::json &j, UndoStack &u)
{
  // TODO
}
}
