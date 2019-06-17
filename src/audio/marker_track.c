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

#include "audio/marker_track.h"
#include "audio/track.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

/**
 * Creates the default marker track.
 */
MarkerTrack *
marker_track_default ()
{
  MarkerTrack * self =
    calloc (1, sizeof (MarkerTrack));

  Track * track = (Track *) self;
  track->type = TRACK_TYPE_MARKER;
  track_init (track);

  self->name = g_strdup (_("Markers"));

  gdk_rgba_parse (&self->color, "#A3289a");

  return self;
}

/**
 * Adds a Marker to the Track.\
 *
 * @gen_widget Generates a widget for the Marker.
 */
void
marker_track_add_marker (
  MarkerTrack * track,
  Marker *      marker,
  int           gen_widget)
{
  g_warn_if_fail (
    track->type == TRACK_TYPE_MARKER && marker);
  g_warn_if_fail (
    marker->obj_info.counterpart ==
      AOI_COUNTERPART_MAIN);
  g_warn_if_fail (
    marker->obj_info.main &&
    marker->obj_info.main_trans &&
    marker->obj_info.lane &&
    marker->obj_info.lane_trans);

  marker_set_track (marker, track);
  array_append (track->markers,
                track->num_markers,
                marker);

  if (gen_widget)
    marker_gen_widget (marker);

  EVENTS_PUSH (ET_MARKER_CREATED, marker);
}

/**
 * Removes a marker, optionally freeing it.
 */
void
marker_track_remove_marker (
  MarkerTrack * self,
  Marker *      marker,
  int           free)
{
  array_delete (self->markers,
                self->num_markers,
                marker);

  if (free)
    free_later (marker, marker_free);
}

void
marker_track_free (MarkerTrack * self)
{
  /* TODO free Marker's */

}
