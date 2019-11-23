/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/marker.h"
#include "audio/marker_track.h"
#include "gui/widgets/marker.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/string.h"

/**
 * Creates a Marker.
 */
Marker *
marker_new (
  const char * name,
  int          is_main)
{
  Marker * self = calloc (1, sizeof (Marker));

  ArrangerObject * obj =
    (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_MARKER;

  self->name = g_strdup (name);
  self->type = MARKER_TYPE_CUSTOM;
  position_set_bar (&obj->pos, 1);

  return self;
}

/**
 * Sets the Track of the Marker.
 */
void
marker_set_track (
  Marker * marker,
  Track *  track)
{
  marker->track = track;
  marker->track_pos = track->pos;
}

/**
 * Returns if the two Marker's are equal.
 */
int
marker_is_equal (
  Marker * a,
  Marker * b)
{
  ArrangerObject * obj_a =
    (ArrangerObject *) a;
  ArrangerObject * obj_b =
    (ArrangerObject *) b;
  return
    position_is_equal (&obj_a->pos, &obj_b->pos) &&
    string_is_equal (a->name, b->name, 1);
}
