// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "commands/add_track_command.h"
#include "commands/route_track_command.h"
#include "undo/undo_stack.h"

#include <QAction>

namespace zrythm::undo
{
UndoStack::UndoStack (
  GraphStopRequester   graph_stop_requester,
  GraphResumeRequester graph_resume_requester,
  QObject *            parent)
    : QObject (parent), stack_ (utils::make_qobject_unique<QUndoStack> (this)),
      graph_stop_requester_ (std::move (graph_stop_requester)),
      graph_resume_requester_ (std::move (graph_resume_requester))
{
  connect (stack_.get (), &QUndoStack::indexChanged, this, [this] () {
    Q_EMIT undoActionsChanged ();
    Q_EMIT redoActionsChanged ();
    Q_EMIT indexChanged ();
    Q_EMIT canUndoChanged ();
    Q_EMIT canRedoChanged ();
  });
  connect (stack_.get (), &QUndoStack::cleanChanged, this, [this] () {
    Q_EMIT undoActionsChanged ();
    Q_EMIT redoActionsChanged ();
    Q_EMIT canUndoChanged ();
    Q_EMIT canRedoChanged ();
  });
}

bool
UndoStack::command_or_children_require_graph_pause (const QUndoCommand &cmd) const
{
  static constexpr std::array<int, 2> command_ids_with_graph_pause = {
    commands::AddEmptyTrackCommand::CommandId,
    commands::RouteTrackCommand::CommandId,
  };

  // return if command itself requires graph pause
  if (
    cmd.id () >= 0
    && std::ranges::contains (command_ids_with_graph_pause, cmd.id ()))
    {
      return true;
    }

  // return if any of its children (recursively) requires pause
  return std::ranges::any_of (
    std::views::iota (0, cmd.childCount ())
      | std::views::transform ([&cmd] (const auto i) { return cmd.child (i); }),
    [this] (const auto * child) {
      return command_or_children_require_graph_pause (*child);
    });

  return false;
}

void
UndoStack::push (UndoCommand * cmd)
{
  z_debug ("Performing action '{}'", cmd->text ());

  EngineState state{};
  const auto  pause_graph = command_or_children_require_graph_pause (*cmd);
  if (pause_graph)
    {
      graph_stop_requester_ (state);
    }
  stack_->push (cmd);
  if (pause_graph)
    {
      graph_resume_requester_ (state);
    }
}

void
UndoStack::redo ()
{
  assert (canRedo ());
  assert (stack_->canRedo ());
  z_debug ("Redoing");
  const auto * cmd = stack_->command (stack_->index ());
  EngineState  state{};
  const auto   pause_graph = command_or_children_require_graph_pause (*cmd);
  if (pause_graph)
    {
      graph_stop_requester_ (state);
    }
  stack_->redo ();
  if (pause_graph)
    {
      graph_resume_requester_ (state);
    }
}

void
UndoStack::undo ()
{
  assert (canUndo ());
  z_debug ("Undoing");
  const auto * cmd = stack_->command (stack_->index () - 1);
  assert (cmd != nullptr);
  EngineState state{};
  const auto  pause_graph = command_or_children_require_graph_pause (*cmd);
  if (pause_graph)
    {
      graph_stop_requester_ (state);
    }
  stack_->undo ();
  if (pause_graph)
    {
      graph_resume_requester_ (state);
    }
}

QStringList
UndoStack::undoActions ()
{
  QStringList actions;
  const int   index = stack_->index ();

  for (int i = 0; i < index; ++i)
    {
      actions.prepend (stack_->text (i));
    }

  return actions;
}

QStringList
UndoStack::redoActions ()
{
  QStringList actions;
  const int   count = stack_->count ();
  const int   index = stack_->index ();

  for (int i = index; i < count; ++i)
    {
      actions.append (stack_->text (i));
    }

  return actions;
}

void
UndoStack::setIndex (int idx)
{
  if (idx == index ())
    return;

  const auto cur_idx = stack_->index ();
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
  // TODO
}

void
from_json (const nlohmann::json &j, UndoStack &u)
{
  // TODO
}
}
