/*
 * undo/undoable_action.h - UndoableAction
 *
 * Copyright (C) 2018 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UNDO_UNDOABLE_ACTION_H__
#define __UNDO_UNDOABLE_ACTION_H__

typedef enum UndoableActionType
{
  UNDOABLE_ACTION_TYPE_CREATE_CHANNEL,
  UNDOABLE_ACTION_TYPE_EDIT_CHANNEL,
  UNDOABLE_ACTION_TYPE_DELETE_CHANNEL,
  UNDOABLE_ACTION_TYPE_MOVE_CHANNEL,
  UNDOABLE_ACTION_TYPE_CREATE_REGIONS,
  UNDOABLE_ACTION_TYPE_MOVE_REGIONS,
  UNDOABLE_ACTION_TYPE_DELETE_REGIONS,
} UndoableActionType;

typedef struct UndoableAction
{
  UndoableActionType         type;
  /**
   * Label for showing in the Edit menu.
   *
   * e.g. "move region(s)" -> Undo move region(s)
   */
  char *                      label;
} UndoableAction;

/**
 * Performs the action.
 */
void
undoable_action_do (UndoableAction * self);

/**
 * Undoes the action.
 */
void
undoable_action_undo (UndoableAction * self);

void
undoable_action_free (UndoableAction * self);

#endif
