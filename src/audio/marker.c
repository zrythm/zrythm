/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
#include "utils/objects.h"
#include "utils/string.h"

/**
 * Creates a Marker.
 */
Marker *
marker_new (const char * name)
{
  Marker * self = object_new (Marker);

  ArrangerObject * obj = (ArrangerObject *) self;
  obj->type = ARRANGER_OBJECT_TYPE_MARKER;

  self->name = g_strdup (name);
  self->type = MARKER_TYPE_CUSTOM;
  position_init (&obj->pos);

  arranger_object_init (obj);

  arranger_object_gen_escaped_name (
    (ArrangerObject *) self);

  return self;
}

void
marker_set_index (Marker * self, int index)
{
  self->index = index;
}

/**
 * Sets the Track of the Marker.
 */
void
marker_set_track_name_hash (
  Marker *     marker,
  unsigned int track_name_hash)
{
  marker->track_name_hash = track_name_hash;
}

Marker *
marker_find_by_name (const char * name)
{
  for (int i = 0; i < P_MARKER_TRACK->num_markers;
       i++)
    {
      Marker * marker = P_MARKER_TRACK->markers[i];
      if (string_is_equal (name, marker->name))
        {
          return marker;
        }
    }

  return NULL;
}

/**
 * Returns if the two Marker's are equal.
 */
int
marker_is_equal (Marker * a, Marker * b)
{
  ArrangerObject * obj_a = (ArrangerObject *) a;
  ArrangerObject * obj_b = (ArrangerObject *) b;
  return position_is_equal (&obj_a->pos, &obj_b->pos)
         && string_is_equal (a->name, b->name);
}
