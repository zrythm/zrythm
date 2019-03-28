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
#include "project.h"
#include "actions/delete_midi_arranger_selections_action.h"

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
delete_midi_arranger_selections_action_new ()
{
  DeleteMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
                 DeleteMidiArrangerSelectionsAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DELETE_MA_SELECTIONS;

  self->mas = midi_arranger_selections_clone ();

  return ua;
}

void
delete_midi_arranger_selections_action_do (
  DeleteMidiArrangerSelectionsAction * self)
{
  MidiNote * _mn, *mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* this is a clone */
      _mn = self->mas->midi_notes[i];

      /* find actual midi note */
      mn =
        project_get_midi_note (_mn->actual_note);

      /* remove it */
      midi_region_remove_midi_note (
        mn->midi_region, mn);
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
delete_midi_arranger_selections_action_undo (
  DeleteMidiArrangerSelectionsAction * self)
{
  MidiNote * mn_clone, * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* this is a clone */
      mn = self->mas->midi_notes[i];

      /* clone the clone */
      mn_clone =
        midi_note_clone (mn, mn->midi_region);

      /* set actual note to the orig clone */
      mn->actual_note = mn_clone->id;

      /* add the new clone to the region */
      midi_region_add_midi_note (
        mn->midi_region, mn_clone);
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
delete_midi_arranger_selections_action_free (
  DeleteMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}
