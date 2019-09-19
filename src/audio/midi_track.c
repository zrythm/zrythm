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
  const long        g_start_frames,
  const nframes_t local_start_frame,
  nframes_t     nframes,
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
       tmp_start, tmp_end,
       mn_start_frames, mn_end_frames,
       region_end_frames;
  /*long transport_loop_end_adjusted;*/
  /*long region_start_frames;*/
  Region * region, * r;
  MidiNote * midi_note;

  g_end_frames =
    transport_frames_add_frames (
      TRANSPORT, g_start_frames, nframes);
  int loop_point_met = g_end_frames < g_start_frames;
  /*int diff_from_loop_start;*/
  unsigned int diff_to_loop_end;

  if (loop_point_met)
    {
      /*diff_from_loop_start =*/
        /*g_end_frames -*/
        /*TRANSPORT->loop_start_pos.frames;*/
      diff_to_loop_end =
        (unsigned int)
        (TRANSPORT->loop_end_pos.frames -
         g_start_frames);
      /*g_message (*/
        /*"loop point met for %s at %ld"*/
        /*"(diff from loop start %d | "*/
        /*"diff to loop end %d | "*/
        /*"loop end point: %ld)",*/
        /*track->name,*/
        /*g_start_frames,*/
        /*diff_from_loop_start,*/
        /*diff_to_loop_end,*/
        /*TRANSPORT->loop_end_pos.frames);*/
    }
  else
    {
      /*diff_from_loop_start = 0;*/
      diff_to_loop_end = 0;
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

              /*region_start_frames =*/
                /*region->start_pos.frames;*/
              region_end_frames =
                region->end_pos.frames;

              /*g_message ("checking region at %ld "*/
                         /*"for %s",*/
                         /*g_start_frames,*/
                         /*track->name);*/

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
                  /*transport_loop_end_adjusted =*/
                    /*TRANSPORT->loop_end_pos.frames -*/
                    /*region_start_frames;*/
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

              /*g_message ("region is hit");*/

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

              /* send all MIDI notes off if the
               * transport loop end point is
               * reached within this cycle */
              if (loop_point_met && k == 0)
                {
                  for (i = 0;
                       i < region->num_midi_notes;
                       i++)
                    {
                      midi_note =
                        region->midi_notes[i];

                      /* FIXME check if note is on
                       * first */
                      /*g_message ("loop met, off at "*/
                        /*"%d",*/
                        /*diff_to_loop_end - 1);*/
                      midi_events_add_note_off (
                        midi_events,
                        midi_region_get_midi_ch (
                          midi_note->region),
                        midi_note->val,
                        (midi_time_t)
                        (diff_to_loop_end - 1),
                        1);
                    }
                }
              /* send all MIDI notes off if end of
               * the region (at loop point or
               * actual end) is within this cycle */
              else if (region_end_adjusted >=
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

                      mn_start_frames =
                        midi_note->start_pos.frames;
                      mn_end_frames =
                        midi_note->end_pos.frames;

                      /* check for note on event on
                       * the boundary */
                      if (mn_start_frames <
                          region_end_adjusted &&
                          mn_end_frames >=
                          region_end_adjusted)
                        {
                          /*g_message (*/
                            /*"end of region within "*/
                            /*"cycle - note off %ld",*/
                            /*region_end_adjusted -*/
                              /*local_pos);*/
                          midi_events_add_note_off (
                            midi_events,
                            midi_region_get_midi_ch (
                              midi_note->region),
                            midi_note->val,
                            (midi_time_t)
                            (region_end_adjusted -
                              local_pos),
                            1);
                        }
                    }
                }
              /* if region actually ends on the
               * timeline within this cycle */
              else if (region_end_frames >=
                         g_start_frames &&
                       /* FIXME should be <=? */
                       region_end_frames <
                         g_end_frames)
                {
                  for (i = 0;
                       i < region->num_midi_notes;
                       i++)
                    {
                      midi_note =
                        region->midi_notes[i];

                      mn_start_frames =
                        midi_note->start_pos.frames;
                      mn_end_frames =
                        midi_note->end_pos.frames;

                      /* add num_loops * loop_ticks to the
                       * midi note start & end poses to get
                       * last pos in region */
                      tmp_start =
                        mn_start_frames;
                      tmp_end =
                        mn_end_frames;
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
                          /*g_message (*/
                            /*"region ends on timeline"*/
                            /*" within cycle - note "*/
                            /*"off %ld",*/
                            /*(r->end_pos.frames - 1) -*/
                              /*g_start_frames);*/
                          midi_events_add_note_off (
                            midi_events,
                            midi_region_get_midi_ch (
                              midi_note->region),
                            midi_note->val,
                            (midi_time_t)
                            ((region_end_frames - 1) -
                              g_start_frames),
                            1);
                        }
                    }
                  continue;
                }
              /* if crossing loop (local end pos
               * will be back to the loop start) */
              else if (local_end_pos <= local_pos)
                {
                  for (i = 0;
                       i < region->num_midi_notes;
                       i++)
                    {
                      midi_note =
                        region->midi_notes[i];

                      mn_start_frames =
                        midi_note->start_pos.frames;
                      mn_end_frames =
                        midi_note->end_pos.frames;

                      /* check for note on event
                       * on the boundary */
                      if (mn_start_frames <
                            loop_end_adjusted &&
                          mn_end_frames >=
                            loop_end_adjusted)
                        {
                          /*g_message (*/
                            /*"crossing loop, off "*/
                            /*"at %ld",*/
                            /*(loop_end_adjusted -*/
                              /*local_pos) +*/
                              /*diff_to_loop_end);*/
                          /* diff_from_loop_start
                           * will be 0 unless this
                           * is the 2nd part of
                           * a split loop (k is 1)
                           */
                          midi_events_add_note_off (
                            midi_events,
                            midi_region_get_midi_ch (
                              midi_note->region),
                            midi_note->val,
                            (midi_time_t)
                            ((loop_end_adjusted -
                              local_pos) +
                              diff_to_loop_end),
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

                  mn_start_frames =
                    midi_note->start_pos.frames;
                  mn_end_frames =
                    midi_note->end_pos.frames;

                  /*g_message ("checking note "*/
                             /*"(local pos %ld | "*/
                             /*"local end pos %ld)",*/
                             /*local_pos,*/
                             /*local_end_pos);*/

                  /* check for note on event in
                   * between a region loop point */
                  if (local_pos > local_end_pos &&
                      mn_start_frames <
                        local_end_pos)
                    {
                      /*g_message ("on at %ld",*/
                        /*(loop_end_adjusted -*/
                          /*local_pos) +*/
                        /*midi_note->start_pos.frames);*/
                      midi_events_add_note_on (
                        midi_events,
                        midi_region_get_midi_ch (
                          midi_note->region),
                        midi_note->val,
                        midi_note->vel->vel,
                        (midi_time_t)
                        ((loop_end_adjusted -
                          local_pos) +
                          mn_start_frames),
                        1);
                    }
                  /* check for note on event in the
                   * normal case */
                  else if (midi_note->start_pos.
                        frames >=
                        local_pos &&
                      midi_note->start_pos.frames <
                        local_end_pos)
                    {
                      /*g_message ("on at %ld",*/
                        /*(midi_note->start_pos.*/
                           /*frames -*/
                         /*local_pos) +*/
                        /*diff_to_loop_end);*/
                      midi_events_add_note_on (
                        midi_events,
                        midi_region_get_midi_ch (
                          midi_note->region),
                        midi_note->val,
                        midi_note->vel->vel,
                        (midi_time_t)
                        ((mn_start_frames -
                           local_pos) +
                         diff_to_loop_end),
                        1);
                    }

                  /* note off event */
                  if (midi_note->end_pos.frames >=
                        local_pos &&
                      midi_note->end_pos.frames <
                        local_end_pos)
                    {
                      /*g_message ("off at %ld "*/
                        /*"(local pos %ld | "*/
                        /*"local end pos %ld | "*/
                        /*"midi note end pos %ld)",*/
                        /*(midi_note->end_pos.*/
                           /*frames -*/
                         /*local_pos) +*/
                        /*diff_to_loop_end,*/
                        /*local_pos,*/
                        /*local_end_pos,*/
                        /*midi_note->end_pos.frames);*/
                      midi_events_add_note_off (
                        midi_events,
                        midi_region_get_midi_ch (
                          midi_note->region),
                        midi_note->val,
                        (midi_time_t)
                        ((mn_end_frames -
                           local_pos) +
                          diff_to_loop_end),
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
midi_track_free (Track * track)
{
}

