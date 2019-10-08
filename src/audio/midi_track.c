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
#include "plugins/lv2/control.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "gui/widgets/track.h"
#include "gui/widgets/automation_track.h"
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

  /** Send notes off due to Region actual end
   * within this cycle. */
  TYPE_REGION_END,

  /** Send all notes off due to Region loop end
   * point crossed. */
  TYPE_REGION_LOOP_END,
} NoteProcessingType;

/**
 * Sends all MIDI notes off at the given local
 * point.
 */
static inline void
send_notes_off_at (
  Region *           region,
  MidiEvents *       midi_events,
  midi_time_t        time,
  NoteProcessingType type)
{
  MidiNote * mn;
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

          /* FIXME check if note is on
           * first */
          midi_events_add_note_off (
            midi_events,
            midi_region_get_midi_ch (
              mn->region),
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
          region_get_num_loops (region, 0);
        long region_end_adjusted =
          region_get_full_length_in_frames (
            region);
        long loop_length_frames =
          region_get_loop_length_in_frames (region);

        for (int i = 0;
             i < region->num_midi_notes;
             i++)
          {
            mn = region->midi_notes[i];

            /* add num_loops * loop_ticks to the
             * midi note start & end poses to get
             * last pos in region */
            tmp_start =
              mn->start_pos.frames;
            tmp_end =
              mn->end_pos.frames;
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
                  midi_region_get_midi_ch (
                    mn->region),
                  mn->val,
                  time, 1);
              }
          }
      }
      break;
    case TYPE_REGION_LOOP_END:
      g_message (
        "--- sending notes off REGION_LOOP_END");
      {
        long region_loop_end_adj =
          region->loop_end_pos.frames -
          region->clip_start_pos.frames;
        for (int i = 0;
             i < region->num_midi_notes;
             i++)
          {
            mn = region->midi_notes[i];

            /* if note is on at the boundary, send
             * note off */
            if (mn->start_pos.frames <
                  region_loop_end_adj &&
                mn->end_pos.frames >=
                  region_loop_end_adj)
              {
                midi_events_add_note_off (
                  midi_events,
                  midi_region_get_midi_ch (
                    mn->region),
                  mn->val,
                  time, 1);
              }
          }
      }
      break;
    }
}

/**
 * Fills MIDI event queue from track.
 *
 * The events are dequeued right after the call to
 * this function.
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
  Track * track,
  const long      g_start_frames,
  const nframes_t local_start_frame,
  nframes_t       nframes,
  MidiEvents *    midi_events)
{
  int i, j, k, kk;
  /* global end frames.
   * the inclusive end frames are 1 sample before
   * the exclusive ones */
  long g_end_frames_excl, g_end_frames_incl;
  long local_pos, local_end_pos_excl,
       local_end_pos_incl;
  long region_loop_start_adj,
       region_loop_end_adj,
       clip_start_frames,
       mn_start_frames, mn_end_frames,
       region_end_frames;
  Region * region, * r;
  MidiNote * midi_note;
  midi_time_t time;

  g_end_frames_excl =
    transport_frames_add_frames (
      TRANSPORT, g_start_frames, nframes);
  g_end_frames_incl =
    transport_frames_add_frames (
      TRANSPORT, g_start_frames, nframes - 1);
  int transport_loop_met =
    g_end_frames_excl < g_start_frames;
  unsigned int diff_to_loop_end;
  int region_hit_exclusive = 0;

  if (transport_loop_met)
    {
      diff_to_loop_end =
        (unsigned int)
        (TRANSPORT->loop_end_pos.frames -
         g_start_frames);
    }
  else
    {
      diff_to_loop_end = 0;
    }

  zix_sem_wait (&midi_events->access_sem);

  TrackLane * lane;

  /* loop once if no loop met, twice if loop met
   * (split the  ranges in 2) */
  for (k = 0; k < transport_loop_met + 1; k++)
    {
      for (j = 0; j < track->num_lanes; j++)
        {
          lane = track->lanes[j];

          for (i = 0; i < lane->num_regions; i++)
            {
              region = lane->regions[i];
              r = (Region *) region;

              region_end_frames =
                region->end_pos.frames;

              if (transport_loop_met)
                {
                  /* check first half */
                  if (k == 0)
                    {
                      /* skip regions not hit */
                      if (!region_is_hit_by_range (
                            r, g_start_frames,
                            TRANSPORT->
                              loop_end_pos.frames,
                          1))
                        continue;

                      /* check also if region is
                       * hit without counting its
                       * end point */
                      region_hit_exclusive =
                        region_is_hit_by_range (
                          r, g_start_frames,
                          TRANSPORT->
                            loop_end_pos.frames,
                          0);
                    }
                  /* check second half */
                  else if (k == 1)
                    {
                      /* skip regions not hit */
                      if (!region_is_hit_by_range (
                            r,
                            TRANSPORT->
                              loop_start_pos.frames,
                            g_end_frames_excl,
                          1))
                        continue;

                      /* check also if region is
                       * hit without counting its
                       * end point */
                      region_hit_exclusive =
                        region_is_hit_by_range (
                          r,
                          TRANSPORT->
                            loop_start_pos.frames,
                          g_end_frames_excl,
                          0);
                    }
                }
              else
                {
                  /* skip regions not hit */
                  if (!region_is_hit_by_range (
                        r, g_start_frames,
                        g_end_frames_excl,
                      1))
                    continue;

                  /* check also if region is
                   * hit without counting its
                   * end point */
                  region_hit_exclusive =
                    region_is_hit_by_range (
                      r, g_start_frames,
                      g_end_frames_excl, 0);
                }

              /* get local positions */
              /* first half (before loop end) */
              if (transport_loop_met && k == 0)
                {
                  local_pos =
                    region_timeline_frames_to_local (
                      r, g_start_frames, 1);
                  local_end_pos_excl =
                    region_timeline_frames_to_local (
                      r,
                      TRANSPORT->
                        loop_end_pos.frames,
                      1);
                  local_end_pos_incl =
                    region_timeline_frames_to_local (
                      r,
                      TRANSPORT->
                        loop_end_pos.frames - 1,
                      1);
                }
              /* second half (after loop start) */
              else if (transport_loop_met && k == 1)
                {
                  local_pos =
                    region_timeline_frames_to_local (
                      r,
                      TRANSPORT->loop_start_pos.
                        frames, 1);
                  local_end_pos_excl =
                    region_timeline_frames_to_local (
                      r, g_end_frames_excl, 1);
                  local_end_pos_incl =
                    region_timeline_frames_to_local (
                      r, g_end_frames_incl, 1);
                }
              else
                {
                  local_pos =
                    region_timeline_frames_to_local (
                      r, g_start_frames, 1);
                  local_end_pos_excl =
                    region_timeline_frames_to_local (
                      r, g_end_frames_excl, 1);
                  local_end_pos_incl =
                    region_timeline_frames_to_local (
                      r, g_end_frames_incl, 1);
                }

              /* add clip_start position to start
               * from there */
              clip_start_frames =
                r->clip_start_pos.frames;
              region_loop_start_adj =
                r->loop_start_pos.frames;
              region_loop_end_adj =
                r->loop_end_pos.frames;
              local_pos += clip_start_frames;
              local_end_pos_excl +=
                clip_start_frames;
              local_end_pos_incl +=
                clip_start_frames;
              region_loop_start_adj -=
                clip_start_frames;
              region_loop_end_adj -=
                clip_start_frames;

              /* send all MIDI notes off if the
               * transport loop end point is
               * reached within this cycle */
              if (transport_loop_met && k == 0)
                {
                  time =
                    (midi_time_t)
                      (diff_to_loop_end - 1);
                  send_notes_off_at (
                    region, midi_events, time,
                    TYPE_TRANSPORT_LOOP);
                  g_warn_if_fail (
                    time <
                      local_start_frame + nframes);
                  continue;
                }
              /* if region actually ends on the
               * timeline within this cycle */
              else if (
                region->end_pos.frames !=
                  TRANSPORT->loop_end_pos.frames &&
                /* greater than because we need at
                 * least 1 sample before */
                region->end_pos.frames >
                  g_start_frames &&
                /* +1 because we need at least 1
                 * sample before */
                region->end_pos.frames <=
                  g_end_frames_incl + 1)
                {
                  /* -1 sample to end 1 sample
                   * earlier than the end pos */
                  time =
                    (midi_time_t)
                    ((region_end_frames - 1) -
                      g_start_frames);
                  send_notes_off_at (
                    region, midi_events, time,
                    TYPE_REGION_END);
                  g_warn_if_fail (
                    time <
                    local_start_frame + nframes);
                }
              /* if crossing region loop (local end
               * pos will be back to the loop
               * start) */
              else if (local_end_pos_excl <
                         local_pos)
                {
                  /* diff_from_loop_start will be 0
                   * unless this is the 2nd part of
                   * a split loop (k is 1).
                   * subtract 1 sample to make the
                   * note end 1 sample before its
                   * end point */
                  time =
                    (midi_time_t)
                    (((region_loop_end_adj -
                      local_pos) +
                      diff_to_loop_end) - 1);
                  send_notes_off_at (
                    region, midi_events, time,
                    TYPE_REGION_LOOP_END);
                  g_warn_if_fail (
                    time <
                    local_start_frame + nframes);
                }

              /* readjust position */
              local_pos -= clip_start_frames;
              local_end_pos_excl -=
                clip_start_frames;
              local_end_pos_incl -=
                clip_start_frames;

              for (kk = 0;
                   kk < region->num_midi_notes;
                   kk++)
                {
                  midi_note =
                    region->midi_notes[kk];

                  mn_start_frames =
                    midi_note->start_pos.frames;
                  mn_end_frames =
                    midi_note->end_pos.frames;

                  /* check for note on event in
                   * between a region loop point */
                  if (region_hit_exclusive &&
                      local_pos >
                        local_end_pos_excl &&
                      mn_start_frames <
                        local_end_pos_excl)
                    {
                      time =
                        (midi_time_t)
                        ((region_loop_end_adj -
                          local_pos) +
                          mn_start_frames);
                      midi_events_add_note_on (
                        midi_events,
                        midi_region_get_midi_ch (
                          midi_note->region),
                        midi_note->val,
                        midi_note->vel->vel,
                        time, 1);
                      g_warn_if_fail (
                        time <
                        local_start_frame + nframes);
                    }
                  /* check for note on event in the
                   * normal case */
                  else if (
                    region_hit_exclusive &&
                    midi_note->start_pos.frames >=
                      local_pos &&
                    midi_note->start_pos.frames <
                      local_end_pos_excl)
                    {
                      time =
                        (midi_time_t)
                        ((mn_start_frames -
                           local_pos) +
                         diff_to_loop_end);
                      midi_events_add_note_on (
                        midi_events,
                        midi_region_get_midi_ch (
                          midi_note->region),
                        midi_note->val,
                        midi_note->vel->vel,
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
                      midi_note->end_pos.frames >=
                        local_pos &&
                      /* either note ends before
                       * the cycle end or the region
                       * looped */
                      (midi_note->end_pos.frames <
                         local_end_pos_excl ||
                       local_end_pos_excl <
                         local_pos))
                    {
                      time =
                        (midi_time_t)
                        ((mn_end_frames -
                           local_pos) +
                          diff_to_loop_end);
                      midi_events_add_note_off (
                        midi_events,
                        midi_region_get_midi_ch (
                          midi_note->region),
                        midi_note->val,
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

