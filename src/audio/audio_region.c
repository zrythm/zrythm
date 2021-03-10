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

#include "audio/channel.h"
#include "audio/audio_region.h"
#include "audio/clip.h"
#include "audio/fade.h"
#include "audio/pool.h"
#include "audio/stretcher.h"
#include "audio/tempo_track.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

/**
 * Creates a ZRegion for audio data.
 *
 * FIXME First create the
 * audio on the pool and then pass the pool id here.
 *
 * @param pool_id The pool ID. This is used when
 *   creating clone regions (non-main) and must be
 *   -1 when creating a new clip.
 * @param filename Filename, if loading from
 *   file, otherwise NULL.
 * @param frames Float array, if loading from
 *   float array, otherwise NULL.
 * @param nframes Number of frames per channel.
 * @param clip_name Name of audio clip, if not
 *   loading from file.
 */
ZRegion *
audio_region_new (
  const int        pool_id,
  const char *     filename,
  const float *    frames,
  const long       nframes,
  const char *     clip_name,
  const channels_t channels,
  const Position * start_pos,
  int              track_pos,
  int              lane_pos,
  int              idx_inside_lane)
{
  ZRegion * self = object_new (ZRegion);
  ArrangerObject * obj =
    (ArrangerObject *) self;

  g_return_val_if_fail (
    start_pos && start_pos->frames >= 0, NULL);

  self->id.type = REGION_TYPE_AUDIO;
  self->pool_id = -1;

  AudioClip * clip = NULL;
  int recording = 0;
  if (pool_id == -1)
    {
      if (filename)
        {
          clip =
            audio_clip_new_from_file (filename);
        }
      else if (frames)
        {
          g_return_val_if_fail (clip_name, NULL);
          clip =
            audio_clip_new_from_float_array (
              frames, nframes, channels, clip_name);
        }
      else
        {
          clip =
            audio_clip_new_recording (
              2, nframes, clip_name);
          recording = 1;
        }
      g_return_val_if_fail (clip, NULL);

      self->pool_id =
        audio_pool_add_clip (
          AUDIO_POOL, clip);
      g_warn_if_fail (self->pool_id > -1);
    }
  else
    {
      self->pool_id = pool_id;
      clip = AUDIO_POOL->clips[pool_id];
      g_warn_if_fail (clip && clip->frames);
    }

  /* set end pos to sample end */
  position_set_to_pos (
    &obj->end_pos, start_pos);
  position_add_frames (
    &obj->end_pos, clip->num_frames);

  /* init split points */
  self->split_points_size = 1;
  self->split_points =
    object_new_n (
      self->split_points_size, Position);
  self->num_split_points = 0;

  /* init APs */
  self->aps_size = 2;
  self->aps =
    object_new_n (
      self->aps_size, AutomationPoint *);

  /* init */
  region_init (
    self, start_pos, &obj->end_pos, track_pos,
    lane_pos, idx_inside_lane);

  (void) recording;
  g_warn_if_fail (audio_region_get_clip (self));
  /*if (!recording)*/
    /*audio_clip_write_to_pool (clip);*/

  return self;
}

/**
 * Returns the audio clip associated with the
 * Region.
 */
AudioClip *
audio_region_get_clip (
  const ZRegion * self)
{
  g_return_val_if_fail (
    self->pool_id >= 0 &&
    self->id.type == REGION_TYPE_AUDIO,
    NULL);

  AudioClip * clip =
    AUDIO_POOL->clips[self->pool_id];

  g_return_val_if_fail (
    clip && clip->frames && clip->num_frames > 0,
    NULL);

  return clip;
}

/**
 * Sets the clip ID on the region and updates any
 * references.
 */
void
audio_region_set_clip_id (
  ZRegion * self,
  int       clip_id)
{
  self->pool_id = clip_id;

  /* TODO update identifier - needed? */
}

/**
 * Replaces the region's frames from \ref
 * start_frames with \ref frames.
 *
 * @param duplicate_clip Whether to duplicate the
 *   clip (eg, when other regions refer to it).
 * @param frames Frames, interleaved.
 */
void
audio_region_replace_frames (
  ZRegion * self,
  float *   frames,
  size_t    start_frame,
  size_t    num_frames,
  bool      duplicate_clip)
{
  AudioClip * clip = audio_region_get_clip (self);
  g_return_if_fail (clip);

  if (duplicate_clip)
    {
      g_warn_if_reached ();

      /* TODO delete */
      int prev_id = clip->pool_id;
      int id =
        audio_pool_duplicate_clip (
          AUDIO_POOL, clip->pool_id,
          F_NO_WRITE_FILE);
      g_return_if_fail (id != prev_id);
      clip = audio_pool_get_clip (AUDIO_POOL, id);
      g_return_if_fail (clip);

      self->pool_id = clip->pool_id;
    }

  dsp_copy (
    &clip->frames[start_frame * clip->channels],
    frames, num_frames * clip->channels);

  audio_clip_write_to_pool (clip, false);
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
    (long)
    (in_frame_offset + in_frames_to_process) <=
      clip->num_frames);
  ssize_t retrieved =
    stretcher_stretch (
      self->rt_stretcher,
      &clip->ch_frames[0][in_frame_offset],
      clip->channels == 1 ?
        &clip->ch_frames[0][in_frame_offset] :
        &clip->ch_frames[1][in_frame_offset],
      in_frames_to_process,
      &lbuf_after_ts[out_frame_offset],
      &rbuf_after_ts[out_frame_offset],
      (size_t) frames_to_process);
  g_warn_if_fail (retrieved == frames_to_process);
}

/**
 * Fills audio data from the region.
 *
 * @note The caller already splits calls to this
 *   function at each sub-loop inside the region,
 *   so region loop related logic is not needed.
 *
 * @param g_start_frames Global start frame.
 * @param local_start_frame The start frame offset
 *   from 0 in this cycle.
 * @param nframes Number of frames at start
 *   Position.
 * @param stereo_ports StereoPorts to fill.
 */
REALTIME
void
audio_region_fill_stereo_ports (
  ZRegion *     r,
  long          g_start_frames,
  nframes_t     local_start_frame,
  nframes_t     nframes,
  StereoPorts * stereo_ports)
{
  ArrangerObject * r_obj =  (ArrangerObject *) r;
  AudioClip * clip =
    audio_pool_get_clip (AUDIO_POOL, r->pool_id);
  Track * track =
    arranger_object_get_track (r_obj);

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
        (double) cur_bpm / (double) clip->bpm;
      g_message (
        "timestretching: "
        "(cur bpm %f clip bpm %f) %f",
        (double) cur_bpm, (double) clip->bpm,
        timestretch_ratio);
    }

  /* buffers after timestretch */
  float lbuf_after_ts[nframes];
  float rbuf_after_ts[nframes];
  dsp_fill (lbuf_after_ts, 0, nframes);
  dsp_fill (rbuf_after_ts, 0, nframes);

  long r_local_pos_at_start =
    region_timeline_frames_to_local (
      r, g_start_frames, F_NORMALIZE);

  size_t buff_index_start =
    (size_t) clip->num_frames + 16;
  size_t buff_size = 0;
  unsigned int prev_offset = local_start_frame;
  for (nframes_t j =
         (r_local_pos_at_start < 0) ?
           - r_local_pos_at_start : 0;
       j < nframes; j++)
    {
      long current_local_frame =
        local_start_frame + j;
      long r_local_pos =
        region_timeline_frames_to_local (
          r, g_start_frames + j, F_NORMALIZE);
      if (r_local_pos < 0 ||
          j > AUDIO_ENGINE->block_length)
        {
          g_critical (
            "invalid r_local_pos %ld, j %u, "
            "g_start_frames %ld, nframes %u",
            r_local_pos, j, g_start_frames, nframes);
          return;
        }

      ssize_t buff_index =
        (ssize_t) r_local_pos;

#define STRETCH \
timestretch_buf ( \
track, r, clip, buff_index_start, \
timestretch_ratio, \
lbuf_after_ts, rbuf_after_ts, \
prev_offset, \
(current_local_frame - prev_offset) + 1)

      /* if we are starting at a new
       * point in the audio clip */
      if (needs_rt_timestretch)
        {
          buff_index =
            (ssize_t)
            (buff_index * timestretch_ratio);
          if (buff_index <
                (ssize_t) buff_index_start)
            {
              g_message (
                "buff index (%zd) < "
                "buff index start (%zd)",
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
                  STRETCH;
                  prev_offset = current_local_frame;
                }
              buff_size = 0;
            }
          /* else if last sample */
          else if (j == (nframes - 1))
            {
              STRETCH;
              prev_offset = current_local_frame;
            }
          else
            {
              buff_size++;
            }
        }
      /* else if no need for timestretch */
      else if (!needs_rt_timestretch)
        {
          lbuf_after_ts[j] =
            clip->ch_frames[0][buff_index];
          rbuf_after_ts[j] =
            clip->channels == 1 ?
            clip->ch_frames[0][buff_index] :
            clip->ch_frames[1][buff_index];
        }
    }

  /* apply fades */
  for (nframes_t j = 0; j < nframes; j++)
    {
      long current_local_frame =
        local_start_frame + j;

      float fade_in = 1.f;
      float fade_out = 1.f;

      /* if inside fade in */
      if (current_local_frame >= 0 &&
          current_local_frame <
            r_obj->fade_in_pos.frames)
        {
          fade_in =
            (float)
            fade_get_y_normalized (
              (double) current_local_frame /
              (double)
              r_obj->fade_in_pos.frames,
              &r_obj->fade_in_opts, 1);
        }
      /* else if inside fade out */
      else if (current_local_frame >=
                 r_obj->fade_out_pos.frames)
        {
          fade_out =
            (float)
            fade_get_y_normalized (
              (double)
              (current_local_frame -
                  r_obj->fade_out_pos.frames) /
              (double)
              (r_obj->end_pos.frames -
                (r_obj->fade_out_pos.frames +
                 r_obj->pos.frames)),
              &r_obj->fade_out_opts, 0);
        }

      stereo_ports->l->buf[current_local_frame] =
          lbuf_after_ts[j] *
          fade_in * fade_out;
      stereo_ports->r->buf[current_local_frame] =
          rbuf_after_ts[j] *
          fade_in * fade_out;
    }
}

/**
 * Frees members only but not the audio region itself.
 *
 * Regions should be free'd using region_free.
 */
void
audio_region_free_members (ZRegion * self)
{
}
