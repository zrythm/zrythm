/*
 * audio/audio_region.h - Audio region
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#ifndef __AUDIO_AUDIO_REGION_H__
#define __AUDIO_AUDIO_REGION_H__

#include "audio/position.h"
#include "audio/region.h"

typedef struct _RegionWidget RegionWidget;
typedef struct Channel Channel;
typedef struct Track Track;

typedef struct AudioRegion
{
  Region          parent;

} AudioRegion;

AudioRegion *
audio_region_new (Track *    track,
                 Position * start_pos,
                 Position * end_pos);

/**
 * Creates region (used when loading projects).
 */
AudioRegion *
audio_region_get_or_create_blank (int id);

#endif // __AUDIO_AUDIO_REGION_H__
