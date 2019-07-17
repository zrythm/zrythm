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

#include "actions/create_midi_arranger_selections_action.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"
#include "project.h"
#include "utils/flags.h"

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
  MidiNote * mn;
	for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      mn = midi_note_find (self->mas->midi_notes[i]);

      /* if already created (this should be the case
       * the first time) then move on, after setting
       * the transient end pos to the main */
      if (mn)
        {
          mn =
            midi_note_get_main_trans_midi_note (
              mn);

          continue;
        }

      /* clone the clone */
      mn =
        midi_note_clone (
          self->mas->midi_notes[i],
          MIDI_NOTE_CLONE_COPY_MAIN);

      /* find the region */
      mn->region =
        region_find_by_name (mn->region_name);
      g_return_val_if_fail (mn->region, -1);

      /* add it to the region */
      midi_region_add_midi_note (
        mn->region, mn);
    }
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
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
      mn = midi_note_find (self->mas->midi_notes[i]);

      /* remove it */
      midi_region_remove_midi_note (
        mn->region, mn, F_FREE, F_NO_PUBLISH_EVENTS);
    }

  EVENTS_PUSH (ET_MIDI_NOTE_REMOVED, NULL);
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
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
