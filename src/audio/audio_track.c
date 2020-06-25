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

#include <math.h>
#include <stdlib.h>

#include "audio/audio_track.h"
#include "audio/automation_tracklist.h"
#include "audio/clip.h"
#include "audio/engine.h"
#include "audio/fade.h"
#include "audio/pool.h"
#include "audio/port.h"
#include "audio/stretcher.h"
#include "audio/tempo_track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/math.h"
#include "zrythm_app.h"

void
audio_track_init (
  Track * self)
{
  self->type = TRACK_TYPE_AUDIO;
  gdk_rgba_parse (&self->color, "#19664c");

  self->rt_stretcher =
    stretcher_new_rubberband (
      AUDIO_ENGINE->sample_rate, 2, 1.0,
      1.0, true);
}

void
audio_track_setup (Track * self)
{
  channel_track_setup (self);
}

static void
timestretch_buf (
  Track *      self,
  ZRegion *    r,
  AudioClip *  clip,
  size_t      in_frame_offset,
  double       timestretch_ratio,
  float *      lbuf_after_ts,
  float *      rbuf_after_ts,
  unsigned int out_frame_offset,
  ssize_t      frames_to_process)
{
  g_return_if_fail (
    r && self->rt_stretcher);
  stretcher_set_time_ratio (
    self->rt_stretcher, 1.0 / timestretch_ratio);
  size_t in_frames_to_process =
    (size_t)
    (frames_to_process * timestretch_ratio);
  g_message (
    "%s: in frame offset %zd, out frame offset %u, "
    "in frames to process %zu, "
    "out frames to process %zd",
    __func__, in_frame_offset, out_frame_offset,
    in_frames_to_process, frames_to_process);
  g_return_if_fail (
    in_frame_offset + in_frames_to_process <=
      r->num_frames);
  ssize_t retrieved =
    stretcher_stretch (
      self->rt_stretcher,
      &r->ch_frames[0][in_frame_offset],
      clip->channels == 1 ?
        &r->ch_frames[0][in_frame_offset] :
        &r->ch_frames[1][in_frame_offset],
      in_frames_to_process,
      &lbuf_after_ts[out_frame_offset],
      &rbuf_after_ts[out_frame_offset],
      (size_t) frames_to_process);
  g_warn_if_fail (retrieved == frames_to_process);
}

/**
 * Fills the buffers in the given StereoPorts with
 * the frames from the current clip.
 */
void
audio_track_fill_stereo_ports_from_clip (
  Track *         self,
  StereoPorts *   stereo_ports,
  const long      g_start_frames,
  const nframes_t local_start_frame,
  nframes_t       nframes)
{
  g_return_if_fail (IS_TRACK (self) && stereo_ports);

  long region_end_frames,
       frames_start_from_cycle_start,
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

          g_return_if_fail (IS_REGION (r));

          if (r_obj->muted)
            continue;

          /* skip if in bounce mode and the
           * region should not be bounced */
          if (AUDIO_ENGINE->bounce_mode !=
                BOUNCE_OFF &&
              (!r->bounce || !self->bounce))
            continue;

          /* skip if region is currently being
           * stretched FIXME what if stretching
           * starts inside this - need a semaphore */
          if (r->stretching)
            continue;

          if (region_is_hit_by_range (
                r,
                cycle_start_frames,
                cycle_end_frames, 1))
            {
              region_end_frames =
                r_obj->end_pos.frames;
              frames_start_from_cycle_start =
                cycle_start_frames -
                  r_obj->pos.frames;
              /*local_frames_end =*/
                /*frames_start_from_cycle_start + nframes;*/

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
              long frames_start_from_cycle_start_adj =
                frames_start_from_cycle_start +
                clip_start_frames;
              while (frames_start_from_cycle_start_adj >=
                     loop_end_frames)
                {
                  frames_start_from_cycle_start_adj -=
                    loop_frames;
                }

              buff_index = 0;
              AudioClip * clip =
                AUDIO_POOL->clips[r->pool_id];

              /* frames to skip if the region starts
               * somewhere within this cycle */
              unsigned int frames_to_skip = 0;
              if (frames_start_from_cycle_start_adj < 0)
                {
                  frames_to_skip =
                    (unsigned int)
                    (- frames_start_from_cycle_start_adj);
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
                frames_start_from_cycle_start_adj;

              /* restretch if necessary */
              Position g_start_pos;
              position_from_frames (
                &g_start_pos, g_start_frames);
              bpm_t cur_bpm =
                tempo_track_get_bpm_at_pos (
                  P_TEMPO_TRACK, &g_start_pos);
              double timestretch_ratio = 1.0;
              bool needs_rt_timestretch = false;
              if (region_get_musical_mode (r) &&
                  !math_floats_equal (
                    clip->bpm, cur_bpm))
                {
                  needs_rt_timestretch = true;
                  timestretch_ratio =
                    (double) cur_bpm /
                    (double) clip->bpm;
                  g_message (
                    "timestretching: "
                    "(cur bpm %f clip bpm %f) %f",
                    (double) cur_bpm,
                    (double) clip->bpm,
                    timestretch_ratio);
                }

              if (frames_to_process < 1)
                {
                  /*g_message (*/
                    /*"%s: %s:%s "*/
                    /*"no frames to process,"*/
                    /*"skipping...",*/
                    /*__func__, self->name, r->name);*/
                  continue;
                }

              /* buffers after timestretch */
              float lbuf_after_ts[frames_to_process];
              float rbuf_after_ts[frames_to_process];
              memset (
                lbuf_after_ts, 0,
                (size_t) frames_to_process *
                  sizeof (float));
              memset (
                rbuf_after_ts, 0,
                (size_t) frames_to_process *
                  sizeof (float));
              unsigned int prev_j_offset =
                frames_to_skip;

              size_t buff_index_start =
                r->num_frames + 16;
              size_t buff_size = 0;
              for (j = frames_to_skip;
                   (long) j < frames_to_process;
                   j++)
                {
                  current_local_frames =
                    frames_start_from_cycle_start_adj +
                    (long) j;

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
                    (ssize_t) current_local_frames;

#define STRETCH \
  timestretch_buf ( \
    self, r, clip, buff_index_start, \
    timestretch_ratio, \
    lbuf_after_ts, rbuf_after_ts, \
    prev_j_offset, \
    (j - prev_j_offset) + 1)

                  /* if we are starting at a new
                   * point in the audio clip */
                  if (needs_rt_timestretch)
                    {
                      buff_index =
                        (ssize_t)
                        (buff_index *
                         timestretch_ratio);
                      if (buff_index <
                            (ssize_t)
                            buff_index_start)
                        {
                          g_message (
                            "buff index (%zd) < buff index start (%zd)",
                            buff_index,
                            buff_index_start);
                          /* set the start point (
                           * used when
                           * timestretching) */
                          buff_index_start =
                            (size_t) buff_index;

                          /* timestretch the material
                           * up to this point */
                          if (buff_size > 0)
                            {
                              g_message (
                                "buff size (%zd) > 0",
                                buff_size);
                              g_message ("j %u", j);
                              STRETCH;
                              prev_j_offset = j;
                            }
                          buff_size = 0;
                        }
                      else if ((long) j ==
                                 frames_to_process - 1)
                        {
                          g_message ("last sample");
                          g_message ("j %u", j);
                          STRETCH;
                          prev_j_offset = j;
                        }
                      else
                        {
                          buff_size++;
                          /*g_message ("buff index end %zu", buff_index_end);*/
                        }
                    }

                  /* make sure we are within the
                   * bounds of the frame array */
                  /*g_warn_if_fail (*/
                    /*buff_index >= 0 &&*/
                    /*buff_index +*/
                      /*(ssize_t)*/
                      /*(clip->channels - 1) <*/
                        /*(ssize_t)*/
                        /*(clip->channels **/
                           /*r->num_frames));*/

                  if (!needs_rt_timestretch)
                    {
                      lbuf_after_ts[j] =
                        r->ch_frames[0][buff_index];
                      rbuf_after_ts[j] =
                        clip->channels == 1 ?
                        r->ch_frames[0][buff_index] :
                        r->ch_frames[1][buff_index];
                    }
                }

              /* apply fades */
              for (j = frames_to_skip;
                   (long) j < frames_to_process;
                   j++)
                {
                  long local_frames =
                    frames_start_from_cycle_start + j;
                  float fade_in = 1.f;
                  float fade_out = 1.f;
                  if (local_frames >= 0 &&
                      local_frames <
                        r_obj->fade_in_pos.frames)
                    {
                      fade_in =
                        (float)
                        fade_get_y_normalized (
                          (double) local_frames /
                          (double)
                          r_obj->fade_in_pos.frames,
                          &r_obj->fade_in_opts, 1);
                    }
                  else if (local_frames >=
                             r_obj->fade_out_pos.
                               frames)
                    {
                      fade_out =
                        (float)
                        fade_get_y_normalized (
                          (double)
                          (local_frames -
                              r_obj->fade_out_pos.
                                frames) /
                          (double)
                          (r_obj->end_pos.frames -
                            (r_obj->fade_out_pos.
                               frames +
                             r_obj->pos.frames)),
                          &r_obj->fade_out_opts, 0);
                    }

                  stereo_ports->l->buf[j] =
                    lbuf_after_ts[j] *
                    fade_in * fade_out;
                  stereo_ports->r->buf[j] =
                    rbuf_after_ts[j] *
                    fade_in * fade_out;
                }
            }
        }
    }
}
