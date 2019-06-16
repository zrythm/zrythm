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
#include "utils/flags.h"

#define SET_POS(_c,pos_name,_pos,_trans_only) \
  POSITION_SET_ARRANGER_OBJ_POS ( \
    marker, _c, pos_name, _pos, _trans_only)

void
marker_init_loaded (Marker * self)
{
  self->widget = marker_widget_new (self);
}

DEFINE_ARRANGER_OBJ_SET_POS (
  Marker, marker);

/**
 * Creates a Marker.
 */
Marker *
marker_new (
  const char * name,
  int          is_main)
{
  Marker * self = calloc (1, sizeof (Marker));

  self->name = g_strdup (name);
  self->type = MARKER_TYPE_CUSTOM;

  if (is_main)
    {
      /* set it as main */
      Marker * main_trans =
        marker_clone (
          self, MARKER_CLONE_COPY);
      Marker * lane =
        marker_clone (
          self, MARKER_CLONE_COPY);
      Marker * lane_trans =
        marker_clone (
          self, MARKER_CLONE_COPY);
      arranger_object_info_init_main (
        self,
        main_trans,
        lane,
        lane_trans,
        AOI_TYPE_MARKER);
    }

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

DEFINE_ARRANGER_OBJ_MOVE (
  Marker, marker);

/**
 * Generates a widget for the Marker.
 */
void
marker_gen_widget (
  Marker * self)
{
  Marker * c = self;
  for (int i = 0; i < 2; i++)
    {
      if (i == 0)
        c = marker_get_main_marker (c);
      else if (i == 1)
        c = marker_get_main_trans_marker (c);

      c->widget = marker_widget_new (c);
    }
}

DEFINE_IS_ARRANGER_OBJ_SELECTED (
  Marker, marker, timeline_selections,
  TL_SELECTIONS);

Marker *
marker_clone (
  Marker * src,
  MarkerCloneFlag flag)
{
  int is_main = 0;
  if (flag == MARKER_CLONE_COPY_MAIN)
    is_main = 1;

  Marker * marker =
    marker_new (src->name, is_main);

  position_set_to_pos (
    &marker->pos, &src->pos);

  return marker;
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
