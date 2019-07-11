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

#include "actions/move_midi_arranger_selections_action.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

/**
 * Note: this action will add delta beats
 * to start and end pos of all selected midi_notes */
UndoableAction *
move_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas,
  long                     ticks,
  int                      delta)
{
	MoveMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
    	MoveMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_MOVE_MA_SELECTIONS;

  self->mas = midi_arranger_selections_clone (mas);
  self->delta = delta;
  self->ticks = ticks;
  self->first_call = 1;

  return ua;
}

int
move_midi_arranger_selections_action_do (
	MoveMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      if (!self->first_call)
        {
          /* find actual midi note */
          mn = midi_note_find (
            self->mas->midi_notes[i]);
          g_return_val_if_fail (mn, -1);

          /* shift it */
          midi_note_shift_by_ticks (
            mn, self->ticks, AO_UPDATE_ALL);
          midi_note_shift_pitch (
            mn, self->delta, AO_UPDATE_ALL);

          /* shift the clone too so they can match */
          mn = self->mas->midi_notes[i];
          midi_note_shift_by_ticks (
            mn, self->ticks, AO_UPDATE_THIS);
          midi_note_shift_pitch (
            mn, self->delta, AO_UPDATE_THIS);
        }
    }
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);

  self->first_call = 0;

  return 0;
}

int
move_midi_arranger_selections_action_undo (
	MoveMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* find actual midi note */
      mn = midi_note_find (self->mas->midi_notes[i]);
      g_return_val_if_fail (mn, -1);

      /* shift it */
      midi_note_shift_by_ticks (
        mn, - self->ticks, AO_UPDATE_ALL);
      midi_note_shift_pitch (
        mn, - self->delta, AO_UPDATE_ALL);

      /* shift the clone too so they can match */
      midi_note_shift_by_ticks (
        self->mas->midi_notes[i],
        - self->ticks, AO_UPDATE_THIS);
      midi_note_shift_pitch (
        self->mas->midi_notes[i],
        - self->delta, AO_UPDATE_THIS);
    }
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
move_midi_arranger_selections_action_stringize (
	MoveMidiArrangerSelectionsAction * self)
{
  return g_strdup (
    _("Move Object(s)"));
}

void
move_midi_arranger_selections_action_free (
	MoveMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}
