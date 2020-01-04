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

#include <math.h>
#include <stdlib.h>

#include "audio/audio_track.h"
#include "audio/clip.h"
#include "audio/automation_tracklist.h"
#include "audio/engine.h"
#include "audio/pool.h"
#include "audio/port.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/math.h"

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
 * Fills the buffers in the given StereoPorts with
 * the frames from the current clip.
 */
void
audio_track_fill_stereo_ports_from_clip (
  AudioTrack *    self,
  StereoPorts *   stereo_ports,
  const long      g_start_frames,
  const nframes_t local_start_frame,
  nframes_t       nframes)
{
  long region_end_frames,
       local_frames_start,
       /*local_frames_end,*/
       loop_start_frames,
       loop_end_frames,
       loop_frames,
       clip_start_frames;
  int i, k;
  ssize_t buff_index;
  unsigned int j;
  long cycle_start_frames =
    g_start_frames;
  long cycle_end_frames =
    cycle_start_frames + (long) nframes;

  if (!TRANSPORT_IS_ROLLING)
    return;

  ZRegion * r;
  ArrangerObject * r_obj;
  TrackLane * lane;
  for (k = 0; k < self->num_lanes; k++)
    {
      lane = self->lanes[k];

      for (i = 0; i < lane->num_regions; i++)
        {
          r = lane->regions[i];
          r_obj = (ArrangerObject *) r;
          if (region_is_hit_by_range (
                r,
                cycle_start_frames,
                cycle_end_frames, 1))
            {
              region_end_frames =
                r_obj->end_pos.frames;
              local_frames_start =
                cycle_start_frames -
                  r_obj->pos.frames;
              /*local_frames_end =*/
                /*local_frames_start + nframes;*/

              loop_start_frames =
                position_to_frames (
                  &r_obj->loop_start_pos);
              loop_end_frames =
                position_to_frames (
                  &r_obj->loop_end_pos);
              loop_frames =
                arranger_object_get_loop_length_in_frames (
                  r_obj);
              clip_start_frames =
                position_to_frames (
                  &r_obj->clip_start_pos);
              local_frames_start +=
                clip_start_frames;
              while (local_frames_start >=
                     loop_end_frames)
                local_frames_start -= loop_frames;

              buff_index = 0;
              AudioClip * clip =
                AUDIO_POOL->clips[r->pool_id];

              /* frames to skip if the region starts
               * somewhere wi thin this cycle */
              unsigned int frames_to_skip = 0;
              if (local_frames_start < 0)
                {
                  frames_to_skip =
                    (unsigned int)
                    (- local_frames_start);
                }

              /* frames to process if the region
               * ends within this cycle */
              long frames_to_process =
                (long) nframes;
              if (cycle_end_frames >=
                    region_end_frames)
                {
                  /* -1 because the region's last
                   * frame is not counted */
                  frames_to_process =
                    (region_end_frames - 1) -
                      cycle_start_frames;
                }
              frames_to_process -=
                (long) frames_to_skip;

              long current_local_frames =
                local_frames_start;
              for (j = frames_to_skip;
                   (long) j < frames_to_process;
                   j++)
                {
                  current_local_frames =
                    local_frames_start + (long) j;

                  /* if loop point hit in the
                   * cycle, go back to loop start */
                  while (
                    current_local_frames >=
                    loop_end_frames)
                    {
                      current_local_frames =
                        (current_local_frames -
                          loop_end_frames) +
                        loop_start_frames;
                    }

                  buff_index =
                    (ssize_t) clip->channels *
                    (ssize_t) current_local_frames;

                  /* make sure we are within the
                   * bounds of the frame array */
                  g_warn_if_fail (
                    buff_index >= 0 &&
                    buff_index +
                      (ssize_t)
                      (clip->channels - 1) <
                        (ssize_t)
                        (clip->channels *
                           r->num_frames));

                  stereo_ports->l->buf[j] =
                    r->frames[buff_index];
                  stereo_ports->r->buf[j] =
                    r->frames[
                      buff_index +
                        (clip->channels - 1)];
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
