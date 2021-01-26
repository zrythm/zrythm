/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include <inttypes.h>
#include <stdlib.h>

#include "zrythm-config.h"

#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/midi_event.h"
#include "audio/midi_note.h"
#include "audio/midi_track.h"
#include "audio/position.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/velocity.h"
#include "plugins/lv2/lv2_control.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "gui/widgets/track.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/stoat.h"

#include <gtk/gtk.h>

/**
 * Initializes an midi track.
 */
void
midi_track_init (
  Track * self)
{
  self->type = TRACK_TYPE_MIDI;
  gdk_rgba_parse (&self->color, "#F79616");

  self->icon_name = g_strdup ("instrument");
}

void
midi_track_setup (Track * self)
{
  channel_track_setup (self);
}

#if 0
/**
 * Add a note on event if note on exists before a
 * ZRegion loop end point during a region loop.
 */
static inline void
note_ons_during_region_loop (
  MidiEvents *     midi_events,
  MidiNote *       mn,
  ChordObject *    co,
  const long       r_local_pos,
  const long       r_local_end_pos,
  const long       g_frames_start,
  const nframes_t  local_start_frame,
  const nframes_t  nframes)
{
  g_message ("note on during loop");

  /* return if not looping */
  if (r_local_end_pos > r_local_pos)
    return;

  ArrangerObject * mn_obj =
    mn ?
    (ArrangerObject *) mn : (ArrangerObject *) co;
  ZRegion * region =
    arranger_object_get_region (mn_obj);
  ArrangerObject * r_obj = (ArrangerObject *) region;

  /* get frames till r loop end */
  long frames_till_r_loop_end =
    r_obj->loop_end_pos.frames -
      r_local_pos;

  /* get global frames at loop end */
  long g_r_loop_end =
    g_frames_start + frames_till_r_loop_end;
  /*long r_loop_length =*/
    /*region_get_loop_length_in_frames (mn->region);*/
  /*while (g_r_loop_end <= g_r_end)*/
    /*{*/
      /*g_r_loop_end += r_loop_length;*/
    /*}*/
  /*g_r_loop_end -= r_loop_length;*/

  if (
    /* mn starts before the loop point */
    mn_obj->pos.frames >= r_local_pos &&
    mn_obj->pos.frames <
      r_obj->loop_end_pos.frames - 1)
    {
      g_message (
        "midi note on during region loop (before "
        "loop point)");
      midi_time_t time =
        (midi_time_t)
        (mn_obj->pos.frames - r_local_pos);
      if (mn)
        {
          midi_events_add_note_on (
            midi_events,
            midi_region_get_midi_ch (region),
            mn->val, mn->vel->vel, time, 1);
        }
      else if (co)
        {
          ChordDescriptor * descr =
            chord_object_get_chord_descriptor (co);
          midi_events_add_note_ons_from_chord_descr (
            midi_events, descr, 1,
            VELOCITY_DEFAULT, time, F_QUEUED);
        }
      g_warn_if_fail (
        time < local_start_frame + nframes);
      return;
    }
  /* mn starts after the loop point */
  else if (
    mn_obj->pos.frames >=
      r_obj->loop_start_pos.frames &&
    mn_obj->pos.frames <
      r_obj->loop_start_pos.frames +
        r_local_end_pos &&
    /* we are still inside the region */
    r_local_end_pos + g_r_loop_end +
      (mn_obj->pos.frames -
         r_obj->loop_start_pos.frames) <
       r_obj->end_pos.frames)
    {
      g_message (
        "midi note on during region loop (after "
        "loop point)");
      midi_time_t time =
        (midi_time_t)
        (r_obj->loop_end_pos.frames -
           r_local_pos);
      if (mn)
        {
          midi_events_add_note_on (
            midi_events,
            midi_region_get_midi_ch (region),
            mn->val, mn->vel->vel, time, 1);
        }
      else if (co)
        {
          ChordDescriptor * descr =
            chord_object_get_chord_descriptor (co);
          midi_events_add_note_ons_from_chord_descr (
            midi_events, descr, 1,
            VELOCITY_DEFAULT, time, F_QUEUED);
        }
      g_warn_if_fail (
        time < local_start_frame + nframes);

      return;
    }
}

/**
 * Returns if the ZRegion is hit in the range.
 *
 * @param transport_loop_met Whether the transport
 *   loop point is met.
 * @param first_half If the transport loop point is
 *   met, this indicates if we are in the first half
 *   or second half.
 * @param g_start_frames Global start frames.
 * @param g_end_frames_excl Global end frames (the
 *   last frame is not part of the cycle).
 * @param inclusive Check if the ZRegion is hit
 *   with counting its end point or not.
 */
static int
region_hit (
  const ZRegion * r,
  const int      transport_loop_met,
  const int      first_half,
  const long     g_start_frames,
  const long     g_end_frames_excl,
  const int      inclusive)
{
  if (transport_loop_met)
    {
      /* check first half */
      if (first_half)
        {
          /* check also if region is
           * hit without counting its
           * end point */
          return
            region_is_hit_by_range (
              r, g_start_frames,
              TRANSPORT->loop_end_pos.frames,
              inclusive);
        }
      /* check second half */
      else
        {
          /* skip regions not hit */
          return
            region_is_hit_by_range (
              r,
              TRANSPORT->loop_start_pos.frames,
              g_end_frames_excl, inclusive);
        }
    }
  else
    {
      return
        region_is_hit_by_range (
          r, g_start_frames,
          g_end_frames_excl, inclusive);
    }
}
#endif

/**
 * Fills MIDI event queue from track.
 *
 * The events are dequeued right after the call to
 * this function.
 *
 * @note The engine splits the cycle so transport
 *   loop related logic is not needed.
 *
 * Caveats:
 * - This will not work properly if the loop sizes
 *   (region or transport) are smaller than nframes,
 *   so small sizes should not be allowed.
 *
 * @param g_start_frame Global start frame.
 * @param local_start_frame The start frame offset
 *   from 0 in this cycle.
 * @param nframes Number of frames at start
 *   Position.
 * @param midi_events MidiEvents to fill (from
 *   Piano Roll Port for example).
 */
REALTIME
void
midi_track_fill_midi_events (
  Track *         track,
  const long      g_start_frames,
  const nframes_t local_start_frame,
  nframes_t       nframes,
  MidiEvents *    midi_events)
{
  const long g_end_frames = g_start_frames + nframes;

  zix_sem_wait (&midi_events->access_sem);

#if 0
  g_message (
    "TRACK %s STARTING from %ld, "
    "local start frame %u, nframes %u",
    track->name, g_start_frames,
    local_start_frame, nframes);
#endif

  /* go through each lane */
  const int num_loops =
    (track->type == TRACK_TYPE_CHORD ?
     1 : track->num_lanes);
  for (int j = 0; j < num_loops; j++)
    {
      TrackLane * lane = NULL;
      if (track->type != TRACK_TYPE_CHORD)
        {
          lane = track->lanes[j];
        }

      /* go through each region */
      const int num_regions =
        (track->type == TRACK_TYPE_CHORD ?
         track->num_chord_regions :
         lane->num_regions);
      for (int i = 0; i < num_regions; i++)
        {
          ZRegion * r =
            track->type == TRACK_TYPE_CHORD ?
            track->chord_regions[i] :
            lane->regions[i];
          ArrangerObject * r_obj =
            (ArrangerObject *) r;

          /* skip region if muted */
          if (arranger_object_get_muted (r_obj))
            continue;

          /* skip if in bounce mode and the
           * region should not be bounced */
          if (AUDIO_ENGINE->bounce_mode !=
                BOUNCE_OFF &&
              (!r->bounce || !track->bounce))
            continue;

          /* skip if region is not hit
           * (inclusive of its last point) */
          if (!region_is_hit_by_range (
                 r, g_start_frames,
                 g_end_frames, F_INCLUSIVE))
            {
              continue;
            }

          long num_frames_to_process =
            MIN (
              r_obj->end_pos.frames - g_start_frames,
              nframes);
          nframes_t frames_processed = 0;

          while (num_frames_to_process > 0)
            {
              long cur_g_start_frame =
                g_start_frames + frames_processed;
              nframes_t cur_local_start_frame =
                local_start_frame + frames_processed;

              bool is_end_loop;
              long cur_num_frames;
              region_get_frames_till_next_loop_or_end (
                r, cur_g_start_frame,
                &cur_num_frames, &is_end_loop);

#if 0
              g_message (
                "cur num frames %ld, "
                "num_frames_to_process %ld, "
                "cur local start frame %u",
                cur_num_frames,
                num_frames_to_process,
                cur_local_start_frame);
#endif

              /* whether we need a note off (ie,
               * the cur_num_frames are for region
               * end and not a random place) */
              bool need_note_off =
                (cur_num_frames <
                   num_frames_to_process) ||
                (TRANSPORT_IS_LOOPING &&
                 g_start_frames +
                   num_frames_to_process ==
                     TRANSPORT->loop_end_pos.frames);

              /* number of frames to process this
               * time */
              cur_num_frames =
                MIN (
                  cur_num_frames,
                  num_frames_to_process);

              midi_region_fill_midi_events (
                r, cur_g_start_frame,
                cur_local_start_frame,
                cur_num_frames, need_note_off,
                midi_events);

              frames_processed += cur_num_frames;
              num_frames_to_process -=
                cur_num_frames;
            } /* end while frames left */
        }
    }

#if 0
  g_message ("TRACK %s ENDING", track->name);
#endif

  midi_events_clear_duplicates (
    midi_events, F_QUEUED);

  /* sort events */
  midi_events_sort (midi_events, F_QUEUED);

  zix_sem_post (&midi_events->access_sem);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
midi_track_free (Track * track)
{
}

