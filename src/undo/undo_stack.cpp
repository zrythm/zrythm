// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "undo/undo_stack.h"

#include <QAction>

namespace zrythm::undo
{
UndoStack::UndoStack (QObject * parent)
    : QObject (parent), stack_ (utils::make_qobject_unique<QUndoStack> (this))
{
  connect (stack_.get (), &QUndoStack::indexChanged, this, [this] () {
    Q_EMIT undoActionsChanged ();
    Q_EMIT redoActionsChanged ();
    Q_EMIT indexChanged ();
  });
  connect (stack_.get (), &QUndoStack::cleanChanged, this, [this] () {
    Q_EMIT undoActionsChanged ();
    Q_EMIT redoActionsChanged ();
  });
}

void
UndoStack::push (QUndoCommand * cmd)
{
  z_info ("Performing action '{}'", cmd->text ());
  stack_->push (cmd);
}

void
UndoStack::redo ()
{
  z_debug ("Redoing");
  stack_->redo ();
}

void
UndoStack::undo ()
{
  z_debug ("Undoing");
  stack_->undo ();
}

QStringList
UndoStack::undoActions ()
{
  QStringList actions;
  const int   index = stack_->index ();

  for (int i = 0; i < index; ++i)
    {
      actions.append (stack_->text (i));
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

  stack_->setIndex (idx);
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
