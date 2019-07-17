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

#include "actions/delete_midi_arranger_selections_action.h"
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
      mn = midi_note_find (self->mas->midi_notes[i]);

      /* remove it */
      midi_region_remove_midi_note (
        mn->region,
        mn, F_FREE, F_NO_PUBLISH_EVENTS);
    }

  EVENTS_PUSH (ET_MIDI_NOTE_REMOVED, NULL);
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
delete_midi_arranger_selections_action_undo (
  DeleteMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* clone the clone */
      mn =
        midi_note_clone (
          self->mas->midi_notes[i],
          MIDI_NOTE_CLONE_COPY_MAIN);

      /* add it to the region */
      midi_region_add_midi_note (
        region_find_by_name (mn->region_name), mn);
    }
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
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
