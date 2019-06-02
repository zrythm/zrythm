/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/track.h"
#include "audio/track_lane.h"
#include "utils/arrays.h"

#include <glib/gi18n.h>

/**
 * Creates a new TrackLane at the given pos in the
 * given Track.
 *
 * @param track The Track to create the TrackLane for.
 * @param pos The position (index) in the Track that
 *   this lane will be placed in.
 */
TrackLane *
track_lane_new (
  Track * track,
  int     pos)
{
  TrackLane * self = calloc (1, sizeof (TrackLane));

  self->name = g_strdup_printf (_("Lane %d"), pos);
  self->pos = pos;
  self->track = track;
  self->track_pos = track->pos;

  return self;
}

/**
 * Adds a Region to the given TrackLane.
 */
void
track_lane_add_region (
  TrackLane * self,
  Region *    region)
{
  region_set_lane (region, self);

  array_append (self->regions,
                self->num_regions,
                region);
}

/**
 * Clones the TrackLane.
 *
 * Mainly used when cloning Track's.
 */
TrackLane *
track_lane_clone (
  TrackLane * lane)
{
  TrackLane * new_lane =
    calloc (1, sizeof (TrackLane));

  Region * region, * new_region;
  for (int i = 0; i < lane->num_regions; i++)
    {
      /* clone region */
      region = lane->regions[i];
      new_region =
        region_clone (
          region, REGION_CLONE_COPY_MAIN);

      /* add to new lane */
      array_append (
        new_lane->regions,
        new_lane->num_regions,
        new_region);
    }

  return new_lane;
}

/**
 * Frees the TrackLane.
 */
void
track_lane_free (
  TrackLane * self)
{
  g_object_unref (
    G_OBJECT (self->widget));

  if (self->name)
    g_free (self->name);

  for (int i = 0; i < self->num_regions; i++)
    region_free_all (self->regions[i]);

  free (self);
}
