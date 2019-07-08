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

#include "actions/edit_midi_arranger_selections_action.h"
#include "audio/track.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/midi_arranger.h"
#include "project.h"

#include <glib/gi18n.h>

UndoableAction *
edit_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas,
  EditMidiArrangerSelectionsType type,
  long                     ticks)
{
	EditMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
    	EditMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_MA_SELECTIONS;

  self->mas = midi_arranger_selections_clone (mas);
  self->type = type;
  self->ticks = ticks;

  return ua;
}

int
edit_midi_arranger_selections_action_do (
	EditMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* get the actual region */
      mn = midi_note_find (self->mas->midi_notes[i]);
      g_warn_if_fail (mn);

      switch (self->type)
        {
        case EMAS_TYPE_RESIZE_L:
          /* resize */
          midi_note_resize (
            mn, 1, self->ticks);

          /* remember the new start pos for undoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->start_pos,
            &mn->start_pos);
          break;
        case EMAS_TYPE_RESIZE_R:
          /* resize */
          midi_note_resize (
            mn, 0, self->ticks);

          /* remember the end pos for undoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->end_pos,
            &mn->end_pos);
          break;
        default:
          g_warn_if_reached ();
          break;
        }
    }
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);

  return 0;
}

int
edit_midi_arranger_selections_action_undo (
	EditMidiArrangerSelectionsAction * self)
{
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* get the actual region */
      mn = midi_note_find (self->mas->midi_notes[i]);
      g_warn_if_fail (mn);

      switch (self->type)
        {
        case EMAS_TYPE_RESIZE_L:
          /* resize */
          midi_note_resize (
            mn, 1, - self->ticks);

          /* remember the new start pos for redoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->start_pos,
            &mn->start_pos);
          break;
        case EMAS_TYPE_RESIZE_R:
          /* resize */
          midi_note_resize (
            mn, 0, - self->ticks);

          /* remember the end start pos for redoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->end_pos,
            &mn->end_pos);
          break;
        default:
          g_warn_if_reached ();
          break;
        }
    }
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);
  return 0;
}

char *
edit_midi_arranger_selections_action_stringize (
	EditMidiArrangerSelectionsAction * self)
{
  switch (self->type)
    {
      case EMAS_TYPE_RESIZE_L:
      case EMAS_TYPE_RESIZE_R:
        return g_strdup (_("Resize MIDI Note(s)"));
      default:
        g_return_val_if_reached (
          g_strdup (""));
    }
}

void
edit_midi_arranger_selections_action_free (
	EditMidiArrangerSelectionsAction * self)
{
  midi_arranger_selections_free (self->mas);

  free (self);
}

