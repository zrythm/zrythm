/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

#include "config.h"

#include "audio/engine.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/midi.h"
#include "audio/transport.h"
#include "project.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/arranger_playhead.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/midi_arranger.h"
#include "gui/widgets/midi_modifier_arranger.h"
#include "gui/widgets/editor_ruler.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_ruler.h"
#include "gui/widgets/top_bar.h"

#include <gtk/gtk.h>

DEFINE_START_POS;

/**
 * Sets BPM and does any necessary processing (like
 * notifying interested parties).
 */
void
transport_set_bpm (
  Transport * self,
  bpm_t       bpm)
{
  if (bpm < MIN_BPM)
    bpm = MIN_BPM;
  else if (bpm > MAX_BPM)
    bpm = MAX_BPM;
  self->bpm = bpm;
  engine_update_frames_per_tick (
    AUDIO_ENGINE,
    self->beats_per_bar,
    bpm,
    AUDIO_ENGINE->sample_rate);
}

/**
 * Initialize transport
 */
void
transport_init (
  Transport * self,
  int         loading)
{
  g_message ("Initializing transport");

  if (loading)
    {
      transport_set_beat_unit (
        self, self->beat_unit);
    }
  else
    {
      // set inital total number of beats
      // this is applied to the ruler
      self->total_bars = DEFAULT_TOTAL_BARS;

      /* set BPM related defaults */
      self->beats_per_bar = DEFAULT_BEATS_PER_BAR;
      transport_set_beat_unit (self, 4);

      // set positions of playhead, start/end markers
      position_set_to_bar (&self->playhead_pos, 1);
      position_set_to_bar (&self->cue_pos, 1);
      /*position_set_to_bar (*/
        /*&self->start_marker_pos, 1);*/
      /*position_set_to_bar (*/
        /*&self->end_marker_pos, 128);*/
      position_set_to_bar (&self->loop_start_pos, 1);
      position_set_to_bar (&self->loop_end_pos, 5);

      self->loop = 1;

      self->metronome_enabled =
        g_settings_get_int (
          S_UI,
          "metronome-enabled");

      transport_set_bpm (self, DEFAULT_BPM);
    }

  /* set playstate */
  self->play_state = PLAYSTATE_PAUSED;

  zix_sem_init(&self->paused, 0);
}

/**
 * Gets beat unit as int.
 */
static inline BeatUnit
get_ebeat_unit (
  int beat_unit)
{
  switch (beat_unit)
    {
    case 2:
      return BEAT_UNIT_2;
    case 4:
      return BEAT_UNIT_4;
    case 8:
      return BEAT_UNIT_8;
    case 16:
      return BEAT_UNIT_16;;
    default:
      g_warn_if_reached ();
      return 0;
    }
}

/**
 * Updates beat unit and anything depending on it.
 */
void
transport_set_beat_unit (
  Transport * self,
  int beat_unit)
{
  self->beat_unit = beat_unit;
  self->ebeat_unit = get_ebeat_unit (beat_unit);

  /**
   * Regarding calculation:
   * 3840 = TICKS_PER_QUARTER_NOTE * 4 to get the ticks
   * per full note.
   * Divide by beat unit (e.g. if beat unit is 2,
   * it means it is a 1/2th note, so multiply 1/2
   * with the ticks per note
   */
  self->lticks_per_beat =
    (long)
    (3840.0 / (double) TRANSPORT->beat_unit);
  self->ticks_per_beat =
    (int) self->lticks_per_beat;
  self->lticks_per_bar =
    (long)
    (self->lticks_per_beat * self->beats_per_bar);
  self->ticks_per_bar =
    (int)
    self->lticks_per_bar;
  self->sixteenths_per_beat =
    (int)
    (16.0 / (double) self->beat_unit);
  self->sixteenths_per_bar =
    (int)
    (self->sixteenths_per_beat *
     self->beats_per_bar);
  g_warn_if_fail (self->ticks_per_bar > 0);
  g_warn_if_fail (self->ticks_per_beat > 0);
  g_warn_if_fail (self->lticks_per_bar > 0);
  g_warn_if_fail (self->lticks_per_beat > 0);
}

void
transport_set_ebeat_unit (
  Transport * self,
  BeatUnit bu)
{
  switch (bu)
    {
    case BEAT_UNIT_2:
      transport_set_beat_unit (self, 2);
      break;
    case BEAT_UNIT_4:
      transport_set_beat_unit (self, 4);
      break;
    case BEAT_UNIT_8:
      transport_set_beat_unit (self, 8);
      break;
    case BEAT_UNIT_16:
      transport_set_beat_unit (self, 16);
      break;
    default:
      g_warn_if_reached ();
      break;
    }
}

void
transport_request_pause (
  Transport * self)
{
  self->play_state = PLAYSTATE_PAUSE_REQUESTED;
}

void
transport_request_roll (
  Transport * self)
{
  g_message ("requesting roll");
  self->play_state = PLAYSTATE_ROLL_REQUESTED;
}


/**
 * Moves the playhead by the time corresponding to
 * given samples, taking into account the loop
 * end point.
 */
void
transport_add_to_playhead (
  Transport *     self,
  const nframes_t frames)
{
  transport_position_add_frames (
    self, &TRANSPORT->playhead_pos, frames);
  EVENTS_PUSH (ET_PLAYHEAD_POS_CHANGED, NULL);
}

/**
 * Setter for playhead Position.
 */
void
transport_set_playhead_pos (
  Transport * self,
  Position *  pos)
{
  position_set_to_pos (
    &self->playhead_pos, pos);
  EVENTS_PUSH (ET_PLAYHEAD_POS_CHANGED, NULL);
}

/**
 * Getter for playhead Position.
 */
void
transport_get_playhead_pos (
  Transport * self,
  Position *  pos)
{
  position_set_to_pos (
    pos, &self->playhead_pos);
}

/**
 * Moves playhead to given pos.
 *
 * This is only for moves other than while playing
 * and for looping while playing.
 *
 * @param target Position to set to.
 * @param panic Send MIDI panic or not.
 */
void
transport_move_playhead (
  Position * target,
  int      panic)
{
  int i, j, k, l;
  /* send MIDI note off on currently playing timeline
   * objects */
  Track * track;
  Region * region;
  MidiNote * midi_note;
  MidiEvents * midi_events;
  TrackLane * lane;
  for (i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];

      for (k = 0; k < track->num_lanes; k++)
        {
          lane = track->lanes[k];

          for (l = 0;
               l < lane->num_regions; l++)
            {
              region = lane->regions[l];

              if (!region_is_hit (
                    region, PLAYHEAD->frames))
                continue;

              for (j = 0;
                   j < region->num_midi_notes;
                   j++)
                {
                  midi_note = region->midi_notes[j];

                  if (midi_note_hit (
                        midi_note, PLAYHEAD->frames))
                    {
                      midi_events =
                        track->processor.
                          piano_roll->
                            midi_events;

                      zix_sem_wait (
                        &midi_events->access_sem);
                      midi_events_add_note_off (
                        midi_events, 1,
                        midi_note->val,
                        0, 1);
                      zix_sem_post (
                        &midi_events->access_sem);
                    }
                }
            }
        }
    }

  /* move to new pos */
  position_set_to_pos (&TRANSPORT->playhead_pos,
                       target);
  /*if (panic)*/
    /*{*/
      /*AUDIO_ENGINE->panic = 1;*/
    /*}*/

  EVENTS_PUSH (ET_PLAYHEAD_POS_CHANGED, NULL);
}

/**
 * Sets whether metronome is enabled or not.
 */
void
transport_set_metronome_enabled (
  Transport * self,
  const int   enabled)
{
  self->metronome_enabled = enabled;
}

/**
 * Updates the frames in all transport positions
 */
void
transport_update_position_frames (
  Transport * self)
{
  position_update_frames (
    &self->playhead_pos);
  position_update_frames (
    &self->cue_pos);
  /*position_update_frames (*/
    /*&TRANSPORT->start_marker_pos);*/
  /*position_update_frames (*/
    /*&TRANSPORT->end_marker_pos);*/
  position_update_frames (
    &self->loop_start_pos);
  position_update_frames (
    &self->loop_end_pos);
}

#define GATHER_MARKERS \
  /* gather all markers */ \
  Position markers[60]; \
  int num_markers = 0, i; \
  for (i = 0; \
       i < P_MARKER_TRACK->num_markers; i++) \
    { \
       position_set_to_pos ( \
          &markers[num_markers++], \
          &P_MARKER_TRACK->markers[i]->pos); \
    } \
  position_set_to_pos ( \
    &markers[num_markers++], \
    &self->cue_pos); \
  position_set_to_pos ( \
    &markers[num_markers++], \
    &self->loop_start_pos); \
  position_set_to_pos ( \
    &markers[num_markers++], \
    &self->loop_end_pos); \
  position_set_to_pos ( \
    &markers[num_markers++], \
    START_POS); \
  position_sort_array ( \
    markers, (size_t) num_markers)

/**
 * Moves the playhead to the prev Marker.
 */
void
transport_goto_prev_marker (
  Transport * self)
{
  GATHER_MARKERS;

  for (i = num_markers - 1; i >= 0; i--)
    {
      if (position_is_before (
            &markers[i], &self->playhead_pos) &&
          TRANSPORT_IS_ROLLING &&
          i > 0 &&
          (position_to_ms (&self->playhead_pos) -
             position_to_ms (&markers[i])) <
           180)
        {
          transport_move_playhead (
            &markers[i - 1], 1);
          break;
        }
      else if (
        position_is_before (
          &markers[i], &self->playhead_pos))
        {
          transport_move_playhead (
            &markers[i], 1);
          break;
        }
    }
}

/**
 * Moves the playhead to the next Marker.
 */
void
transport_goto_next_marker (
  Transport * self)
{
  GATHER_MARKERS;

  for (i = 0; i < num_markers; i++)
    {
      if (position_is_after (
            &markers[i], &self->playhead_pos))
        {
          transport_move_playhead (
            &markers[i], 1);
          break;
        }
    }
}

/**
 * Enables or disables loop.
 */
void
transport_set_loop (
  Transport * self,
  int         enabled)
{
  self->loop = enabled;

  EVENTS_PUSH (ET_LOOP_TOGGLED, NULL);
}

/**
 * Adds frames to the given global frames, while
 * adjusting the new frames to loop back if the
 * loop point was crossed.
 *
 * @return The new frames adjusted.
 */
long
transport_frames_add_frames (
  const Transport * self,
  const long        gframes,
  const nframes_t   frames)
{
  long new_frames = gframes + frames;

  /* if start frames were before the loop-end point
   * and the new frames are after (loop crossed) */
  if (TRANSPORT_IS_LOOPING &&
      gframes < self->loop_end_pos.frames &&
      new_frames >= self->loop_end_pos.frames)
    {
      /* adjust the new frames */
      new_frames +=
        self->loop_start_pos.frames -
        self->loop_end_pos.frames;
    }

  return new_frames;
}

/**
 * Adds frames to the given position similar to
 * position_add_frames(), except that it adjusts
 * the new Position to loop back if the loop end
 * point was crossed.
 */
void
transport_position_add_frames (
  const Transport * self,
  Position *        pos,
  const nframes_t   frames)
{
  long new_global_frames =
    transport_frames_add_frames (
      self, pos->frames, frames);
  position_from_frames (
    pos, new_global_frames);

  /* set the frames manually again because
   * position_from_frames rounds them */
  pos->frames = new_global_frames;
}

/**
 * Returns the number of processable frames until
 * and excluding the loop end point as a positive
 * number (>= 1) if the loop point was met between
 * g_start_frames and (g_start_frames + nframes),
 * otherwise returns 0;
 */
nframes_t
transport_is_loop_point_met (
  const Transport * self,
  const long        g_start_frames,
  const nframes_t   nframes)
{
  if (
    TRANSPORT_IS_LOOPING &&
    self->loop_end_pos.frames > g_start_frames &&
    self->loop_end_pos.frames <=
      g_start_frames + nframes)
    {
      return
        (nframes_t)
        (self->loop_end_pos.frames -
         g_start_frames);
    }
  return 0;
}
