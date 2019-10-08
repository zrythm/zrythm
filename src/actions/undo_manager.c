/*
 * actions/undo_redo_manager.c - Undo/Redo Manager
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/widgets/header_notebook.h"
#include "gui/widgets/home_toolbar.h"
#include "gui/widgets/main_window.h"
#include "actions/undoable_action.h"
#include "actions/undo_manager.h"
#include "project.h"

#define MAX_UNDO_STACK_LENGTH 64

void
undo_manager_init (UndoManager * self)
{
  g_message ("Initializing undo manager...");
  self->undo_stack =
    stack_new (MAX_UNDO_STACK_LENGTH);
  self->redo_stack =
    stack_new (MAX_UNDO_STACK_LENGTH);
  self->undo_stack->top = -1;
  self->redo_stack->top = -1;
  stack_is_empty (self->undo_stack);
}

/**
 * Undo last action.
 */
void
undo_manager_undo (UndoManager * self)
{
  g_warn_if_fail (!stack_is_empty (self->undo_stack));

  /* pop the action from the undo stack and undo it */
  UndoableAction * action =
    (UndoableAction *) stack_pop (self->undo_stack);
  /* if error return. this should never happen and
   * anything that happens afterwards is undefined */
  if (undoable_action_undo (action))
    {
      g_warn_if_reached ();
      return;
    }

  /* if the redo stack is full, delete the last element */
  if (stack_is_full (self->redo_stack))
    {
      UndoableAction * action_to_delete =
        (UndoableAction *) stack_pop_last (
          self->redo_stack);
      undoable_action_free (action_to_delete);
    }

  /* push action to the redo stack */
  STACK_PUSH (self->redo_stack,
              action);
}

/**
 * Redo last undone action.
 */
void
undo_manager_redo (UndoManager * self)
{
  g_warn_if_fail (
    !stack_is_empty (self->redo_stack));

  /* pop the action from the redo stack and execute it */
  UndoableAction * action =
    (UndoableAction *) stack_pop (self->redo_stack);

  /* if error return. this should never happen and
   * anything that happens afterwards is undefined */
  if (undoable_action_do (action))
    {
      g_warn_if_reached ();
      return;
    }

  /* if the undo stack is full, delete the last element */
  if (stack_is_full (self->undo_stack))
    {
      UndoableAction * action_to_delete =
        (UndoableAction *) stack_pop_last (
          self->undo_stack);
      undoable_action_free (action_to_delete);
    }

  /* push action to the undo stack */
  STACK_PUSH (self->undo_stack,
              action);
}

/**
 * Does performs the action and pushes it to the
 * undo stack.
 */
void
undo_manager_perform (UndoManager *    self,
                      UndoableAction * action)
{
  /* if error return */
  if (undoable_action_do (action))
    {
      g_message ("action not performed");
      return;
    }

  STACK_PUSH (self->undo_stack,
              action);
  self->redo_stack->top = -1;
  if (MAIN_WINDOW && MW_HEADER_NOTEBOOK &&
      MW_HOME_TOOLBAR)
    home_toolbar_widget_refresh_undo_redo_buttons (
      MW_HOME_TOOLBAR);
}
