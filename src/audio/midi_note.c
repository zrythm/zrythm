/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * A MIDI note in the timeline.
 */

#include "audio/midi.h"
#include "audio/midi_note.h"
#include "audio/position.h"
#include "audio/track.h"
#include "audio/velocity.h"
#include "gui/backend/midi_arranger_selections.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/clip_editor.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_note.h"
#include "gui/widgets/velocity.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"

void
midi_note_init_loaded (
  MidiNote * self)
{
  self->widget =
    midi_note_widget_new (self);
}

MidiNote *
midi_note_new (MidiRegion * region,
               Position *   start_pos,
               Position *   end_pos,
               int          val,
               Velocity *   vel)
{
  MidiNote * midi_note =
    calloc (1, sizeof (MidiNote));

  position_set_to_pos (&midi_note->start_pos,
                       start_pos);
  position_set_to_pos (&midi_note->end_pos,
                       end_pos);
  midi_note->region = region;
  midi_note->region_name =
    g_strdup (region->name);
  midi_note->val = val;
  midi_note->vel = vel;
  vel->midi_note = midi_note;
  vel->widget = velocity_widget_new (vel);
  midi_note->widget = midi_note_widget_new (midi_note);
  g_object_ref (midi_note->widget);

  return midi_note;
}

/**
 * Finds the actual MidiNote in the project from the
 * given clone.
 */
MidiNote *
midi_note_find (
  MidiNote * clone)
{
  Region * r =
    region_find_by_name (clone->region_name);
  g_warn_if_fail (r);

  for (int i = 0; i < r->num_midi_notes; i++)
    {
      if (midi_note_is_equal (
            r->midi_notes[i],
            clone))
        return r->midi_notes[i];
    }
  g_warn_if_reached ();
  return NULL;
}

/**
 * Deep clones the midi note.
 */
MidiNote *
midi_note_clone (
  MidiNote *  src)
{
  Velocity * vel = velocity_clone (src->vel);

  MidiNote * mn =
    midi_note_new (
      src->region, &src->start_pos, &src->end_pos,
      src->val, vel);

  return mn;
}

/**
 * Returns if MidiNote is in MidiArrangerSelections.
 */
int
midi_note_is_selected (MidiNote * self)
{
  if (array_contains (
        MIDI_ARRANGER_SELECTIONS->midi_notes,
        MIDI_ARRANGER_SELECTIONS->num_midi_notes,
        self) ||
      array_contains (
        MIDI_ARRANGER_SELECTIONS->transient_notes,
        MIDI_ARRANGER_SELECTIONS->num_midi_notes,
        self))
    return 1;

  return 0;
}

/**
 * Returns if MidiNote is (should be) visible.
 */
int
midi_note_is_visible (MidiNote * self)
{
  ARRANGER_WIDGET_GET_PRIVATE (MIDI_ARRANGER);

  if (ar_prv->action ==
        UI_OVERLAY_ACTION_MOVING &&
      array_contains (
        MIDI_ARRANGER_SELECTIONS->midi_notes,
        MIDI_ARRANGER_SELECTIONS->num_midi_notes,
        self))
    return 0;

  return 1;
}

/**
 * Returns 1 if the MidiNotes match, 0 if not.
 */
int
midi_note_is_equal (
  MidiNote * src,
  MidiNote * dest)
{
  return
    !position_compare (&src->start_pos,
                       &dest->start_pos) &&
    !position_compare (&src->end_pos,
                       &dest->end_pos) &&
    src->val == dest->val &&
    src->muted == dest->muted &&
    src->transient == dest->transient &&
    !g_strcmp0 (src->region->name,
                dest->region->name);
}

/**
 * Shifts MidiNote's position and/or value.
 */
void
midi_note_shift (
  MidiNote * self,
  long       ticks, ///< x (Position)
  int        delta) ///< y (0-127)
{
  if (ticks)
    {
      position_add_ticks (
        &self->start_pos,
        ticks);
      position_add_ticks (
        &self->end_pos,
        ticks);
    }
  if (delta)
    self->val += delta;
}

void
midi_note_delete (MidiNote * midi_note)
{
  g_object_unref (midi_note->widget);
  free (midi_note);
}

/**
 * Clamps position then sets it.
 * TODO
 */
void
midi_note_set_start_pos (MidiNote * midi_note,
                      Position * pos)
{
  position_set_to_pos (&midi_note->start_pos,
                       pos);
}

/**
 * Clamps position then sets it.
 */
void
midi_note_set_end_pos (MidiNote * midi_note,
                    Position * pos)
{
  position_set_to_pos (&midi_note->end_pos,
                       pos);
}

/**
 * Converts an array of MIDI notes to MidiEvents.
 */
void
midi_note_notes_to_events (MidiNote     ** midi_notes, ///< array
                           int          num_notes, ///< number of events in array
                           Position     * pos, ///< position to offset time from
                           MidiEvents   * events)  ///< preallocated struct to fill
{
  for (int i = 0; i < num_notes; i++)
    {
      MidiNote * note = midi_notes[i];

      /* note on */
#ifdef HAVE_JACK
      jack_midi_event_t * ev = &events->jack_midi_events[events->num_events];
      ev->time = position_to_frames (&note->start_pos) -
        position_to_frames (pos);
      ev->size = 3;
      if (!ev->buffer)
        ev->buffer = calloc (3, sizeof (jack_midi_data_t));
      ev->buffer[0] = MIDI_CH1_NOTE_ON; /* status byte */
      ev->buffer[1] = note->val; /* note number 0-127 */
      ev->buffer[2] = note->vel->vel; /* velocity 0-127 */
      events->num_events++;

      /* note off */
      ev = &events->jack_midi_events[events->num_events];
      ev->time = position_to_frames (&note->end_pos) -
        position_to_frames (pos);
      ev->size = 3;
      if (!ev->buffer)
        ev->buffer = calloc (3, sizeof (jack_midi_data_t));
      ev->buffer[0] = MIDI_CH1_NOTE_OFF; /* status byte */
      ev->buffer[1] = note->val; /* note number 0-127 */
      ev->buffer[2] = note->vel->vel; /* velocity 0-127 */
      events->num_events++;
#endif
    }
}

/**
 * Returns if the MIDI note is hit at given pos (in the
 * timeline).
 */
int
midi_note_hit (MidiNote * midi_note,
               Position *  pos)
{
  Position local_pos;
  Region * region = midi_note->region;

  /* get local positions */
  region_timeline_pos_to_local (
    region, pos, &local_pos, 1);

  /* add clip_start position to start from
   * there */
  long clip_start_ticks =
    position_to_ticks (
      &region->clip_start_pos);
  /*long region_length_ticks =*/
    /*region_get_full_length_in_ticks (*/
      /*r);*/
  /*position_set_to_pos (*/
    /*&loop_start_adjusted,*/
    /*&r->loop_start_pos);*/
  /*position_set_to_pos (*/
    /*&loop_end_adjusted,*/
    /*&r->loop_end_pos);*/
  /*position_init (&region_end_adjusted);*/
  position_add_ticks (
    &local_pos, clip_start_ticks);
  /*position_add_ticks (*/
    /*&local_end_pos, clip_start_ticks);*/
  /*position_add_ticks (*/
    /*&loop_start_adjusted, - clip_start_ticks);*/
  /*position_add_ticks (*/
    /*&loop_end_adjusted, - clip_start_ticks);*/
  /*position_add_ticks (*/
    /*&region_end_adjusted, region_length_ticks);*/

  /* check for note on event on the
   * boundary */
  if (position_compare (
        &midi_note->start_pos,
        &local_pos) < 0 &&
      position_compare (
        &midi_note->end_pos,
        &local_pos) >= 0)
    return 1;

  return 0;
}

void
midi_note_free (MidiNote * self)
{
  if (self->widget)
    {
      g_message ("destroying %p",
                 self->widget);
      midi_note_widget_destroy (
        self->widget);
    }

  velocity_free (self->vel);

  free (self);
}
