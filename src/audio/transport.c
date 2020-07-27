/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-config.h"

#include "audio/audio_region.h"
#include "audio/engine.h"
#include "audio/marker.h"
#include "audio/marker_track.h"
#include "audio/midi.h"
#include "audio/transport.h"
#include "project.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
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
#include "settings/settings.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

static void
init_common (
  Transport * self)
{
  /* set playstate */
  self->play_state = PLAYSTATE_PAUSED;

  self->loop =
    ZRYTHM_TESTING ? true :
    g_settings_get_boolean (
      S_TRANSPORT, "loop");
  self->metronome_enabled =
    ZRYTHM_TESTING ? true :
    g_settings_get_boolean (
      S_TRANSPORT, "metronome-enabled");

  zix_sem_init (&self->paused, 0);
}

void
transport_init_loaded (
  Transport * self)
{
  transport_set_beat_unit (
    self, self->beat_unit);

  init_common (self);
}

/**
 * Create a new transport.
 */
Transport *
transport_new (
  AudioEngine * engine)
{
  g_message ("%s: Creating transport...", __func__);

  Transport * self = object_new (Transport);

  if (engine)
    {
      engine->transport = self;
    }

  // set inital total number of beats
  // this is applied to the ruler
  self->total_bars =
    TRANSPORT_DEFAULT_TOTAL_BARS;

  /* set BPM related defaults */
  self->beats_per_bar =
    TRANSPORT_DEFAULT_BEATS_PER_BAR;
  transport_set_beat_unit (self, 4);

  /* set positions */
  position_set_to_bar (&self->playhead_pos, 1);
  position_set_to_bar (&self->cue_pos, 1);
  position_set_to_bar (&self->loop_start_pos, 1);
  position_set_to_bar (&self->loop_end_pos, 5);

  position_set_to_bar (&self->range_1, 1);
  position_set_to_bar (&self->range_2, 1);

  init_common (self);

  return self;
}

/**
 * Prepares audio regions for stretching (sets the
 * \ref ZRegion.before_length).
 *
 * @param selections If NULL, all audio regions
 *   are used. If non-NULL, only the regions in the
 *   selections are used.
 */
void
transport_prepare_audio_regions_for_stretch (
  Transport *          self,
  TimelineSelections * sel)
{
  if (sel)
    {
      for (int i = 0; i < sel->num_regions; i++)
        {
          ZRegion * region = sel->regions[i];
          region->before_length =
            arranger_object_get_length_in_ticks (
              (ArrangerObject *) region);
        }
    }
  else
    {
      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * track = TRACKLIST->tracks[i];

          if (track->type != TRACK_TYPE_AUDIO)
            continue;

          for (int j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              for (int k = 0; k < lane->num_regions;
                   k++)
                {
                  ZRegion * region =
                    lane->regions[k];
                  region->before_length =
                    arranger_object_get_length_in_ticks (
                      (ArrangerObject *) region);
                } // foreach region
            } // foreach lane
        } // foreach track
    }
}

/**
 * Stretches audio regions.
 *
 * @param selections If NULL, all audio regions
 *   are used. If non-NULL, only the regions in the
 *   selections are used.
 * @param with_fixed_ratio Stretch all regions with
 *   a fixed ratio. If this is off, the current
 *   region length and \ref ZRegion.before_length
 *   will be used to calculate the ratio.
 */
void
transport_stretch_audio_regions (
  Transport *          self,
  TimelineSelections * sel,
  bool                 with_fixed_ratio,
  double               time_ratio)
{
  if (sel)
    {
      for (int i = 0; i < sel->num_regions; i++)
        {
          ZRegion * region =
            TL_SELECTIONS->regions[i];
          ArrangerObject * r_obj =
            (ArrangerObject *) region;
          double ratio =
            with_fixed_ratio ? time_ratio :
            arranger_object_get_length_in_ticks (
              r_obj) /
            region->before_length;
          region_stretch (region, ratio);
        }
    }
  else
    {
      for (int i = 0; i < TRACKLIST->num_tracks; i++)
        {
          Track * track = TRACKLIST->tracks[i];

          if (track->type != TRACK_TYPE_AUDIO)
            continue;

          for (int j = 0; j < track->num_lanes; j++)
            {
              TrackLane * lane = track->lanes[j];

              for (int k = 0; k < lane->num_regions;
                   k++)
                {
                  ZRegion * region =
                    lane->regions[k];
                  ArrangerObject * r_obj =
                    (ArrangerObject *) region;
                  double ratio =
                    with_fixed_ratio ? time_ratio :
                    arranger_object_get_length_in_ticks (
                      r_obj) /
                    region->before_length;
                  region_stretch (region, ratio);
                }
            }
        }
    }
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
  int         beat_unit)
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
  self->ticks_per_beat =
    3840.0 / (double) self->beat_unit;
  self->ticks_per_bar =
    (self->ticks_per_beat * self->beats_per_bar);
  self->sixteenths_per_beat = 16 / self->beat_unit;
  self->sixteenths_per_bar =
    (self->sixteenths_per_beat *
     self->beats_per_bar);
  g_warn_if_fail (self->ticks_per_bar > 0.0);
  g_warn_if_fail (self->ticks_per_beat > 0.0);
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

  if (g_settings_get_boolean (
        S_TRANSPORT, "return-to-cue"))
    {
      transport_move_playhead (
        self, &self->cue_pos, F_NO_PANIC,
        F_NO_SET_CUE_POINT);
    }
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
  EVENTS_PUSH (
    ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
}

/**
 * Getter for playhead Position.
 */
void
transport_get_playhead_pos (
  Transport * self,
  Position *  pos)
{
  g_return_if_fail (self && pos);

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
 * @param set_cue_point Also set the cue point at
 *   this position.
 */
void
transport_move_playhead (
  Transport * self,
  Position *  target,
  bool        panic,
  bool        set_cue_point)
{
  int i, j, k, l;
  /* send MIDI note off on currently playing timeline
   * objects */
  Track * track;
  ZRegion * region;
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
                    region, PLAYHEAD->frames, 1))
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
                        track->processor->
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
  position_set_to_pos (&self->playhead_pos, target);

  if (set_cue_point)
    {
      /* move cue point */
      position_set_to_pos (&self->cue_pos, target);
    }

  EVENTS_PUSH (
    ET_PLAYHEAD_POS_CHANGED_MANUALLY, NULL);
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
  g_settings_set_boolean (
    S_TRANSPORT, "metronome-enabled", enabled);
}

/**
 * Returns the PPQN (Parts/Ticks Per Quarter Note).
 */
double
transport_get_ppqn (
  Transport * self)
{
  double res =
    self->ticks_per_beat *
    ((double) self->beat_unit / 4.0);
  return res;
}

/**
 * Updates the frames in all transport positions
 */
void
transport_update_position_frames (
  Transport * self)
{
  position_update_ticks_and_frames (
    &self->playhead_pos);
  position_update_ticks_and_frames (
    &self->cue_pos);
  /*position_update_ticks_and_frames (*/
    /*&TRANSPORT->start_marker_pos);*/
  /*position_update_ticks_and_frames (*/
    /*&TRANSPORT->end_marker_pos);*/
  position_update_ticks_and_frames (
    &self->loop_start_pos);
  position_update_ticks_and_frames (
    &self->loop_end_pos);
}

#define GATHER_MARKERS \
  /* gather all markers */ \
  Position markers[60]; \
  int num_markers = 0, i; \
  for (i = 0; \
       i < P_MARKER_TRACK->num_markers; i++) \
    { \
      ArrangerObject * m_obj = \
        (ArrangerObject *) \
        P_MARKER_TRACK->markers[i]; \
       position_set_to_pos ( \
          &markers[num_markers++], \
          &m_obj->pos); \
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
    &POSITION_START); \
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
            self, &markers[i - 1], F_PANIC,
            F_SET_CUE_POINT);
          break;
        }
      else if (
        position_is_before (
          &markers[i], &self->playhead_pos))
        {
          transport_move_playhead (
            self, &markers[i],
            F_PANIC, F_SET_CUE_POINT);
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
            self, &markers[i], F_PANIC,
            F_SET_CUE_POINT);
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
  bool        enabled)
{
  self->loop = enabled;
  g_settings_set_boolean (
    S_TRANSPORT, "loop", enabled);

  EVENTS_PUSH (ET_LOOP_TOGGLED, NULL);
}

#if 0
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
  long new_frames = gframes + (long) frames;

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
#endif

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
  Position pos_before_adding = *pos;
  position_add_frames (pos, frames);

  /* if start frames were before the loop-end point
   * and the new frames are after (loop crossed) */
  if (TRANSPORT_IS_LOOPING &&
      pos_before_adding.total_ticks <
        self->loop_end_pos.total_ticks &&
      pos->total_ticks >=
        self->loop_end_pos.total_ticks)
    {
      /* adjust the new frames */
      position_add_ticks (
        pos,
        self->loop_start_pos.total_ticks -
          self->loop_end_pos.total_ticks);
    }

  /*long new_global_frames =*/
    /*transport_frames_add_frames (*/
      /*self, pos->frames, frames);*/
  /*position_from_frames (*/
    /*pos, new_global_frames);*/

  /* set the frames manually again because
   * position_from_frames rounds them */
  /*pos->frames = new_global_frames;*/
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
      g_start_frames + (long) nframes)
    {
      return
        (nframes_t)
        (self->loop_end_pos.frames -
         g_start_frames);
    }
  return 0;
}

/**
 * Sets if the project has range and updates UI.
 */
void
transport_set_has_range (
  Transport * self,
  bool        has_range)
{
  self->has_range = has_range;

  EVENTS_PUSH (ET_RANGE_SELECTION_CHANGED, NULL);
}

/**
 * Stores the position of the range in \ref pos.
 */
void
transport_get_range_pos (
  Transport * self,
  bool        first,
  Position *  pos)
{
  bool range1_first =
    position_is_before_or_equal (
      &self->range_1, &self->range_2);

  if (first)
    {
      if (range1_first)
        {
          position_set_to_pos (pos, &self->range_1);
        }
      else
        {
          position_set_to_pos (pos, &self->range_2);
        }
    }
  else
    {
      if (range1_first)
        {
          position_set_to_pos (pos, &self->range_2);
        }
      else
        {
          position_set_to_pos (pos, &self->range_1);
        }
    }
}

void
transport_set_range1 (
  Transport * self,
  Position *  pos,
  bool        snap)
{
  Position init_pos;
  position_init (&init_pos);
  if (position_is_before (pos, &init_pos))
    {
      position_set_to_pos (
        &self->range_1, &init_pos);
    }
  else
    {
      position_set_to_pos (
        &self->range_1, pos);
    }

  if (snap)
    {
      position_snap_simple (
        &self->range_1,
        SNAP_GRID_TIMELINE);
    }
}

void
transport_set_range2 (
  Transport * self,
  Position *  pos,
  bool        snap)
{
  Position init_pos;
  position_init (&init_pos);
  if (position_is_before (pos, &init_pos))
    {
      position_set_to_pos (
        &self->range_2, &init_pos);
    }
  else
    {
      position_set_to_pos (
        &self->range_2, pos);
    }

  if (snap)
    {
      position_snap_simple (
        &self->range_2,
        SNAP_GRID_TIMELINE);
    }
}

void
transport_free (
  Transport * self)
{
  zix_sem_destroy (&self->paused);

  object_zero_and_free (self);
}
