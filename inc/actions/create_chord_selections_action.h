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

#ifndef __UNDO_CREATE_CHORD_SELECTIONS_ACTION_H__
#define __UNDO_CREATE_CHORD_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct ChordSelections
  ChordSelections;

/**
 * It currently "creates" the current selection so
 * that it's undoable.
 *
 * It does nothing on perform due to how the
 * arrangers currently work. This might change in
 * the future.
 */
typedef struct CreateChordSelectionsAction
{
  UndoableAction    parent_instance;

  /**
   * A clone of the chord selections at the time.
   */
  ChordSelections * ts;
} CreateChordSelectionsAction;

/**
 * The given ChordSelections must already
 * contain the created selections in the transient
 * arrays.
 */
UndoableAction *
create_chord_selections_action_new (
  ChordSelections * ts);

int
create_chord_selections_action_do (
  CreateChordSelectionsAction * self);

int
create_chord_selections_action_undo (
  CreateChordSelectionsAction * self);

char *
create_chord_selections_action_stringize (
  CreateChordSelectionsAction * self);

void
create_chord_selections_action_free (
  CreateChordSelectionsAction * self);

#endif
