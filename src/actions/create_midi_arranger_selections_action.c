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

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
create_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas)
{
  CreateMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
                 CreateMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_CREATE_MA_SELECTIONS;

  self->mas =
    midi_arranger_selections_clone (mas);

  return ua;
}

int
create_midi_arranger_selections_action_do (
  CreateMidiArrangerSelectionsAction * self)
{
  MidiNote * mn, * orig_mn;
	for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* get the clone */
      orig_mn = self->mas->midi_notes[i];

      /* if already created (this should be the case
       * the first time) then move on */
      if (project_get_midi_note (orig_mn->id))
        continue;

      /* clone the clone */
      mn =
        midi_note_clone (
          orig_mn,
          project_get_region (orig_mn->region_id));

      /* add it to the project to get unique ID */
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

int
create_midi_arranger_selections_action_undo (
  CreateMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* find the actual MidiNote */
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

char *
create_midi_arranger_selections_action_stringize (
  CreateMidiArrangerSelectionsAction * self)
{
  return g_strdup (
    _("Create Object(s)"));
}

void
create_midi_arranger_selections_action_free (
  CreateMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}
