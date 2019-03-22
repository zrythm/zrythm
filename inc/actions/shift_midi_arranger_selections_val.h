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

#ifndef __UNDO_SHIFT_MIDI_ARRANGER_SELECTIONS_VAL_ACTION_H__
#define __UNDO_SHIFT_MIDI_ARRANGER_SELECTIONS_VAL_ACTION_H__

#include "actions/undoable_action.h"

typedef struct MidiArrangerSelections
  MidiArrangerSelections;

typedef struct ShiftMidiArrangerSelectionsValAction
{
  UndoableAction              parent_instance;

  /**
   * A clone of the midi_arranger selections at the time.
   */
  int         delta;
} ShiftMidiArrangerSelectionsValAction;

UndoableAction *
shift_midi_arranger_selections_val_action_new (int delta);

void
shift_midi_arranger_selections_val_action_do (
	ShiftMidiArrangerSelectionsValAction * self);

void
shift_midi_arranger_selections_val_action_undo (
	ShiftMidiArrangerSelectionsValAction * self);

void
shift_midi_arranger_selections_val_action_free (
	ShiftMidiArrangerSelectionsValAction * self);

#endif
