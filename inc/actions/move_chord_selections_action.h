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

#ifndef __UNDO_MOVE_CHORD_SELECTIONS_ACTION_H__
#define __UNDO_MOVE_CHORD_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct ChordSelections
  ChordSelections;

typedef struct MoveChordSelectionsAction
{
  UndoableAction              parent_instance;

  /** Ticks moved. */
  long        ticks;

  /** Tracks moved. */
  int         delta;

  int         first_call;

  /** Chord selections clone. */
  ChordSelections * ts;
} MoveChordSelectionsAction;

UndoableAction *
move_chord_selections_action_new (
  ChordSelections * ts,
  long                 ticks,
  int                  delta);

int
move_chord_selections_action_do (
	MoveChordSelectionsAction * self);

int
move_chord_selections_action_undo (
	MoveChordSelectionsAction * self);

char *
move_chord_selections_action_stringize (
	MoveChordSelectionsAction * self);

void
move_chord_selections_action_free (
	MoveChordSelectionsAction * self);

#endif
