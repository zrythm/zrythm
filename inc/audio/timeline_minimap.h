/*
 * audio/timeline_minimap.h - Timeline minimap backend
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

/** \file */

#ifndef __AUDIO_TIMELINE_MINIMAP_H__
#define __AUDIO_TIMELINE_MINIMAP_H__

#include "audio/position.h"

typedef struct TimelineMinimap
{
  Position            selection_start;
  Position            selection_end;
  float *             zoom; ///< pointer to the zoom value
} TimelineMinimap;

/**
 * To be called once during initialization.
 */
TimelineMinimap *
timeline_minimap_new ();

void
timeline_minimap_update_selector_from_zoom (int zoom);

/**
 * Returns zoom level from current selector poses.
 */
int
timeline_minimap_get_zoom_from_selector ();

#endif
