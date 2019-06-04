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

#ifndef __GUI_WIDGETS_MARKER_TRACK_H__
#define __GUI_WIDGETS_MARKER_TRACK_H__

#include "gui/widgets/track.h"

#include <gtk/gtk.h>

#define MARKER_TRACK_WIDGET_TYPE \
  (marker_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MarkerTrackWidget,
  marker_track_widget,
  Z, MARKER_TRACK_WIDGET,
  TrackWidget)

typedef struct Track MarkerTrack;

typedef struct _MarkerTrackWidget
{
  TrackWidget   parent_instance;

  /** The Marker track. */
  Track *       track;
} MarkerTrackWidget;

/**
 * Creates a new marker_track widget from the given marker_track.
 */
MarkerTrackWidget *
marker_track_widget_new (Track * track);

void
marker_track_widget_refresh_buttons (
  MarkerTrackWidget * self);

/**
 * Updates changes in the backend to the ui
 */
void
marker_track_widget_refresh (
  MarkerTrackWidget * self);

#endif
