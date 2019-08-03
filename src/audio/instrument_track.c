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
#include "audio/instrument_track.h"
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
 * Initializes an instrument track.
 */
void
instrument_track_init (Track * track)
{
  track->type = TRACK_TYPE_INSTRUMENT;
  gdk_rgba_parse (&track->color, "#F79616");
}

void
instrument_track_setup (InstrumentTrack * self)
{
  channel_track_setup (self);
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
instrument_track_fill_midi_events (
  InstrumentTrack * track,
  const long        g_start_frames,
  const int         local_start_frame,
  uint32_t          nframes,
  MidiEvents *      midi_events)
{
  int i, j, k, jj, num_loops;
  long g_end_frames;
  long local_pos, local_end_pos;
  long loop_start_adjusted,
       loop_end_adjusted,
       region_end_adjusted,
       clip_start_frames,
       region_length_frames,
       loop_length_frames,
       tmp_start, tmp_end;
  Region * region, * r;
  MidiNote * midi_note;

  g_end_frames =
    transport_frames_add_frames (
      TRANSPORT, g_start_frames, nframes);
  int loop_point_met = g_end_frames < g_start_frames;
  int diff_from_loop_start;

  if (loop_point_met)
    {
      diff_from_loop_start =
        g_end_frames -
        TRANSPORT->loop_start_pos.frames;
      g_message (
        "loop point met for %s "
        "(diff from loop start %d)",
        track->name,
        diff_from_loop_start);
    }
  else
    {
      diff_from_loop_start = 0;
    }

  zix_sem_wait (&midi_events->access_sem);

  TrackLane * lane;

  /* loop once if no loop met, twice if loop met
   * (split the  ranges in 2) */
  for (k = 0; k < loop_point_met + 1; k++)
    {
      for (j = 0; j < track->num_lanes; j++)
        {
          lane = track->lanes[j];

          for (i = 0; i < lane->num_regions; i++)
            {
              region = lane->regions[i];
              r = (Region *) region;

              g_message ("checking region at");

              num_loops =
                region_get_num_loops (r, 0);

              if (loop_point_met)
                {
                  /* check first half */
                  if (k == 0)
                    {
                      if (!region_is_hit_by_range (
                            r, g_start_frames,
                            TRANSPORT->
                              loop_end_pos.frames))
                        continue;
                    }
                  /* check second half */
                  else if (k == 1)
                    {
                      if (!region_is_hit_by_range (
                            r,
                            TRANSPORT->
                              loop_start_pos.frames,
                            g_end_frames))
                        continue;
                    }
                }
              else
                {
                  if (!region_is_hit_by_range (
                        r, g_start_frames,
                        g_end_frames))
                    continue;
                }

              /* get local positions */
              if (loop_point_met && k == 0)
                {
                  local_pos =
                    region_timeline_frames_to_local (
                      r, g_start_frames, 1);
                  local_end_pos =
                    region_timeline_frames_to_local (
                      r,
                      TRANSPORT->loop_end_pos.frames,
                      1);
                }
              else if (loop_point_met && k == 1)
                {
                  local_pos =
                    region_timeline_frames_to_local (
                      r,
                      TRANSPORT->loop_start_pos.
                        frames, 1);
                  local_end_pos =
                    region_timeline_frames_to_local (
                      r, g_end_frames, 1);
                }
              else
                {
                  local_pos =
                    region_timeline_frames_to_local (
                      r, g_start_frames, 1);
                  local_end_pos =
                    region_timeline_frames_to_local (
                      r, g_end_frames, 1);
                }

              g_message ("region is hit");

              /* add clip_start position to start
               * from there */
              clip_start_frames =
                r->clip_start_pos.frames;
              region_length_frames =
                region_get_full_length_in_frames (
                  r);
              loop_length_frames =
                region_get_loop_length_in_frames (r);
              loop_start_adjusted =
                r->loop_start_pos.frames;
              loop_end_adjusted =
                r->loop_end_pos.frames;
              local_pos += clip_start_frames;
              local_end_pos += clip_start_frames;
              loop_start_adjusted -=
                clip_start_frames;
              loop_end_adjusted -=
                clip_start_frames;
              region_end_adjusted =
                region_length_frames;

              /* send all MIDI notes off if end of
               * the region (at loop point or
               * actual end) is within this cycle */
              if (region_end_adjusted >=
                    local_pos &&
                  region_end_adjusted <=
                    local_end_pos)
                {
                  for (i = 0;
                       i < region->num_midi_notes;
                       i++)
                    {
                      midi_note =
                        region->midi_notes[i];

                      /* check for note on event on
                       * the boundary */
                      if (midi_note->start_pos.
                            frames <
                          region_end_adjusted &&
                          midi_note->end_pos.
                            frames >=
                          region_end_adjusted)
                        {
                          midi_events_add_note_off (
                            midi_events,
                            1, midi_note->val,
                            region_end_adjusted -
                              local_pos,
                            1);
                        }
                    }
                }
              /* if region actually ends on the
               * timeline within this cycle */
              else if (r->end_pos.frames >=
                         g_start_frames &&
                       /* FIXME should be <=? */
                       r->end_pos.frames <
                         g_end_frames)
                {
                  for (i = 0;
                       i < region->num_midi_notes;
                       i++)
                    {
                      midi_note =
                        region->midi_notes[i];

                      /* add num_loops * loop_ticks to the
                       * midi note start & end poses to get
                       * last pos in region */
                      tmp_start =
                        midi_note->start_pos.frames;
                      tmp_end =
                        midi_note->end_pos.frames;
                      for (jj = 0;
                           jj < num_loops;
                           jj++)
                        {
                          tmp_start +=
                            loop_length_frames;
                          tmp_end +=
                            loop_length_frames;
                        }

                      /* check for note on event on the
                       * boundary */
                      if (tmp_start <
                            region_end_adjusted &&
                          tmp_end >=
                            region_end_adjusted)
                        {
                          midi_events_add_note_off (
                            midi_events, 1,
                            midi_note->val,
                            r->end_pos.frames -
                              g_start_frames,
                            1);
                        }
                    }
                }

              /* if crossing loop (local end pos
               * will be back to the loop start) */
              if (local_end_pos <= local_pos)
                {
                  for (i = 0;
                       i < region->num_midi_notes;
                       i++)
                    {
                      midi_note =
                        region->midi_notes[i];

                      /* check for note on event
                       * on the boundary */
                      if (midi_note->start_pos.
                            frames <
                            loop_end_adjusted &&
                          midi_note->end_pos.
                            frames >=
                            loop_end_adjusted)
                        {
                          /* diff_from_loop_start
                           * will be 0 unless this
                           * is the 2nd part of
                           * a split loop (k is 1)
                           */
                          midi_events_add_note_off (
                            midi_events, 1,
                            midi_note->val,
                            (loop_end_adjusted -
                              local_pos) +
                              diff_from_loop_start,
                            1);
                        }
                    }
                }

              /* readjust position */
              local_pos -= clip_start_frames;
              local_end_pos -= clip_start_frames;

              for (i = 0;
                   i < region->num_midi_notes;
                   i++)
                {
                  midi_note =
                    region->midi_notes[i];

                  g_message ("checking note");

                  /* check for note on event */
                  if (midi_note->start_pos.
                        frames >=
                        local_pos &&
                      midi_note->start_pos.frames <=
                        local_end_pos)
                    {
                      g_message ("on at %ld",
                        (midi_note->start_pos.
                           frames -
                         local_pos) +
                        diff_from_loop_start);
                      midi_events_add_note_on (
                        midi_events, 1,
                        midi_note->val,
                        midi_note->vel->vel,
                        (midi_note->start_pos.
                           frames -
                           local_pos) +
                           diff_from_loop_start,
                        1);
                    }

                  /* note off event */
                  if (midi_note->end_pos.frames >=
                        local_pos &&
                      midi_note->end_pos.frames <=
                        local_end_pos)
                    {
                      g_message ("off");
                      midi_events_add_note_off (
                        midi_events, 1,
                        midi_note->val,
                        /*start_frame +*/
                        (midi_note->end_pos.frames -
                           local_pos) +
                          diff_from_loop_start,
                        1);
                    }
                }
            }
        }
    }

  zix_sem_post (&midi_events->access_sem);
}

/**
 * Frees the track.
 *
 * TODO
 */
void
instrument_track_free (InstrumentTrack * track)
{
}
