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
#include "actions/duplicate_midi_arranger_selections_action.h"

#include <glib/gi18n.h>

/**
 * Note: chord addresses are to be copied.
 */
UndoableAction *
duplicate_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas,
  long                     ticks,
  int                      delta)
{
  DuplicateMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
                 DuplicateMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
    UNDOABLE_ACTION_TYPE_DUPLICATE_MA_SELECTIONS;

  self->mas = midi_arranger_selections_clone (mas);
  self->ticks = ticks;
  self->delta = delta;

  return ua;
}

int
duplicate_midi_arranger_selections_action_do (
  DuplicateMidiArrangerSelectionsAction * self)
{
  MidiNote * mn, * orig_mn;
	for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* get the clone */
      orig_mn = self->mas->midi_notes[i];

      /* clone the clone */
      mn =
        midi_note_clone (
          orig_mn,
          project_get_region (orig_mn->region_id));

      /* add to project to get a unique ID */
      project_add_midi_note (mn);

      /* add and shift it */
      midi_region_add_midi_note (
        project_get_region (mn->region_id),
        mn);
      midi_note_shift (
        mn, self->ticks, self->delta);

      /* remember ID */
      orig_mn->id = mn->id;
    }
  EVENTS_PUSH (ET_MIDI_ARRANGER_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
duplicate_midi_arranger_selections_action_undo (
  DuplicateMidiArrangerSelectionsAction * self)
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
duplicate_midi_arranger_selections_action_stringize (
  DuplicateMidiArrangerSelectionsAction * self)
{
  return g_strdup (
    _("Duplicate Object(s)"));
}

void
duplicate_midi_arranger_selections_action_free (
  DuplicateMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}

