// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <utility>

#include "utils/format_qt.h"

#include "commands/command.h"
#include "undo/undo_stack.h"
#include "utils/logger.h"
#include "utils/serialization.h"

namespace zrythm::undo
{
namespace
{
// Marks the stack as executing for the lifetime of the guard, covering the
// synchronous execution of a push, undo or redo
class ScopedExecution
{
public:
  explicit ScopedExecution (bool &executing) : executing_ (executing)
  {
    executing_ = true;
  }
  ~ScopedExecution () { executing_ = false; }
  Q_DISABLE_COPY_MOVE (ScopedExecution)

private:
  bool &executing_;
};
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
  std::move_only_function<void ()>     action)
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
  const commands::Command         &cmd,
  std::move_only_function<void ()> action)
{
  dispatch_with_engine_cooperation (
    cmd.engine_requirement (), std::move (action));
}

void
UndoStack::beginMacro (const QString &text)
{
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
  --open_macro_count_;
  if (!pending_macros_.empty ())
    {
      // Macro never received a command - discard it
      pending_macros_.pop_back ();
      emit_changed ();
      return;
    }
  end_macro_core ();
}

void
UndoStack::begin_macro_core (const QString &text)
{
  auto macro = std::make_unique<commands::MacroCommand> (text);
  commands::MacroCommand * macro_ptr = macro.get ();
  if (macro_stack_.empty ())
    {
      // opening a root macro discards the redo history
      commands_.resize (index_);
      commands_.push_back (std::move (macro));
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
  if (executing_)
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
    ScopedExecution executing (executing_);
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
          merge_candidate = commands_[index_ - 1].get ();
        }
      // pushing discards the redo history above the current index
      commands_.resize (index_);
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
          commands_.push_back (std::move (cmd));
          check_undo_limit ();
          ++index_;
        }
    }
  // while a macro is being composed, closing the root macro emits once for
  // the whole macro
  if (macro_stack_.empty ())
    {
      emit_changed ();
    }
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
  if (executing_)
    {
      z_critical (
        "Refusing nested undo() while the undo stack is executing another command: undo/redo must not trigger further undos");
      return;
    }
  z_debug ("Undoing");
  execute_with_engine_pause_if_needed (*commands_[index_ - 1], [this] () {
    ScopedExecution executing (executing_);
    commands_[index_ - 1]->undo ();
    --index_;
    emit_changed ();
  });
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
  if (executing_)
    {
      z_critical (
        "Refusing nested redo() while the undo stack is executing another command: undo/redo must not trigger further undos");
      return;
    }
  z_debug ("Redoing");
  execute_with_engine_pause_if_needed (*commands_[index_], [this] () {
    ScopedExecution executing (executing_);
    commands_[index_]->redo ();
    ++index_;
    emit_changed ();
  });
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
  const auto command_count = static_cast<int> (commands_.size ());
  if (undo_limit_ >= command_count)
    {
      return;
    }
  const auto excess = command_count - undo_limit_;
  commands_.erase (
    commands_.begin (), std::ranges::next (commands_.begin (), excess));
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

const commands::Command *
UndoStack::command (int index) const
{
  if (index < 0 || index >= count ())
    {
      return nullptr;
    }
  return commands_[index].get ();
}

QString
UndoStack::text (int idx) const
{
  if (idx < 0 || idx >= count ())
    {
      return {};
    }
  return commands_[idx]->text ();
}

QStringList
UndoStack::undoActions ()
{
  QStringList actions;

  for (int i = 0; i < index_; ++i)
    {
      actions.prepend (commands_[i]->text ());
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
      actions.append (commands_[i]->text ());
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

  const auto cur_idx = index_;
  if (idx > cur_idx)
    {
      const auto num_actions_to_redo = idx - cur_idx;
      for (const auto _ : std::views::iota (0, num_actions_to_redo))
        {
          redo ();
        }
    }
  else
    {
      const auto num_actions_to_undo = cur_idx - idx;
      for (const auto _ : std::views::iota (0, num_actions_to_undo))
        {
          undo ();
        }
    }
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
