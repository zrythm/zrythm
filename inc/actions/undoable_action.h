/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
  UNDOABLE_ACTION_TYPE_EDIT_TRACK,
  UNDOABLE_ACTION_TYPE_CREATE_PLUGIN,
  UNDOABLE_ACTION_TYPE_MOVE_PLUGIN,
  UNDOABLE_ACTION_TYPE_COPY_PLUGIN,
  UNDOABLE_ACTION_TYPE_DELETE_PLUGIN,

  /**
   * Delete selections on timeline.
   */
  UNDOABLE_ACTION_TYPE_DELETE_TL_SELECTIONS,
  UNDOABLE_ACTION_TYPE_CREATE_TL_SELECTIONS,
  UNDOABLE_ACTION_TYPE_MOVE_TL_SELECTIONS,
  UNDOABLE_ACTION_TYPE_DUPLICATE_TL_SELECTIONS,

  UNDOABLE_ACTION_TYPE_DELETE_MA_SELECTIONS,
  UNDOABLE_ACTION_TYPE_CREATE_MA_SELECTIONS,
  UNDOABLE_ACTION_TYPE_MOVE_MA_SELECTIONS,
  UNDOABLE_ACTION_TYPE_DUPLICATE_MA_SELECTIONS,
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
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_do (UndoableAction * self);

/**
 * Undoes the action.
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_undo (UndoableAction * self);

void
undoable_action_free (UndoableAction * self);

/**
 * Stringizes the action to be used in Undo/Redo
 * buttons.
 *
 * The string MUST be free'd using g_free().
 */
char *
undoable_action_stringize (
  UndoableActionType type);

#endif
