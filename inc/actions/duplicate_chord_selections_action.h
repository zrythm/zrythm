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

#ifndef __UNDO_DUPLICATE_CHORD_SELECTIONS_ACTION_H__
#define __UNDO_DUPLICATE_CHORD_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct ChordSelections
  ChordSelections;

typedef struct DuplicateChordSelectionsAction
{
  UndoableAction              parent_instance;

  /**
   * A clone of the chord selections at the time.
   */
  ChordSelections *        ts;

  /** Ticks diff. */
  long   ticks;

  /** Value (# of Tracks) diff. */
  int    delta;
} DuplicateChordSelectionsAction;

UndoableAction *
duplicate_chord_selections_action_new (
  ChordSelections * ts,
  long                 ticks,
  int                  delta);

int
duplicate_chord_selections_action_do (
  DuplicateChordSelectionsAction * self);

int
duplicate_chord_selections_action_undo (
  DuplicateChordSelectionsAction * self);

char *
duplicate_chord_selections_action_stringize (
  DuplicateChordSelectionsAction * self);

void
duplicate_chord_selections_action_free (
  DuplicateChordSelectionsAction * self);

#endif
