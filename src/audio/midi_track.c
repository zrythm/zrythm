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

#include <stdlib.h>

#include "config.h"

#include "audio/automatable.h"
#include "audio/automation_track.h"
#include "audio/automation_tracklist.h"
#include "audio/midi_track.h"
#include "audio/midi.h"
#include "audio/midi_note.h"
#include "audio/position.h"
#include "audio/region.h"
#include "audio/track.h"
#include "audio/velocity.h"
#include "plugins/lv2/lv2_control.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "gui/widgets/track.h"
#include "utils/arrays.h"
#include "utils/stoat.h"

#include <gtk/gtk.h>

/**
 * Initializes an midi track.
 */
void
midi_track_init (Track * track)
{
  track->type = TRACK_TYPE_MIDI;
  gdk_rgba_parse (&track->color, "#F79616");
}

/**
 * Add MIDI automatables to the automation tracklist.
 */
void
midi_track_add_midi_automatables (
  Track * self)
{
  /* TODO */
#if 0
  AutomationTracklist * atl =
    track_get_automation_tracklist (track);
  AutomationTrack * at;


  at = automation_track_new (track->mute);
  automation_tracklist_add_at (atl, at);
#endif
}

void
midi_track_setup (Track * self)
{
  channel_track_setup (self);
}

/**
 * To be passed to functions to specify what action
 * should be taken.
 */
typedef enum NoteProcessingType
{
  /** Send all notes off due to transport loop
   * point hit. */
  TYPE_TRANSPORT_LOOP,

  /** Send notes off due to ZRegion actual end
   * within this cycle. */
  TYPE_REGION_END,

  /** Send all notes off due to ZRegion loop end
   * point crossed. */
  TYPE_REGION_LOOP_END,
} NoteProcessingType;

/**
 * Sends all MIDI notes off at the given local
 * point.
 */
static inline void
send_notes_off_at (
  ZRegion *          region,
  MidiEvents *       midi_events,
  midi_time_t        time,
  NoteProcessingType type)
{
  ArrangerObject * r_obj =
    (ArrangerObject *) region;
  MidiNote * mn;
  ArrangerObject * mn_obj;
  switch (type)
    {
    case TYPE_TRANSPORT_LOOP:
      g_message (
        "--- sending notes off TRANSPORT_LOOP");
      for (int i = 0;
           i < region->num_midi_notes;
           i++)
        {
          mn =
            region->midi_notes[i];
          mn_obj =
            (ArrangerObject *) mn;
          if (arranger_object_get_muted (mn_obj))
            continue;

          /* FIXME check if note is on
           * first */
          midi_events_add_note_off (
            midi_events,
            midi_region_get_midi_ch (region),
            mn->val,
            time, 1);
        }
      break;
    case TYPE_REGION_END:
      g_message (
        "--- sending notes off REGION_END");
      {
        long tmp_start;
        long tmp_end;
        int num_loops =
          arranger_object_get_num_loops (r_obj, 0);
        long region_end_adjusted =
          arranger_object_get_length_in_frames (
            r_obj);
        long loop_length_frames =
          arranger_object_get_loop_length_in_frames (
            r_obj);

        for (int i = 0;
             i < region->num_midi_notes;
             i++)
          {
            mn = region->midi_notes[i];
            mn_obj =
              (ArrangerObject *) mn;
            if (arranger_object_get_muted (mn_obj))
              continue;

            /* add num_loops * loop_ticks to the
             * midi note start & end poses to get
             * last pos in region */
            tmp_start =
              mn_obj->pos.frames;
            tmp_end =
              mn_obj->end_pos.frames;
            for (int j = 0; j < num_loops; j++)
              {
                tmp_start +=
                  loop_length_frames;
                tmp_end +=
                  loop_length_frames;
              }

            /* if note is on at the boundary, send
             * note off */
            if (tmp_start <
                  region_end_adjusted &&
                tmp_end >=
                  region_end_adjusted)
              {
                midi_events_add_note_off (
                  midi_events,
                  midi_region_get_midi_ch (region),
                  mn->val, time, 1);
              }
          }
      }
      break;
    case TYPE_REGION_LOOP_END:
      g_message (
        "--- sending notes off REGION_LOOP_END");
      {
        long region_loop_end_adj =
          r_obj->loop_end_pos.frames -
          r_obj->clip_start_pos.frames;
        for (int i = 0;
             i < region->num_midi_notes;
             i++)
          {
            mn = region->midi_notes[i];
            mn_obj =
              (ArrangerObject *) mn;
            if (arranger_object_get_muted (mn_obj))
              continue;

            /* if note is on at the boundary, send
             * note off */
            if (mn_obj->pos.frames <
                  region_loop_end_adj &&
                mn_obj->end_pos.frames >=
                  region_loop_end_adj)
              {
                midi_events_add_note_off (
                  midi_events,
                  midi_region_get_midi_ch (region),
                  mn->val, time, 1);
              }
          }
      }
      break;
    }
}

/**
 * Add a note on event if note on exists before a
 * ZRegion loop end point during a region loop.
 */
static inline void
note_ons_during_region_loop (
  MidiEvents *     midi_events,
  MidiNote *       mn,
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
    (ArrangerObject *) mn;
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
      midi_events_add_note_on (
        midi_events,
        midi_region_get_midi_ch (region),
        mn->val, mn->vel->vel, time, 1);
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
      midi_events_add_note_on (
        midi_events,
        midi_region_get_midi_ch (region),
        mn->val, mn->vel->vel, time, 1);
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

/**
 * Fills MIDI event queue from track.
 *
 * The events are dequeued right after the call to
 * this function.
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
  int i, j, k, kk;
  long g_end_frames;
  long r_local_pos, r_local_end_pos;
  ZRegion * r;
  ArrangerObject * r_obj, * mn_obj;
  MidiNote * mn;
  midi_time_t time;

  g_end_frames =
    transport_frames_add_frames (
      TRANSPORT, g_start_frames, nframes);
  int transport_loop_met =
    g_end_frames < g_start_frames;
  unsigned int diff_to_tp_loop_end;
  int region_hit_exclusive = 0;
  int region_loop_met = 0;

  if (transport_loop_met)
    {
      diff_to_tp_loop_end =
        (unsigned int)
        (TRANSPORT->loop_end_pos.frames -
         g_start_frames);
    }
  else
    {
      diff_to_tp_loop_end = 0;
    }

  zix_sem_wait (&midi_events->access_sem);

  TrackLane * lane;

  /* loop once if no loop met, twice if loop met
   * (split the ranges in 2) */
  for (k = 0; k < transport_loop_met + 1; k++)
    {
      for (j = 0; j < track->num_lanes; j++)
        {
          lane = track->lanes[j];

          for (i = 0; i < lane->num_regions; i++)
            {
              r = lane->regions[i];
              r_obj = (ArrangerObject *) r;
              if (arranger_object_get_muted (r_obj))
                continue;

              region_loop_met = 0;

              /* skip if in bounce mode and the
               * region should not be bounced */
              if (AUDIO_ENGINE->bounce_mode !=
                    BOUNCE_OFF &&
                  (!r->bounce || !track->bounce))
                continue;

              /* skip if region is not hit
               * (inclusive of its last point) */
              if (
                !region_hit (
                  r, transport_loop_met, k == 0,
                  g_start_frames, g_end_frames,
                  1))
                {
                  continue;
                }

              /* check if region is hit exclusive
               * of its last point */
              region_hit_exclusive =
                region_hit (
                  r, transport_loop_met, k == 0,
                  g_start_frames, g_end_frames,
                  0);

              /* get local positions */
              /* first half (before loop end) */
              if (transport_loop_met && k == 0)
                {
                  r_local_pos =
                    region_timeline_frames_to_local (
                      r, g_start_frames, 1);
                  r_local_end_pos =
                    region_timeline_frames_to_local (
                      r,
                      TRANSPORT->
                        loop_end_pos.frames,
                      1);
                }
              /* second half (after loop start) */
              else if (transport_loop_met && k == 1)
                {
                  r_local_pos =
                    region_timeline_frames_to_local (
                      r,
                      TRANSPORT->loop_start_pos.
                        frames, 1);
                  r_local_end_pos =
                    region_timeline_frames_to_local (
                      r, g_end_frames, 1);
                }
              else
                {
                  r_local_pos =
                    region_timeline_frames_to_local (
                      r, g_start_frames, 1);
                  r_local_end_pos =
                    region_timeline_frames_to_local (
                      r, g_end_frames, 1);
                }

              /* send all MIDI notes off if the
               * transport loop end point is
               * reached within this cycle */
              if (transport_loop_met && k == 0)
                {
                  time =
                    (midi_time_t)
                      /* -1 to send event 1 sample
                       * before the tp loop point */
                      (diff_to_tp_loop_end - 1);
                  send_notes_off_at (
                    r, midi_events, time,
                    TYPE_TRANSPORT_LOOP);
                  g_warn_if_fail (
                    time <
                      local_start_frame + nframes);
                }

              /* if region actually ends on the
               * timeline within this cycle */
              if (
                r_obj->end_pos.frames !=
                  TRANSPORT->loop_end_pos.frames &&
                /* greater than because we need at
                 * least 1 sample before */
                r_obj->end_pos.frames >
                  g_start_frames &&
                /* <= because we need 1 sample
                 * before */
                r_obj->end_pos.frames <=
                  g_end_frames)
                {
                  time =
                    (midi_time_t)
                    /* -1 sample to end 1 sample
                     * earlier than the end pos */
                    ((r_obj->end_pos.frames - 1) -
                      g_start_frames);
                  send_notes_off_at (
                    r, midi_events, time,
                    TYPE_REGION_END);
                  g_warn_if_fail (
                    time <
                    local_start_frame + nframes);
                }

              if (r_local_end_pos <
                    r_local_pos)
                {
                  /* diff_from_loop_start will be 0
                   * unless this is the 2nd part of
                   * a split loop (k is 1).
                   * subtract 1 sample to make the
                   * note end 1 sample before its
                   * end point */
                  time =
                    (midi_time_t)
                    (((r_obj->loop_end_pos.frames -
                      r_local_pos)) - 1);
                  send_notes_off_at (
                    r, midi_events, time,
                    TYPE_REGION_LOOP_END);
                  g_warn_if_fail (
                    time <
                    local_start_frame + nframes);
                  region_loop_met = 1;
                }

              for (kk = 0;
                   kk < r->num_midi_notes; kk++)
                {
                  mn =
                    r->midi_notes[kk];
                  mn_obj =
                    (ArrangerObject *) mn;
                  if (arranger_object_get_muted (
                        mn_obj))
                    continue;

                  /* add note ons during region
                   * loop */
                  if (region_loop_met)
                    {
                      note_ons_during_region_loop (
                        midi_events, mn, r_local_pos,
                        r_local_end_pos,
                        g_start_frames,
                        local_start_frame,
                        nframes);
                    }
                  /* add note ons in normal case */
                  else if (
                    region_hit_exclusive &&
                    /* make sure note is after clip
                     * start */
                    mn_obj->pos.frames >=
                      r_obj->clip_start_pos.frames &&
                    mn_obj->pos.frames >=
                      r_local_pos &&
                    mn_obj->pos.frames <
                      r_local_pos + (long) nframes)
                    {
                      g_message (
                        "normal note on");
                      time =
                        (midi_time_t)
                        ((mn_obj->pos.frames -
                           r_local_pos) +
                         (long) diff_to_tp_loop_end);
                      midi_events_add_note_on (
                        midi_events,
                        midi_region_get_midi_ch (
                          r),
                        mn->val,
                        mn->vel->vel,
                        time, 1);
                      g_warn_if_fail (
                        time <
                        local_start_frame + nframes);
                    }

                  /* note off event */
                  /* if note ends within the
                   * cycle */
                  if (
                    /* note ends after the cycle
                     * start */
                    mn_obj->end_pos.frames >=
                      r_local_pos &&
                    /* either note ends before
                     * the cycle end or the region
                     * looped */
                    (mn_obj->end_pos.frames <=
                       r_local_end_pos ||
                     region_loop_met))
                    {
                      /* set time based on whether
                       * note ends before the cycle
                       * end or the region was
                       * looped */
                      if (mn_obj->end_pos.frames <=
                            r_local_end_pos)
                        time =
                          (midi_time_t)
                          (((mn_obj->end_pos.frames -
                             r_local_pos) - 1) +
                            diff_to_tp_loop_end);
                      else /* region was looped */
                        time =
                          (midi_time_t)
                          ((r_obj->loop_end_pos.frames -
                              r_local_pos) - 1);
                      midi_events_add_note_off (
                        midi_events,
                        midi_region_get_midi_ch (
                          r),
                        mn->val,
                        time, 1);
                      g_warn_if_fail (
                        time <
                        local_start_frame +
                          nframes);
                    }
                }
            }
        }
    }

  midi_events_clear_duplicates (
    midi_events, 1);

  /* sort events */
  midi_events_sort (midi_events, 1);

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

