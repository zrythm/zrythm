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

#ifndef __UNDO_EDIT_MA_SELECTIONS_ACTION_H__
#define __UNDO_EDIT_MA_SELECTIONS_ACTION_H__

#include "actions/undoable_action.h"

typedef struct MidiArrangerSelections
  MidiArrangerSelections;

typedef enum EditMidiArrangerSelectionsType
{
  EMAS_TYPE_RESIZE_L,
  EMAS_TYPE_RESIZE_R,
} EditMidiArrangerSelectionsType;

typedef struct EditMidiArrangerSelectionsAction
{
  UndoableAction              parent_instance;

  /** Clone of selections. */
  MidiArrangerSelections * mas;

  /** Type of action. */
  EditMidiArrangerSelectionsType   type;

  /** Ticks when resizing. */
  long                   ticks;

} EditMidiArrangerSelectionsAction;

UndoableAction *
edit_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas,
  EditMidiArrangerSelectionsType type,
  long                     ticks);

int
edit_midi_arranger_selections_action_do (
	EditMidiArrangerSelectionsAction * self);

int
edit_midi_arranger_selections_action_undo (
	EditMidiArrangerSelectionsAction * self);

char *
edit_midi_arranger_selections_action_stringize (
	EditMidiArrangerSelectionsAction * self);

void
edit_midi_arranger_selections_action_free (
	EditMidiArrangerSelectionsAction * self);

#endif
