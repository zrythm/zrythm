/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Undoable actions.
 */

#ifndef __UNDO_UNDOABLE_ACTION_H__
#define __UNDO_UNDOABLE_ACTION_H__

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * Type of UndoableAction.
 */
typedef enum UndoableActionType
{
  /* --------- Track/Channel ---------- */
  UA_CREATE_TRACKS,
  UA_MOVE_TRACKS,
  /** Edit track/channel parameters. */
  UA_EDIT_TRACKS,
  UA_COPY_TRACKS,
  UA_DELETE_TRACKS,

  /* ---------------- end ----------------- */

  /* ---------------- Plugin --------------- */

  UA_CREATE_PLUGINS,
  UA_MOVE_PLUGINS,
  UA_EDIT_PLUGINS,
  UA_COPY_PLUGINS,
  UA_DELETE_PLUGINS,

  /* ---------------- end ----------------- */

  /* --------- ARRANGER SELECTIONS ---------- */

  UA_CREATE_ARRANGER_SELECTIONS,
  UA_MOVE_ARRANGER_SELECTIONS,
  UA_RESIZE_ARRANGER_SELECTIONS,
  UA_SPLIT_ARRANGER_SELECTIONS,
  UA_EDIT_ARRANGER_SELECTIONS,
  UA_DUPLICATE_ARRANGER_SELECTIONS,
  UA_DELETE_ARRANGER_SELECTIONS,
  UA_QUANTIZE_ARRANGER_SELECTIONS,

  /* ---------------- end ----------------- */

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
undoable_action_do (
  UndoableAction * self);

/**
 * Undoes the action.
 *
 * @return Non-zero if errors occurred.
 */
int
undoable_action_undo (
  UndoableAction * self);

void
undoable_action_free (
  UndoableAction * self);

/**
 * Stringizes the action to be used in Undo/Redo
 * buttons.
 *
 * The string MUST be free'd using g_free().
 */
char *
undoable_action_stringize (
  UndoableAction * ua);

/**
 * @}
 */

#endif
