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

#define SET_POS(r,pos_name,pos,update_flag) \
  ARRANGER_OBJ_SET_POS ( \
    midi_note, r, pos_name, pos, update_flag)

ARRANGER_OBJ_DEFINE_MOVABLE_W_LENGTH (
  MidiNote, midi_note, midi_arranger_selections,
  MA_SELECTIONS);
ARRANGER_OBJ_DEFINE_RESIZE_NO_LOOP (
  MidiNote, midi_note);

void
midi_note_init_loaded (
  MidiNote * self)
{
  ARRANGER_OBJECT_SET_AS_MAIN (
    MIDI_NOTE, MidiNote, midi_note);

  midi_note_set_region (self, self->region);

  self->vel->midi_note = self;
  velocity_init_loaded (self->vel);
}

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
  MidiNote * self =
    calloc (1, sizeof (MidiNote));

  position_set_to_pos (&self->start_pos,
                       start_pos);
  position_set_to_pos (&self->end_pos,
                       end_pos);
  self->region = region;
  self->region_name =
    g_strdup (region->name);
  self->val = val;
  self->vel =
    velocity_new (self, vel, is_main);

  if (is_main)
    {
      ARRANGER_OBJECT_SET_AS_MAIN (
        MIDI_NOTE, MidiNote, midi_note);
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
        mn = midi_note_get_main_midi_note (midi_note);
      else if (i == AOI_COUNTERPART_MAIN_TRANSIENT)
        mn = midi_note_get_main_trans_midi_note (midi_note);

      mn->region = region;
      mn->region_name = g_strdup (region->name);
    }
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
  return NULL;
}

/**
 * Deep clones the midi note.
 */
MidiNote *
midi_note_clone (
  MidiNote *  src,
  MidiNoteCloneFlag flag)
{
  int is_main = 0;
  if (flag == MIDI_NOTE_CLONE_COPY_MAIN)
    is_main = 1;

  MidiNote * mn =
    midi_note_new (
      src->region, &src->start_pos, &src->end_pos,
      src->val, src->vel->vel, is_main);

  return mn;
}

/**
 * Gets the Track this MidiNote is in.
 */
Track *
midi_note_get_track (
  MidiNote * self)
{
  return TRACKLIST->tracks[self->region->track_pos];
}

/**
 * For debugging.
 */
void
midi_note_print (
  MidiNote * mn)
{
  g_message (
    "MidiNote: start pos %d.%d.%d.%d "
    "end pos %d.%d.%d.%d",
    mn->start_pos.bars,
    mn->start_pos.beats,
    mn->start_pos.sixteenths,
    mn->start_pos.ticks,
    mn->end_pos.bars,
    mn->end_pos.beats,
    mn->end_pos.sixteenths,
    mn->end_pos.ticks);
}

/**
 * Returns 1 if the MidiNote's match, 0 if not.
 */
int
midi_note_is_equal (
  MidiNote * src,
  MidiNote * dest)
{
  return
    position_is_equal (
      &src->start_pos, &dest->start_pos) &&
    position_is_equal (
      &src->end_pos, &dest->end_pos) &&
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
  position_set_to_pos (
    pos, &self->start_pos);
  position_add_ticks (
    pos,
    position_to_ticks (
      &self->region->start_pos));
}

void
midi_note_set_cache_val (
  MidiNote *    self,
  const uint8_t val)
{
  /* see ARRANGER_OBJ_SET_POS */
  midi_note_get_main_midi_note (self)->
    cache_val = val;
  midi_note_get_main_trans_midi_note (self)->
    cache_val = val;
}

ARRANGER_OBJ_DECLARE_RESET_COUNTERPART (
  MidiNote, midi_note)
{
  MidiNote * src =
    reset_trans ?
      midi_note_get_main_midi_note (midi_note) :
      midi_note_get_main_trans_midi_note (midi_note);
  MidiNote * dest =
    reset_trans ?
      midi_note_get_main_trans_midi_note (midi_note) :
      midi_note_get_main_midi_note (midi_note);

  position_set_to_pos (
    &dest->start_pos, &src->start_pos);
  position_set_to_pos (
    &dest->end_pos, &src->end_pos);
  dest->vel->vel = src->vel->vel;
  dest->val = src->val;
}

/**
 * Sets the pitch of the MidiNote.
 */
void
midi_note_set_val (
  MidiNote *    midi_note,
  const uint8_t val,
  ArrangerObjectUpdateFlag update_flag)
{
  /* if currently playing set a note off event. */
  if (midi_note_hit (
        midi_note, PLAYHEAD->frames) &&
      IS_TRANSPORT_ROLLING)
    {
      MidiEvents * midi_events =
        region_get_track (midi_note->region)->
          channel->piano_roll->midi_events;

      zix_sem_wait (&midi_events->access_sem);
      midi_events_add_note_off (
        midi_events,
        midi_note->region->lane->midi_ch,
        midi_note->val,
        0, 1);
      zix_sem_post (&midi_events->access_sem);
    }

  ARRANGER_OBJ_SET_PRIMITIVE_VAL (
    MidiNote, midi_note, val, val, update_flag);
}

ARRANGER_OBJ_DEFINE_GEN_WIDGET_LANELESS (
  MidiNote, midi_note);

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

void
midi_note_delete (MidiNote * midi_note)
{
  if (midi_note->widget)
    g_object_unref (midi_note->widget);
  free (midi_note);
}

/**
 * Updates the frames of each position in each child
 * of the MidiNote recursively.
 */
void
midi_note_update_frames (
  MidiNote * self)
{
  position_update_frames (
    &self->start_pos);
  position_update_frames (
    &self->end_pos);
  position_update_frames (
    &self->cache_start_pos);
  position_update_frames (
    &self->cache_end_pos);
}

/**
 * Returns the MidiNote length in ticks.
 */
long
midi_note_get_length_in_ticks (
  const MidiNote * self)
{
  return
    position_to_ticks (&self->end_pos) -
    position_to_ticks (&self->start_pos);
}

/**
 * Returns if the MIDI note is hit at given pos (in
 * the timeline).
 */
int
midi_note_hit (
  const MidiNote * midi_note,
  const long       gframes)
{
  Region * region = midi_note->region;

  /* get local positions */
  long local_pos =
    region_timeline_frames_to_local (
      region, gframes, 1);

  /* add clip_start position to start from
   * there */
  long clip_start_frames =
    region->clip_start_pos.frames;
  local_pos += clip_start_frames;

  /* check for note on event on the
   * boundary */
  /* FIXME ok? it was < and >= before */
  if (midi_note->start_pos.frames <= local_pos &&
      midi_note->end_pos.frames > local_pos)
    return 1;

  return 0;
}

ARRANGER_OBJ_DEFINE_FREE_ALL_LANELESS (
  MidiNote, midi_note);

void
midi_note_free (MidiNote * self)
{
  if (self->widget)
    {
      midi_note_widget_destroy (
        self->widget);
    }

  velocity_free_all (self->vel);

  if (self->region_name)
    g_free (self->region_name);

  free (self);
}
