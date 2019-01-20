/*
 * audio/timeline_minimap.c - Timeline minimap backend
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

#include <stdlib.h>

#include "audio/timeline_minimap.h"

/**
 * To be called once during initialization.
 */
TimelineMinimap *
timeline_minimap_new ()
{
  TimelineMinimap * self =
    calloc (1, sizeof (TimelineMinimap));

  position_init (&self->selection_start);
  position_init (&self->selection_end);

  return self;
}

/**
 * Calculates start/end based on zoom.
 *
 * To be called when zoom changed from other sources besides
 * minimap.
 */
void
timeline_minimap_update_selector_from_zoom (int zoom)
{
  /* TODO calculate start/end pos based on zoom level */

}

/**
 * Returns zoom level from current selector poses.
 */
int
timeline_minimap_get_zoom_from_selector ()
{
  /* TODO */

  return 2;
}
