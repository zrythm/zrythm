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

#include "audio/channel.h"
#include "audio/audio_region.h"
#include "audio/clip.h"
#include "audio/pool.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/audio_region.h"
#include "gui/widgets/region.h"
#include "project.h"
#include "utils/audio.h"
#include "utils/io.h"

#include "ext/audio_decoder/ad.h"

/**
 * Creates a Region for audio data.
 *
 * @param add_to_project Add Region to project
 *   registry. This should be false when cloning.
 * @param filename Filename, if loading from
 *   file, otherwise NULL.
 * @param frames Float array, if loading from
 *   float array, otherwise NULL.
 * @param nframes Number of frames per channel.
 */
AudioRegion *
audio_region_new (
  const char *     filename,
  const float *    frames,
  const long       nframes,
  const channels_t channels,
  const Position * start_pos,
  const int        is_main)
{
  AudioRegion * self =
    calloc (1, sizeof (AudioRegion));

  self->type = REGION_TYPE_AUDIO;
  self->pool_id = -1;

  AudioClip * clip = NULL;
  if (filename)
    clip =
      audio_clip_new_from_file (filename);
  else
    clip =
      audio_clip_new_from_float_array (
        frames, nframes, channels, "new audio clip");
  g_return_val_if_fail (clip, NULL);

  self->pool_id =
    audio_pool_add_clip (
      AUDIO_POOL, clip);
  g_warn_if_fail (self->pool_id > -1);

  /* set end pos to sample end */
  position_set_to_pos (
    &self->end_pos, start_pos);
  position_add_frames (
    &self->end_pos, clip->num_frames);

  /* init */
  region_init ((Region *) self,
               start_pos,
               &self->end_pos,
               is_main);

  if (is_main)
    audio_clip_write_to_pool (clip);

  return self;
}

/**
 * Frees members only but not the audio region itself.
 *
 * Regions should be free'd using region_free.
 */
void
audio_region_free_members (AudioRegion * self)
{
}
