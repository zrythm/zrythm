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

#include "actions/move_midi_arranger_selections_action.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"
#include "project.h"

/**
 * Note: this action will add delta beats
 * to start and end pos of all selected midi_notes */
UndoableAction *
move_midi_arranger_selections_action_new (
  long ticks,
  int  delta)
{
	MoveMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
    	MoveMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_MIDI_NOTES;
  int i;
  for (i = 0;
       i < MIDI_ARRANGER_SELECTIONS->num_midi_notes;
       i++)
    {
      self->note_ids[i] =
        MIDI_ARRANGER_SELECTIONS->midi_notes[i]->id;
    }
  self->num_notes = i;
  self->delta = delta;
  self->ticks = ticks;
  return ua;
}

void
move_midi_arranger_selections_action_do (
	MoveMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0;
       i < self->num_notes;
       i++)
    {
      mn =
        project_get_midi_note (self->note_ids[i]);
      g_message ("shifting %p",
                 mn->widget);
      midi_note_shift (
        mn,
        self->ticks,
        self->delta);
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
move_midi_arranger_selections_action_undo (
	MoveMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0;
       i < self->num_notes;
       i++)
    {
      mn =
        project_get_midi_note (self->note_ids[i]);
      midi_note_shift (
        mn,
        - self->ticks,
        - self->delta);
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
move_midi_arranger_selections_action_free (
	MoveMidiArrangerSelectionsAction * self)
{
  free (self);
}
