/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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
#include "gui/widgets/automation_lane.h"
#include "utils/arrays.h"

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
 * Fills MIDI events from track.
 *
 * NOTE: real time func
 */
void
instrument_track_fill_midi_events (
  InstrumentTrack      * track,
  Position   * pos, ///< start position to check
  uint32_t  nframes, ///< n of frames from start po
  MidiEvents * midi_events) ///< midi events to fill
{
  int i, j;
  Position end_pos;
  Position local_pos, local_end_pos;
  Position loop_start_adjusted,
           loop_end_adjusted,
           region_end_adjusted;
#ifdef HAVE_JACK
  jack_midi_event_t * ev;
#endif
  Region * region, * r;
  MidiNote * midi_note;
  position_set_to_pos (&end_pos, pos);
  position_add_frames (&end_pos, nframes);

  midi_events->queue->num_events = 0;
  TrackLane * lane;
  for (j = 0; j < track->num_lanes; j++)
    {
      lane = track->lanes[j];

      for (i = 0; i < lane->num_regions; i++)
        {
          region = lane->regions[i];
          r = (Region *) region;

          if (!region_is_hit (r,
                              pos))
            continue;

          /* get local positions */
          region_timeline_pos_to_local (
            r, pos, &local_pos, 1);
          region_timeline_pos_to_local (
            r, &end_pos, &local_end_pos, 1);

          /* add clip_start position to start from
           * there */
          long clip_start_ticks =
            position_to_ticks (
              &r->clip_start_pos);
          long region_length_ticks =
            region_get_full_length_in_ticks (
              r);
          long loop_length_ticks =
            region_get_loop_length_in_ticks (r);
          position_set_to_pos (
            &loop_start_adjusted,
            &r->loop_start_pos);
          position_set_to_pos (
            &loop_end_adjusted,
            &r->loop_end_pos);
          position_init (&region_end_adjusted);
          position_add_ticks (
            &local_pos, clip_start_ticks);
          position_add_ticks (
            &local_end_pos, clip_start_ticks);
          position_add_ticks (
            &loop_start_adjusted, - clip_start_ticks);
          position_add_ticks (
            &loop_end_adjusted, - clip_start_ticks);
          position_add_ticks (
            &region_end_adjusted, region_length_ticks);

          /* send all MIDI notes off if end if the
           * region (at loop point or actual end) is
           * within this cycle */
          if (position_compare (
                &region_end_adjusted,
                &local_pos) >= 0 &&
              position_compare (
                &region_end_adjusted,
                &local_end_pos) <= 0)
            {
              for (i = 0;
                   i < region->num_midi_notes;
                   i++)
                {
                  midi_note =
                    region->midi_notes[i];

                  /* check for note on event on the
                   * boundary */
                  if (position_compare (
                        &midi_note->start_pos,
                        &region_end_adjusted) < 0 &&
                      position_compare (
                        &midi_note->end_pos,
                        &region_end_adjusted) >= 0)
                    {
#ifdef HAVE_JACK
                      ev =
                        &midi_events->queue->
                          jack_midi_events[
                            midi_events->queue->
                              num_events++];
                      ev->time =
                        position_to_frames (
                          &region_end_adjusted) -
                        position_to_frames (&local_pos);
                      ev->size = 3;
                      /* status byte */
                      ev->buffer[0] =
                        MIDI_CH1_NOTE_OFF;
                      /* note number */
                      ev->buffer[1] =
                        midi_note->val;
                      /* velocity */
                      ev->buffer[2] =
                        midi_note->vel->vel;
#endif
                    }
                }
            }
          /* if region actually ends on the timeline
           * within this cycle */
          else if (position_compare (
                    &r->end_pos,
                    pos) >= 0 &&
                   position_compare (
                    &r->end_pos,
                    &end_pos) <= 0)
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
                  Position tmp_start, tmp_end;
                  position_set_to_pos (
                    &tmp_start,
                    &midi_note->start_pos);
                  position_set_to_pos (
                    &tmp_end,
                    &midi_note->end_pos);
                  int num_loops =
                    region_get_num_loops (r, 0);
                  for (int jj = 0;
                       jj < num_loops;
                       jj++)
                    {
                      position_add_ticks (
                        &tmp_start,
                        loop_length_ticks);
                      position_add_ticks (
                        &tmp_end,
                        loop_length_ticks);
                    }

                  /* check for note on event on the
                   * boundary */
                  if (position_compare (
                        &tmp_start,
                        &region_end_adjusted) < 0 &&
                      position_compare (
                        &tmp_end,
                        &region_end_adjusted) >= 0)
                    {
#ifdef HAVE_JACK
                      ev =
                        &midi_events->queue->
                          jack_midi_events[
                            midi_events->queue->
                              num_events++];
                       ev->time =
                         position_to_frames (
                           &r->end_pos) -
                         position_to_frames (pos);
                      ev->size = 3;
                      /* status byte */
                      ev->buffer[0] =
                        MIDI_CH1_NOTE_OFF;
                      /* note number */
                      ev->buffer[1] =
                        midi_note->val;
                      /* velocity */
                      ev->buffer[2] =
                        midi_note->vel->vel;
#endif
                    }
                }
            }

          /* if crossing loop
           * (local end pos will be back to the loop
           * start) */
          if (position_compare (
                &local_end_pos,
                &local_pos) <= 0)
            {
              for (i = 0;
                   i < region->num_midi_notes;
                   i++)
                {
                  midi_note =
                    region->midi_notes[i];

                  /* check for note on event on the
                   * boundary */
                  if (position_compare (
                        &midi_note->start_pos,
                        &loop_end_adjusted) < 0 &&
                      position_compare (
                        &midi_note->end_pos,
                        &loop_end_adjusted) >= 0)
                    {
#ifdef HAVE_JACK
                      ev =
                        &midi_events->queue->
                          jack_midi_events[
                            midi_events->queue->
                              num_events++];
                      ev->time =
                        position_to_frames (
                          &loop_end_adjusted) -
                        position_to_frames (&local_pos);
                      ev->size = 3;
                      /* status byte */
                      ev->buffer[0] =
                        MIDI_CH1_NOTE_OFF;
                      /* note number */
                      ev->buffer[1] =
                        midi_note->val;
                      /* velocity */
                      ev->buffer[2] =
                        midi_note->vel->vel;
#endif
                    }
                }
            }

          /* readjust position */
          position_add_ticks (
            &local_pos, - clip_start_ticks);
          position_add_ticks (
            &local_end_pos, - clip_start_ticks);

          for (i = 0;
               i < region->num_midi_notes;
               i++)
            {
              midi_note =
                region->midi_notes[i];

              /* check for note on event */
              if (position_compare (
                    &midi_note->start_pos,
                    &local_pos) >= 0 &&
                  position_compare (
                    &midi_note->start_pos,
                    &local_end_pos) <= 0)
                {
#ifdef HAVE_JACK
                  ev =
                    &midi_events->queue->
                      jack_midi_events[
                        midi_events->queue->
                          num_events++];
                  ev->time =
                    position_to_frames (
                      &midi_note->start_pos) -
                    position_to_frames (&local_pos);
                  ev->size = 3;
                  /* status byte */
                  ev->buffer[0] =
                    MIDI_CH1_NOTE_ON;
                  /* note number */
                  ev->buffer[1] =
                    midi_note->val;
                  /* velocity */
                  ev->buffer[2] =
                    midi_note->vel->vel;
#endif
                }

              /* note off event */
              if (position_compare (
                    &midi_note->end_pos,
                    &local_pos) >= 0 &&
                  position_compare (
                    &midi_note->end_pos,
                    &local_end_pos) <= 0)
                {
#ifdef HAVE_JACK
                  ev =
                    &midi_events->queue->
                      jack_midi_events[
                        midi_events->queue->
                          num_events++];
                  ev->time =
                    position_to_frames (
                      &midi_note->end_pos) -
                    position_to_frames (&local_pos);
                  ev->size = 3;
                  /* status byte */
                  ev->buffer[0] = MIDI_CH1_NOTE_OFF;
                  /* note number 0-127 */
                  ev->buffer[1] = midi_note->val;
                  /* velocity 0-127 */
                  ev->buffer[2] = midi_note->vel->vel;
#endif
                }
            }
        }
    }
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
