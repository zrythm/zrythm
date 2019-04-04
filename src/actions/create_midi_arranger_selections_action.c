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
#include "actions/create_midi_arranger_selections_action.h"

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
create_midi_arranger_selections_action_new ()
{
  CreateMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
                 CreateMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_CREATE_MA_SELECTIONS;
  self->mas = midi_arranger_selections_clone ();
  return ua;
}

void
create_midi_arranger_selections_action_do (
  CreateMidiArrangerSelectionsAction * self)
{
  MidiNote * mn, * _mn;
	for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* this is a clone, must not be used */
      mn = self->mas->midi_notes[i];

      if (project_get_midi_note (mn->actual_id))
        continue;

      /* clone the clone */
      _mn =
        midi_note_clone (
          mn,
          project_get_region (mn->region_id));

      /* move it to the original id */
      project_move_midi_note (_mn, mn->actual_id);

      /* add the new clone */
      midi_region_add_midi_note (
        project_get_region (_mn->region_id),
        _mn);
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
create_midi_arranger_selections_action_undo (
  CreateMidiArrangerSelectionsAction * self)
{
  MidiNote * mn, * _mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* this is a clone, must not be used */
      mn = self->mas->midi_notes[i];

      /* find the MidiNote with the actual id */
      _mn = project_get_midi_note (mn->actual_id);

      /* remove it */
      midi_region_remove_midi_note (
        project_get_region (_mn->region_id),
        _mn);
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);
}

void
create_midi_arranger_selections_action_free (
  CreateMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}
