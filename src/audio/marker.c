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

#include "audio/marker.h"
#include "audio/marker_track.h"
#include "gui/widgets/marker.h"
#include "project.h"

NOTE_LABELS;

void
marker_init_loaded (Marker * self)
{
  self->widget = marker_widget_new (self);
}

void
marker_set_pos (Marker *    self,
               Position * pos)
{
  position_set_to_pos (&self->pos, pos);
}

/**
 * Creates a Marker.
 */
Marker *
marker_new (
  Position * pos)
{
  Marker * self = calloc (1, sizeof (Marker));

  self->type = MARKER_TYPE_CUSTOM;

  self->widget = marker_widget_new (self);

  self->visible = 1;

  return self;
}

/**
 * Finds the chord in the project corresponding to the
 * given one.
 */
Marker *
marker_find (
  Marker * clone)
{
  for (int i = 0;
       i < P_MARKER_TRACK->num_markers; i++)
    {
      if (marker_is_equal (
            P_MARKER_TRACK->markers[i],
            clone))
        return P_MARKER_TRACK->markers[i];
    }
  return NULL;
}

/**
 * Frees the Marker.
 */
void
marker_free (Marker * self)
{
  free (self);
}
