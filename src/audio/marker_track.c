// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 */

#include <stdlib.h>

#include "audio/marker_track.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/mem.h"
#include "utils/object_utils.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

/**
 * Inits the marker track.
 */
void
marker_track_init (Track * self)
{
  self->type = TRACK_TYPE_MARKER;
  self->main_height = TRACK_DEF_HEIGHT / 2;
  self->icon_name = g_strdup ("markers");

  /* GTK color picker color */
  gdk_rgba_parse (&self->color, "#813D9C");
}

/**
 * Creates the default marker track.
 */
MarkerTrack *
marker_track_default (int track_pos)
{
  Track * self = track_new (
    TRACK_TYPE_MARKER, track_pos, _ ("Markers"),
    F_WITHOUT_LANE);

#if 0
  /* hack to allow setting positions */
  double frames_per_tick_before =
    AUDIO_ENGINE->frames_per_tick;
  if (math_doubles_equal (
        frames_per_tick_before, 0))
    {
      AUDIO_ENGINE->frames_per_tick = 512;
    }
#endif

  /* add start and end markers */
  Marker * marker;
  Position pos;
  marker = marker_new (_ ("start"));
  ArrangerObject * m_obj = (ArrangerObject *) marker;
  position_set_to_bar (&pos, 1);
  arranger_object_pos_setter (m_obj, &pos);
  marker->type = MARKER_TYPE_START;
  marker_track_add_marker (self, marker);
  marker = marker_new (_ ("end"));
  m_obj = (ArrangerObject *) marker;
  position_set_to_bar (&pos, 129);
  arranger_object_pos_setter (m_obj, &pos);
  marker->type = MARKER_TYPE_END;
  marker_track_add_marker (self, marker);

#if 0
  if (math_doubles_equal (
        frames_per_tick_before, 0))
    {
      AUDIO_ENGINE->frames_per_tick = 0;
    }
#endif

  return self;
}

/**
 * Returns the start marker.
 */
Marker *
marker_track_get_start_marker (const Track * self)
{
  g_return_val_if_fail (
    self->type == TRACK_TYPE_MARKER, NULL);

  Marker * marker;
  for (int i = 0; i < self->num_markers; i++)
    {
      marker = self->markers[i];
      if (marker->type == MARKER_TYPE_START)
        return marker;
    }
  g_critical (
    "start marker not found in track '%s' "
    "(num markers %d)",
    self->name, self->num_markers);
  return NULL;
}

/**
 * Returns the end marker.
 */
Marker *
marker_track_get_end_marker (const Track * self)
{
  g_return_val_if_fail (
    self->type == TRACK_TYPE_MARKER, NULL);

  Marker * marker;
  for (int i = 0; i < self->num_markers; i++)
    {
      marker = self->markers[i];
      if (marker->type == MARKER_TYPE_END)
        return marker;
    }
  g_critical (
    "end marker not found in track '%s' "
    "(num markers %d)",
    self->name, self->num_markers);
  return NULL;
}

/**
 * Inserts a marker to the track.
 */
void
marker_track_insert_marker (
  MarkerTrack * self,
  Marker *      marker,
  int           pos)
{
  g_return_if_fail (
    self->type == TRACK_TYPE_MARKER && marker);

  marker_set_track_name_hash (
    marker, track_get_name_hash (self));
  array_double_size_if_full (
    self->markers, self->num_markers,
    self->markers_size, Marker *);
  array_insert (
    self->markers, self->num_markers, pos, marker);

  for (int i = pos; i < self->num_markers; i++)
    {
      Marker * m = self->markers[i];
      marker_set_index (m, i);
    }

  marker_track_validate (self);

  EVENTS_PUSH (ET_ARRANGER_OBJECT_CREATED, marker);
}

/**
 * Adds a marker to the track.
 */
void
marker_track_add_marker (
  MarkerTrack * self,
  Marker *      marker)
{
  marker_track_insert_marker (
    self, marker, self->num_markers);
}

/**
 * Removes all objects from the marker track.
 *
 * Mainly used in testing.
 */
void
marker_track_clear (MarkerTrack * self)
{
  for (int i = 0; i < self->num_markers; i++)
    {
      Marker * marker = self->markers[i];
      if (
        marker->type == MARKER_TYPE_START
        || marker->type == MARKER_TYPE_END)
        continue;
      marker_track_remove_marker (self, marker, 1);
    }
}

bool
marker_track_validate (MarkerTrack * self)
{
  for (int i = 0; i < self->num_markers; i++)
    {
      Marker * m = self->markers[i];
      g_return_val_if_fail (m->index == i, false);
    }
  return true;
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

  int pos = -1;
  array_delete_return_pos (
    self->markers, self->num_markers, marker, pos);
  g_return_if_fail (pos >= 0);

  for (int i = pos; i < self->num_markers; i++)
    {
      Marker * m = self->markers[i];
      marker_set_index (m, i);
    }

  if (free)
    free_later (marker, arranger_object_free);

  EVENTS_PUSH (
    ET_ARRANGER_OBJECT_REMOVED,
    ARRANGER_OBJECT_TYPE_MARKER);
}
