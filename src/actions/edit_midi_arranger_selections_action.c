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
#include "utils/arrays.h"
#include "utils/flags.h"

#include <glib/gi18n.h>

/**
 * Create the new action.
 */
UndoableAction *
edit_midi_arranger_selections_action_new (
  MidiArrangerSelections * mas,
  EditMidiArrangerSelectionsType type,
  long                     ticks,
  int                      diff,
  Position *               start_pos,
  Position *               end_pos)
{
	EditMidiArrangerSelectionsAction * self =
    calloc (1, sizeof (
    	EditMidiArrangerSelectionsAction));
  UndoableAction * ua = (UndoableAction *) self;
  ua->type =
	  UNDOABLE_ACTION_TYPE_EDIT_MA_SELECTIONS;

  self->type = type;
  self->ticks = ticks;
  self->diff = diff;
  self->first_call = 1;
  /*if (start_pos)*/
    /*position_set_to_pos (*/
      /*&self->start_pos, start_pos);*/
  /*if (end_pos)*/
    /*position_set_to_pos (*/
      /*&self->end_pos, end_pos);*/

  if (type == EMAS_TYPE_VELOCITY_RAMP)
    {
      /* create new midi arranger selections to
       * hold the midi notes that were ramped */
      self->mas =
        calloc (1, sizeof (MidiArrangerSelections));

      /* find enclosed velocities */
      int vel_size = 1;
      Velocity ** velocities =
        (Velocity **)
        malloc (
          vel_size * sizeof (Velocity *));
      int num_vel = 0;
      track_get_velocities_in_range (
        region_get_track (CLIP_EDITOR->region),
        start_pos,
        end_pos,
        &velocities,
        &num_vel,
        &vel_size, 1);

      self->vel_before =
        malloc (num_vel * sizeof (int));
      self->vel_after =
        malloc (num_vel * sizeof (int));

      Velocity * vel;
      MidiNote * mn;
      for (int i = 0; i < num_vel; i++)
        {
          vel = velocities[i];

          /* store the before and after velocities */
          self->vel_before[i] = vel->cache_vel;
          self->vel_after[i] = vel->vel;

          /* add a midi note clone to the mas */
          mn =
            midi_note_clone (
              vel->midi_note, MIDI_NOTE_CLONE_COPY);
          array_append (self->mas->midi_notes,
                        self->mas->num_midi_notes,
                        mn);
        }

      free (velocities);

      /* return NULL if nothing to be done */
      if (self->mas->num_midi_notes == 0)
        {
          edit_midi_arranger_selections_action_free (
            self);
          return NULL;
        }
    }
  else
    {
      self->mas =
        midi_arranger_selections_clone (mas);
    }

  return ua;
}

int
edit_midi_arranger_selections_action_do (
	EditMidiArrangerSelectionsAction * self)
{
  g_message ("performing");
  MidiNote * mn;
  for (int i = 0; i < self->mas->num_midi_notes; i++)
    {
      /* get the actual region */
      mn = midi_note_find (self->mas->midi_notes[i]);
      g_warn_if_fail (mn);

      switch (self->type)
        {
        case EMAS_TYPE_RESIZE_L:
          if (!self->first_call)
            {
              /* resize */
              midi_note_resize (
                mn, 1, self->ticks, AO_UPDATE_ALL);
            }

          /* remember the new start pos for undoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->start_pos,
            &mn->start_pos);
          break;
        case EMAS_TYPE_RESIZE_R:
          if (!self->first_call)
            {
              /* resize */
              midi_note_resize (
                mn, 0, self->ticks, AO_UPDATE_ALL);
            }

          /*g_message ("after resize");*/
          /*position_print_simple (&mn->end_pos);*/

          /* remember the end pos for undoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->end_pos,
            &mn->end_pos);
          break;
        case EMAS_TYPE_VELOCITY_CHANGE:
          if (!self->first_call)
            {
              /* change velocity */
              velocity_set_val (
                mn->vel, mn->vel->vel + self->diff,
                AO_UPDATE_ALL);
            }
          break;
        case EMAS_TYPE_VELOCITY_RAMP:
              /* change velocity */
              velocity_set_val (
                mn->vel, self->vel_after[i],
                AO_UPDATE_ALL);
              /* set the cache too */
              ARRANGER_OBJ_SET_PRIMITIVE_VAL (
                Velocity, mn->vel, cache_vel,
                self->vel_after[i], AO_UPDATE_ALL);
          break;
        default:
          g_warn_if_reached ();
          break;
        }
    }
  EVENTS_PUSH (ET_MA_SELECTIONS_CHANGED,
               NULL);

  self->first_call = 0;

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
            mn, 1, - self->ticks, 1);

          /* remember the new start pos for redoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->start_pos,
            &mn->start_pos);
          break;
        case EMAS_TYPE_RESIZE_R:
          /* resize */
          midi_note_resize (
            mn, 0, - self->ticks, 1);

          /* remember the end start pos for redoing */
          position_set_to_pos (
            &self->mas->midi_notes[i]->end_pos,
            &mn->end_pos);
          break;
        case EMAS_TYPE_VELOCITY_CHANGE:
          /* change velocity */
          velocity_set_val (
            mn->vel, mn->vel->vel - self->diff,
            AO_UPDATE_ALL);
          break;
        case EMAS_TYPE_VELOCITY_RAMP:
          if (!self->first_call)
            {
              /* change velocity */
              velocity_set_val (
                mn->vel, self->vel_before[i],
                AO_UPDATE_ALL);
            }
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
      case EMAS_TYPE_VELOCITY_CHANGE:
      case EMAS_TYPE_VELOCITY_RAMP:
        if (self->mas->num_midi_notes == 1)
          return g_strdup (_("Change Velocity"));
        else if (self->mas->num_midi_notes > 1)
          return g_strdup (_("Change Velocities"));
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
  free (self->vel_before);
  free (self->vel_after);

  free (self);
}

