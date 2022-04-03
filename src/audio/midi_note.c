/*
 * Copyright (C) 2018-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <math.h>

#include "audio/midi_event.h"
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
#include "utils/objects.h"
#include "utils/string.h"

/**
 * Creates a new MidiNote.
 */
MidiNote *
midi_note_new (
  RegionIdentifier * region_id,
  Position *         start_pos,
  Position *         end_pos,
  uint8_t            val,
  uint8_t            vel)
{
  g_return_val_if_fail (region_id, NULL);

  MidiNote * self = object_new (MidiNote);

  self->schema_version = MIDI_NOTE_SCHEMA_VERSION;
  self->magic = MIDI_NOTE_MAGIC;

  ArrangerObject * obj = (ArrangerObject *) self;
  arranger_object_init (obj);
  obj->pos = *start_pos;
  obj->end_pos = *end_pos;
  obj->type = ARRANGER_OBJECT_TYPE_MIDI_NOTE;

  region_identifier_copy (
    &obj->region_id, region_id);
  self->val = val;
  self->vel = velocity_new (self, vel);

  return self;
}

/**
 * Sets the region the MidiNote belongs to.
 */
void
midi_note_set_region_and_index (
  MidiNote * self,
  ZRegion *  region,
  int        idx)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  region_identifier_copy (
    &obj->region_id, &region->id);
  self->pos = idx;
}

/**
 * For debugging.
 */
void
midi_note_print (MidiNote * mn)
{
  ArrangerObject * obj = (ArrangerObject *) mn;
  char             start_pos_str[300];
  position_to_string (&obj->pos, start_pos_str);
  char end_pos_str[300];
  position_to_string (&obj->end_pos, end_pos_str);
  g_message (
    "MidiNote: start pos %s - end pos %s",
    start_pos_str, end_pos_str);
}

/**
 * Listen to the given MidiNote.
 *
 * @param listen Turn note on if 1, or turn it
 *   off if 0.
 */
void
midi_note_listen (MidiNote * mn, bool listen)
{
  /*g_message (*/
  /*"%s: %" PRIu8 " listen %d", __func__,*/
  /*mn->val, listen);*/

  ArrangerObject * obj = (ArrangerObject *) mn;

  Track * track = arranger_object_get_track (obj);
  g_return_if_fail (
    track && track->processor->midi_in);
  MidiEvents * events =
    track->processor->midi_in->midi_events;

  if (listen)
    {
      /* if note is on but pitch changed */
      if (
        mn->currently_listened
        && mn->val != mn->last_listened_val)
        {
          /* create midi note off */
          /*g_message (*/
          /*"%s: adding note off for %" PRIu8,*/
          /*__func__, mn->last_listened_val);*/
          midi_events_add_note_off (
            events, 1, mn->last_listened_val, 0, 1);

          /* create note on at the new value */
          /*g_message (*/
          /*"%s: adding note on for %" PRIu8,*/
          /*__func__, mn->val);*/
          midi_events_add_note_on (
            events, 1, mn->val, mn->vel->vel, 0, 1);
          mn->last_listened_val = mn->val;
        }
      /* if note is on and pitch is the same */
      else if (
        mn->currently_listened
        && mn->val == mn->last_listened_val)
        {
          /* do nothing */
        }
      /* if note is not on */
      else if (!mn->currently_listened)
        {
          /* turn it on */
          /*g_message (*/
          /*"%s: adding note on for %" PRIu8,*/
          /*__func__, mn->val);*/
          midi_events_add_note_on (
            events, 1, mn->val, mn->vel->vel, 0, 1);
          mn->last_listened_val = mn->val;
          mn->currently_listened = 1;
        }
    }
  /* if turning listening off */
  else if (mn->currently_listened)
    {
      /* create midi note off */
      /*g_message (*/
      /*"%s: adding note off for %" PRIu8,*/
      /*__func__, mn->last_listened_val);*/
      midi_events_add_note_off (
        events, 1, mn->last_listened_val, 0, 1);
      mn->currently_listened = 0;
      mn->last_listened_val = 255;
    }
}

/**
 * Returns 1 if the MidiNote's match, 0 if not.
 */
int
midi_note_is_equal (MidiNote * src, MidiNote * dest)
{
  ArrangerObject * src_obj = (ArrangerObject *) src;
  ArrangerObject * dest_obj =
    (ArrangerObject *) dest;
  return position_is_equal_ticks (
           &src_obj->pos, &dest_obj->pos)
         && position_is_equal_ticks (
           &src_obj->end_pos, &dest_obj->end_pos)
         && src->val == dest->val
         && src->muted == dest->muted
         && region_identifier_is_equal (
           &src_obj->region_id, &dest_obj->region_id);
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
  ArrangerObject * region_obj = (ArrangerObject *)
    arranger_object_get_region (self_obj);
  position_set_to_pos (pos, &self_obj->pos);
  position_add_ticks (
    pos, position_to_ticks (&region_obj->pos));
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
  const MidiNote * self,
  char *           buf,
  const int        use_markup)
{
  const char * note_str =
    chord_descriptor_note_to_string (self->val % 12);
  const int note_val = self->val / 12 - 1;
  if (use_markup)
    {
      sprintf (
        buf, "%s<sup>%d</sup>", note_str, note_val);
      if (DEBUGGING)
        {
          char tmp[50];
          sprintf (tmp, "%s (%d)", buf, self->pos);
          strcpy (buf, tmp);
        }
    }
  else
    {
      sprintf (buf, "%s%d", note_str, note_val);
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
  if (
    midi_note_hit (midi_note, PLAYHEAD->frames)
    && TRANSPORT_IS_ROLLING)
    {
      ZRegion * region = arranger_object_get_region (
        (ArrangerObject *) midi_note);
      ArrangerObject * r_obj =
        (ArrangerObject *) region;
      g_return_if_fail (r_obj);
      Track * track =
        arranger_object_get_track (r_obj);
      g_return_if_fail (track);

      MidiEvents * midi_events =
        track->processor->piano_roll->midi_events;

      zix_sem_wait (&midi_events->access_sem);
      uint8_t midi_ch =
        midi_region_get_midi_ch (region);
      midi_events_add_note_off (
        midi_events, midi_ch, midi_note->val, 0,
        F_QUEUED);
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
  MidiNote * self,
  const int  delta)
{
  self->val = (uint8_t) ((int) self->val + delta);
  midi_note_set_val (self, self->val);
}

ZRegion *
midi_note_get_region (MidiNote * self)
{
  ArrangerObject * obj = (ArrangerObject *) self;
  return region_find (&obj->region_id);
}

/**
 * Returns if the MIDI note is hit at given pos (in
 * the timeline).
 */
int
midi_note_hit (
  MidiNote *           self,
  const signed_frame_t gframes)
{
  ZRegion * region = arranger_object_get_region (
    (ArrangerObject *) self);
  ArrangerObject * region_obj =
    (ArrangerObject *) region;

  /* get local positions */
  signed_frame_t local_pos =
    region_timeline_frames_to_local (
      region, gframes, 1);

  /* add clip_start position to start from
   * there */
  signed_frame_t clip_start_frames =
    region_obj->clip_start_pos.frames;
  local_pos += clip_start_frames;

  /* check for note on event on the
   * boundary */
  /* FIXME ok? it was < and >= before */
  ArrangerObject * midi_note_obj =
    (ArrangerObject *) self;
  if (
    midi_note_obj->pos.frames <= local_pos
    && midi_note_obj->end_pos.frames > local_pos)
    return 1;

  return 0;
}
