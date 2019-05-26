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

#include <math.h>
#include <stdlib.h>

#include "audio/audio_track.h"
#include "audio/automation_tracklist.h"
#include "audio/engine.h"
#include "audio/port.h"
#include "project.h"
#include "utils/arrays.h"

void
audio_track_init (Track * track)
{
  track->type = TRACK_TYPE_AUDIO;
  gdk_rgba_parse (&track->color, "#19664c");
}

void
audio_track_setup (AudioTrack * self)
{
  ChannelTrack * bt = (ChannelTrack *) self;

  channel_track_setup (bt);
}

/**
 * Fills stereo in buffers with info from the current clip.
 */
void
audio_track_fill_stereo_in_buffers (
  AudioTrack *  self,
  StereoPorts * stereo_in)
{
  long region_start_frames,
       region_end_frames,
       local_frames_start,
       loop_end_frames,
       loop_frames,
       clip_start_frames;
  int buff_index, i, j, k;
  long cycle_start_frames =
    position_to_frames (&PLAYHEAD);
  long cycle_end_frames =
    cycle_start_frames + AUDIO_ENGINE->block_length;

  Region * r;
  TrackLane * lane;
  for (k = 0; k < self->num_lanes; k++)
    {
      lane = self->lanes[k];

      for (i = 0; i < lane->num_regions; i++)
        {
          r = lane->regions[i];
          region_start_frames =
            position_to_frames (&r->start_pos);
          region_end_frames =
            position_to_frames (&r->end_pos);
          if (region_start_frames <= cycle_end_frames &&
              region_end_frames >= cycle_start_frames)
            {
              local_frames_start =
                cycle_start_frames - region_start_frames;

              loop_end_frames =
                position_to_frames (&r->loop_end_pos);
              loop_frames =
                region_get_loop_length_in_frames (r);
              clip_start_frames =
                position_to_frames (&r->clip_start_pos);
              local_frames_start += clip_start_frames;
              while (local_frames_start >=
                     loop_end_frames)
                local_frames_start -= loop_frames;

              buff_index = 0;
              int block_length =
                AUDIO_ENGINE->block_length;
              if (r->channels == 1)
                {
                  /*g_message ("1 channel");*/
                  for (j = 0;
                       j < block_length;
                       j++)
                    {
                      buff_index = local_frames_start + j;
                      if (buff_index < 0 ||
                          buff_index >= r->buff_size)
                        continue;
                      stereo_in->l->buf[j] =
                        r->buff[buff_index];
                      stereo_in->r->buf[j] =
                        r->buff[buff_index];
                    }
                }
              else if (r->channels == 2)
                {
                  /*g_message ("2 channels");*/
                  for (j = 0;
                       j < block_length;
                       j += 2)
                    {
                      buff_index = local_frames_start +
                        j;
                      if (buff_index < 0 ||
                          buff_index + 1 >=
                            r->buff_size)
                        continue;
                      stereo_in->l->buf[j] =
                        r->buff[buff_index];
                      stereo_in->r->buf[j] =
                        r->buff[buff_index + 1];
                    }
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
audio_track_free (AudioTrack * track)
{

}
