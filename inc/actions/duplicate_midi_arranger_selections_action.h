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

#ifndef __UNDO_DUPLICATE_MA_SELECTIONS_ACTION_H__
#define __UNDO_DUPLICATE_MA_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct MidiArrangerSelections
  MidiArrangerSelections;

typedef struct DuplicateMidiArrangerSelectionsAction
{
  UndoableAction              parent_instance;

  /**
   * A clone of the midi_arranger selections at the time.
   */
  MidiArrangerSelections *        mas;

  /** Ticks diff. */
  long   ticks;

  /** Value diff. */
  int    delta;
} DuplicateMidiArrangerSelectionsAction;

UndoableAction *
duplicate_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas,
  long                     ticks,
  int                      delta);

int
duplicate_midi_arranger_selections_action_do (
  DuplicateMidiArrangerSelectionsAction * self);

int
duplicate_midi_arranger_selections_action_undo (
  DuplicateMidiArrangerSelectionsAction * self);

char *
duplicate_midi_arranger_selections_action_stringize (
  DuplicateMidiArrangerSelectionsAction * self);

void
duplicate_midi_arranger_selections_action_free (
  DuplicateMidiArrangerSelectionsAction * self);

#endif
