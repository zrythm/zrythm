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
#include "utils/string.h"

/**
 * @param is_main Is main MidiNote. If this is 1 then
 *   arranger_object_info_init_main() is called to
 *   create a transient midi note in obj_info.
 */
MidiNote *
midi_note_new (
  MidiRegion * region,
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

  if (is_main)
    {
      arranger_object_set_as_main (obj);
    }

  return self;
}

/**
 * Sets the Region the MidiNote belongs to.
 */
void
midi_note_set_region (
  MidiNote * midi_note,
  Region *   region)
{
  MidiNote * mn;
  for (int i = 0; i < 2; i++)
    {
      if (i == AOI_COUNTERPART_MAIN)
        mn = midi_note_get_main (midi_note);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        mn = midi_note_get_main_trans (midi_note);

      mn->region = region;
      if (mn->region_name)
        g_free (mn->region_name);
      mn->region_name = g_strdup (region->name);
    }
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
  arranger_object_set_primitive (
    MidiNote, self, cache_val, val, AO_UPDATE_ALL);
}

/**
 * Sends a note off if currently playing and sets
 * the pitch of the MidiNote.
 */
void
midi_note_set_val (
  MidiNote *    midi_note,
  const uint8_t val,
  ArrangerObjectUpdateFlag update_flag)
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
      midi_events_add_note_off (
        midi_events,
        midi_note->region->lane->midi_ch,
        midi_note->val,
        0, 1);
      zix_sem_post (&midi_events->access_sem);
    }

  arranger_object_set_primitive (
    MidiNote, midi_note, val, val, update_flag);
}

/**
 * Shifts MidiNote's position and/or value.
 *
 * @param delta Y (0-127)
 */
void
midi_note_shift_pitch (
  MidiNote *    self,
  const int     delta,
  ArrangerObjectUpdateFlag update_flag)
{
  self->val = (uint8_t) ((int) self->val + delta);
  midi_note_set_val (
    self, self->val, update_flag);
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
  Region * region = midi_note->region;
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
