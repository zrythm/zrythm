/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Action for changing chords in a ChordObject.
 */

#ifndef __UNDO_EDIT_CHORD_ACTION_H__
#define __UNDO_EDIT_CHORD_ACTION_H__

#include "actions/undoable_action.h"

typedef struct ChordDescriptor ChordDescriptor;
typedef struct ChordObject ChordObject;

/**
 * @addtogroup actions
 *
 * @{
 */

/**
 * An action for changing the chord stored in a
 * ChordObject.
 */
typedef struct EditChordAction
{
  UndoableAction    parent_instance;

  /** ChordObject clone. */
  ChordObject *     chord;

  /** Clone of new ChordDescriptor */
  ChordDescriptor * descr;
} EditChordAction;

UndoableAction *
edit_chord_action_new (
  ChordObject *     chord,
  ChordDescriptor * descr);

int
edit_chord_action_do (
	EditChordAction * self);

int
edit_chord_action_undo (
	EditChordAction * self);

char *
edit_chord_action_stringize (
	EditChordAction * self);

void
edit_chord_action_free (
	EditChordAction * self);

/**
 * @}
 */

#endif
