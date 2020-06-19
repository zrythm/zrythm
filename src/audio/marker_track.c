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

#include <stdlib.h>

#include "audio/marker_track.h"
#include "audio/track.h"
#include "project.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * Inits the marker track.
 */
void
marker_track_init (
  Track * self)
{
  self->type = TRACK_TYPE_MARKER;
  self->main_height = TRACK_DEF_HEIGHT / 2;

  gdk_rgba_parse (&self->color, "#a328aa");
}

/**
 * Creates the default marker track.
 */
MarkerTrack *
marker_track_default (
  int   track_pos)
{
  Track * self =
    track_new (
      TRACK_TYPE_MARKER, track_pos, _("Markers"),
      F_WITHOUT_LANE);

  /* add start and end markers */
  Marker * marker;
  Position pos;
  marker = marker_new (_("start"));
  ArrangerObject * m_obj =
    (ArrangerObject *) marker;
  position_set_to_bar (&pos, 1);
  arranger_object_pos_setter (m_obj, &pos);
  marker->type = MARKER_TYPE_START;
  marker_track_add_marker (self, marker);
  marker = marker_new (_("end"));
  m_obj =
    (ArrangerObject *) marker;
  position_set_to_bar (&pos, 129);
  arranger_object_pos_setter (m_obj, &pos);
  marker->type = MARKER_TYPE_END;
  marker_track_add_marker (self, marker);

  return self;
}

/**
 * Returns the start marker.
 */
Marker *
marker_track_get_start_marker (
  const Track * track)
{
  g_return_val_if_fail (
    track->type == TRACK_TYPE_MARKER, NULL);

  Marker * marker;
  for (int i = 0; i < track->num_markers; i++)
    {
      marker = track->markers[i];
      if (marker->type == MARKER_TYPE_START)
        return marker;
    }
  g_return_val_if_reached (NULL);
}

/**
 * Returns the end marker.
 */
Marker *
marker_track_get_end_marker (
  const Track * track)
{
  g_return_val_if_fail (
    track->type == TRACK_TYPE_MARKER, NULL);

  Marker * marker;
  for (int i = 0; i < track->num_markers; i++)
    {
      marker = track->markers[i];
      if (marker->type == MARKER_TYPE_END)
        return marker;
    }
  g_return_val_if_reached (NULL);
}

/**
 * Adds a Marker to the Track.\
 *
 * @gen_widget Generates a widget for the Marker.
 */
void
marker_track_add_marker (
  MarkerTrack * track,
  Marker *      marker)
{
  g_warn_if_fail (
    track->type == TRACK_TYPE_MARKER && marker);

  marker_set_track (marker, track);
  array_double_size_if_full (
    track->markers, track->num_markers,
    track->markers_size, Marker *);
  array_append (track->markers,
                track->num_markers,
                marker);
  marker->index = track->num_markers - 1;

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_CREATED, marker);
}

/**
 * Removes all objects from the marker track.
 *
 * Mainly used in testing.
 */
void
marker_track_clear (
  MarkerTrack * self)
{
  for (int i = 0; i < self->num_markers; i++)
    {
      Marker * marker = self->markers[i];
      if (marker->type == MARKER_TYPE_START ||
          marker->type == MARKER_TYPE_END)
        continue;
      marker_track_remove_marker (self, marker, 1);
    }
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
  /* deselect */
  arranger_selections_remove_object (
    (ArrangerSelections *) TL_SELECTIONS,
    (ArrangerObject *) marker);

  array_delete (
    self->markers, self->num_markers, marker);

  if (free)
    free_later (marker, arranger_object_free);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_REMOVED,
    ARRANGER_OBJECT_TYPE_MARKER);
}
