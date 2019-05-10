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

#include "actions/duplicate_midi_arranger_selections_action.h"
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
  MidiNote * mn;
	for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* clone the clone */
      mn =
        midi_note_clone (
          self->mas->midi_notes[i]);

      /* add and shift it */
      midi_region_add_midi_note (
        mn->region, mn);
      midi_note_shift (
        mn, self->ticks, self->delta);
    }

  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
duplicate_midi_arranger_selections_action_undo (
  DuplicateMidiArrangerSelectionsAction * self)
{
  MidiNote * mn, * orig_mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* clone the clone */
      orig_mn =
        midi_note_clone (
          self->mas->midi_notes[i]);

      /* shift it */
      midi_note_shift (
        orig_mn, self->ticks, self->delta);

      /* find the actual MidiNote at the new pos */
      mn = midi_note_find (orig_mn);
      g_return_val_if_fail (mn, -1);
      midi_note_free (orig_mn);

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

