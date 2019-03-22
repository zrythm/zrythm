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
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"
#include "actions/move_midi_arranger_selections_val.h"



/**
 * Note: this action will add delta to value 
 * 0f all selected midi_notes
 */
UndoableAction *
move_midi_arranger_selections_val_action_new (int delta)
{
	MoveMidiArrangerSelectionsValAction * self =
    calloc (1, sizeof (
      MoveMidiArrangerSelectionsValAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_MIDI_NOTES_VAL;
  self->delta = delta;
  return ua;
}

void
move_midi_arranger_selections_val_action_do (
  MoveMidiArrangerSelectionsValAction * self)
{
		midi_arranger_selections_shift_val(self->delta);	
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
move_midi_arranger_selections_val_action_undo (
  MoveMidiArrangerSelectionsValAction * self)
{
	midi_arranger_selections_shift_val(self->delta * -1);	
    EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
move_midi_arranger_selections_val_action_free (
  MoveMidiArrangerSelectionsValAction * self)
{

  free (self);
}

