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

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
delete_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas)
{
  DeleteMidiArrangerSelectionsAction * self =
    calloc (
      1,
      sizeof (
        DeleteMidiArrangerSelectionsAction));

  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DELETE_MA_SELECTIONS;

  self->mas = midi_arranger_selections_clone (mas);

  return ua;
}

int
delete_midi_arranger_selections_action_do (
  DeleteMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* find actual midi note */
      mn =
        project_get_midi_note (
          self->mas->midi_notes[i]->id);

      /* remove it */
      midi_region_remove_midi_note (
        project_get_region (mn->region_id),
        mn);
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
delete_midi_arranger_selections_action_undo (
  DeleteMidiArrangerSelectionsAction * self)
{
  MidiNote * orig_mn, * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* get the clone */
      orig_mn = self->mas->midi_notes[i];

      /* clone the clone */
      mn =
        midi_note_clone (
          orig_mn,
          project_get_region (orig_mn->region_id));

      /* add to project to get unique ID */
      project_add_midi_note (mn);

      /* add it to the region */
      midi_region_add_midi_note (
        project_get_region (mn->region_id),
        mn);

      /* remember the ID */
      orig_mn->id = mn->id;
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

char *
delete_midi_arranger_selections_action_stringize (
  DeleteMidiArrangerSelectionsAction * self)
{
  return g_strdup (
    _("Delete Object(s)"));
}

void
delete_midi_arranger_selections_action_free (
  DeleteMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}
