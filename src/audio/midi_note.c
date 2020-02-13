/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/string.h"

/**
 * @param is_main Is main MidiNote. If this is 1 then
 *   arranger_object_info_init_main() is called to
 *   create a transient midi note in obj_info.
 */
MidiNote *
midi_note_new (
  ZRegion * region,
  Position *   start_pos,
  Position *   end_pos,
  uint8_t      val,
  uint8_t      vel,
  int          is_main)
{
  g_return_val_if_fail (region, NULL);

  MidiNote * self =
    calloc (1, sizeof (MidiNote));

  ArrangerObject * obj =
    (ArrangerObject *) self;
  obj->pos = *start_pos;
  obj->end_pos = *end_pos;
  obj->type = ARRANGER_OBJECT_TYPE_MIDI_NOTE;
  obj->has_length = 1;

  self->region = region;
  self->region_name =
    g_strdup (region->name);
  self->val = val;
  self->vel =
    velocity_new (self, vel, is_main);

  return self;
}

/**
 * Sets the ZRegion the MidiNote belongs to.
 */
void
midi_note_set_region (
  MidiNote * self,
  ZRegion *   region)
{
  self->region = region;
  if (self->region_name)
    g_free (self->region_name);
  self->region_name = g_strdup (region->name);
}

/**
 * For debugging.
 */
void
midi_note_print (
  MidiNote * mn)
{
  ArrangerObject * obj =
    (ArrangerObject *) mn;
  g_message (
    "MidiNote: start pos %d.%d.%d.%d "
    "end pos %d.%d.%d.%d",
    obj->pos.bars,
    obj->pos.beats,
    obj->pos.sixteenths,
    obj->pos.ticks,
    obj->end_pos.bars,
    obj->end_pos.beats,
    obj->end_pos.sixteenths,
    obj->end_pos.ticks);
}

/**
 * Listen to the given MidiNote.
 *
 * @param listen Turn note on if 1, or turn it
 *   off if 0.
 */
void
midi_note_listen (
  MidiNote * mn,
  int        listen)
{
  ArrangerObject * obj =
    (ArrangerObject *) mn;

  Track * track =
    arranger_object_get_track (obj);
  g_return_if_fail (
    track && track->processor.midi_in);
  MidiEvents * events =
    track->processor.midi_in->midi_events;

  if (listen)
    {
      /* if note is on but pitch changed */
      if (mn->currently_listened &&
          mn->val != mn->last_listened_val)
        {
          /* create midi note off */
          midi_events_add_note_off (
            events, 1, mn->last_listened_val,
            0, 1);

          /* create note on at the new value */
          midi_events_add_note_on (
            events, 1, mn->val, mn->vel->vel,
            1, 1);
          mn->last_listened_val = mn->val;
        }
      /* if note is on and pitch is the same */
      else if (mn->currently_listened &&
               mn->val == mn->last_listened_val)
        {
          /* do nothing */
        }
      /* if note is not on */
      else if (!mn->currently_listened)
        {
          /* turn it on */
          midi_events_add_note_on (
            events, 1, mn->val, mn->vel->vel,
            1, 1);
          mn->last_listened_val = mn->val;
          mn->currently_listened = 1;
        }
    }
  /* if turning listening off */
  else if (mn->currently_listened)
    {
      /* create midi note off */
      midi_events_add_note_off (
        events, 1, mn->last_listened_val,
        0, 1);
      mn->currently_listened = 0;
      mn->last_listened_val = 255;
    }
}

/**
 * Returns 1 if the MidiNote's match, 0 if not.
 */
int
midi_note_is_equal (
  MidiNote * src,
  MidiNote * dest)
{
  ArrangerObject * src_obj =
    (ArrangerObject *) src;
  ArrangerObject * dest_obj =
    (ArrangerObject *) dest;
  return
    position_is_equal (
      &src_obj->pos, &dest_obj->pos) &&
    position_is_equal (
      &src_obj->end_pos, &dest_obj->end_pos) &&
    src->val == dest->val &&
    src->muted == dest->muted &&
    string_is_equal (
      src->region->name, dest->region->name, 0);
}

/**
 * Gets the global Position of the MidiNote's
 * start_pos.
 *
 * @param pos Position to fill in.
 */
void
midi_note_get_global_start_pos (
  MidiNote * self,
  Position * pos)
{
  ArrangerObject * self_obj =
    (ArrangerObject *) self;
  ArrangerObject * region_obj =
    (ArrangerObject *) self->region;
  position_set_to_pos (
    pos, &self_obj->pos);
  position_add_ticks (
    pos,
    position_to_ticks (
      &region_obj->pos));
}

/**
 * Gets the MIDI note's value as a string (eg
 * "C#4").
 *
 * @param use_markup Use markup to show the octave
 *   as a superscript.
 */
void
midi_note_get_val_as_string (
  MidiNote * self,
  char *     buf,
  const int  use_markup)
{
  const char * note_str =
    chord_descriptor_note_to_string (
      self->val % 12);
  const int note_val = self->val / 12 - 1;
  if (use_markup)
    {
      sprintf (
        buf, "%s<sup>%d</sup>",
        note_str, note_val);
    }
  else
    {
      sprintf (
        buf, "%s%d",
        note_str, note_val);
    }
}

void
midi_note_set_cache_val (
  MidiNote *    self,
  const uint8_t val)
{
  self->cache_val = val;
}

/**
 * Sends a note off if currently playing and sets
 * the pitch of the MidiNote.
 */
void
midi_note_set_val (
  MidiNote *    midi_note,
  const uint8_t val)
{
  g_return_if_fail (val < 128);

  /* if currently playing set a note off event. */
  if (midi_note_hit (
        midi_note, PLAYHEAD->frames) &&
      TRANSPORT_IS_ROLLING)
    {
      ArrangerObject * r_obj =
        (ArrangerObject *) midi_note->region;
      g_return_if_fail (r_obj);
      Track * track =
        arranger_object_get_track (r_obj);
      g_return_if_fail (track);

      MidiEvents * midi_events =
        track->processor.piano_roll->midi_events;

      zix_sem_wait (&midi_events->access_sem);
      TrackLane * lane =
        region_get_lane (midi_note->region);
      midi_events_add_note_off (
        midi_events, lane->midi_ch,
        midi_note->val, 0, 1);
      zix_sem_post (&midi_events->access_sem);
    }

  midi_note->val = val;
}

/**
 * Shifts MidiNote's position and/or value.
 *
 * @param delta Y (0-127)
 */
void
midi_note_shift_pitch (
  MidiNote *    self,
  const int     delta)
{
  self->val = (uint8_t) ((int) self->val + delta);
  midi_note_set_val (
    self, self->val);
}

/**
 * Returns if the MIDI note is hit at given pos (in
 * the timeline).
 */
int
midi_note_hit (
  MidiNote * midi_note,
  const long       gframes)
{
  ZRegion * region = midi_note->region;
  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  /* get local positions */
  long local_pos =
    region_timeline_frames_to_local (
      region, gframes, 1);

  /* add clip_start position to start from
   * there */
  long clip_start_frames =
    region_obj->clip_start_pos.frames;
  local_pos += clip_start_frames;

  /* check for note on event on the
   * boundary */
  /* FIXME ok? it was < and >= before */
  ArrangerObject * midi_note_obj =
    (ArrangerObject *) midi_note;
  if (midi_note_obj->pos.frames <= local_pos &&
      midi_note_obj->end_pos.frames > local_pos)
    return 1;

  return 0;
}
