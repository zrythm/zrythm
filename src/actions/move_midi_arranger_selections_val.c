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

#include "audio/track.h"
#include "actions/shift_midi_arranger_selections_val.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
shift_midi_arranger_selections_val_action_new (int delta)
{
	ShiftMidiArrangerSelectionsValAction * self =
    calloc (1, sizeof (
    	ShiftMidiArrangerSelectionsValAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_SHIFT_MIDI_NOTES_VAL;
  self->delta = delta;
  return ua;
}

void
shift_midi_arranger_selections_val_action_do (
	ShiftMidiArrangerSelectionsValAction * self)
{
		midi_arranger_selections_shift_val(self->delta);	
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
shift_midi_arranger_selections_val_action_undo (
	ShiftMidiArrangerSelectionsValAction * self)
{
	midi_arranger_selections_shift_val(self->delta * -1);	
    EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
shift_midi_arranger_selections_val_action_free (
	ShiftMidiArrangerSelectionsValAction * self)
{

  free (self);
}

