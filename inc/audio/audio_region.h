/*
 * audio/audio_region.h - Audio region
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

/**
 * \file
 *
 * API for Regions inside audio Track's.
 */

#ifndef __AUDIO_AUDIO_REGION_H__
#define __AUDIO_AUDIO_REGION_H__

#include "audio/position.h"
#include "audio/region.h"
#include "utils/types.h"

typedef struct _RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;
typedef struct Region AudioRegion;
typedef struct AudioClip AudioClip;

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Creates a Region for audio data.
 *
 * FIXME First create the
 * audio on the pool and then pass the pool id here.
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
  const int        is_main);

/**
 * Returns the audio clip associated with the
 * Region.
 */
AudioClip *
audio_region_get_clip (
  const Region * self);

/**
 * Frees members only but not the audio region itself.
 *
 * Regions should be free'd using region_free.
 */
void
audio_region_free_members (AudioRegion * self);

/**
 * @}
 */

#endif // __AUDIO_AUDIO_REGION_H__
