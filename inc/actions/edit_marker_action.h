/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Action for changing the Marker's name.
 */

#ifndef __UNDO_EDIT_MARKER_ACTION_H__
#define __UNDO_EDIT_MARKER_ACTION_H__

#include "actions/undoable_action.h"

typedef struct Marker Marker;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * An action for changing the chord stored in a
 * MarkerObject.
 */
typedef struct EditMarkerAction
{
  UndoableAction parent_instance;

  /** Marker clone. */
  Marker *       marker;

  /** The name to change to. */
  char *         text;
} EditMarkerAction;

UndoableAction *
edit_marker_action_new (
  Marker *     marker,
  const char * text);

int
edit_marker_action_do (
	EditMarkerAction * self);

int
edit_marker_action_undo (
	EditMarkerAction * self);

char *
edit_marker_action_stringize (
	EditMarkerAction * self);

void
edit_marker_action_free (
	EditMarkerAction * self);

/**
 * @}
 */

#endif
