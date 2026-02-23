// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "commands/add_arranger_object_command.h"
#include "commands/add_plugin_command.h"
#include "commands/add_track_command.h"
#include "commands/change_qobject_property_command.h"
#include "commands/move_arranger_objects_command.h"
#include "commands/route_track_command.h"
#include "undo/undo_stack.h"

#include <QAction>

namespace zrythm::undo
{
UndoStack::UndoStack (
  CallbackWithPausedEngineRequester callback_with_paused_engine_requester,
  QObject *                         parent)
    : QObject (parent), stack_ (utils::make_qobject_unique<QUndoStack> (this)),
      callback_with_paused_engine_requester_ (
        std::move (callback_with_paused_engine_requester))
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
UndoStack::command_or_children_require_graph_recalculation (
  const QUndoCommand &cmd) const
{
  static constexpr std::array<int, 3> command_ids_with_graph_pause = {
    commands::AddEmptyTrackCommand::CommandId,
    commands::AddPluginCommand::CommandId,
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
      return command_or_children_require_graph_recalculation (*child);
    });

  return false;
}

bool
UndoStack::command_or_children_require_engine_pause (
  const QUndoCommand &cmd) const
{
  static constexpr std::array<int, 3> command_ids_with_graph_pause = {
    commands::AddTempoMapAffectingArrangerObjectCommand<
      structure::arrangement::TempoObject>::CommandId,
    commands::MoveTempoMapAffectingArrangerObjectsCommand::CommandId,
    commands::ChangeTempoMapAffectingQObjectPropertyCommand::CommandId,
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
      return command_or_children_require_engine_pause (*child);
    });

  return false;
}

void
UndoStack::execute_with_engine_pause_if_needed (
  const QUndoCommand           &cmd,
  const std::function<void ()> &action)
{
  const auto recalc_graph =
    command_or_children_require_graph_recalculation (cmd);
  const auto pause_engine =
    command_or_children_require_engine_pause (cmd) || recalc_graph;

  if (pause_engine)
    {
      callback_with_paused_engine_requester_ (action, recalc_graph);
    }
  else
    {
      action ();
    }
}

void
UndoStack::push (QUndoCommand * cmd)
{
  z_debug ("Performing action '{}'", cmd->text ());
  execute_with_engine_pause_if_needed (*cmd, [this, cmd] () {
    stack_->push (cmd);
  });
}

void
UndoStack::redo ()
{
  assert (canRedo ());
  assert (stack_->canRedo ());
  z_debug ("Redoing");
  const auto * cmd = stack_->command (stack_->index ());
  execute_with_engine_pause_if_needed (*cmd, [this] () { stack_->redo (); });
}

void
UndoStack::undo ()
{
  assert (canUndo ());
  z_debug ("Undoing");
  const auto * cmd = stack_->command (stack_->index () - 1);
  assert (cmd != nullptr);
  execute_with_engine_pause_if_needed (*cmd, [this] () { stack_->undo (); });
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
  j = nlohmann::json::object ();
  // TODO: serialize undo stack commands
}

void
from_json (const nlohmann::json &j, UndoStack &u)
{
  // TODO
}
}
