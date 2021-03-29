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

/** \file
 */

#include "audio/marker_track.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/color_area.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/marker_track.h"
#include "gui/widgets/pinned_tracklist.h"
#include "gui/widgets/track_top_grid.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MarkerTrackWidget,
               marker_track_widget,
               TRACK_WIDGET_TYPE)

/**
 * Creates a new track widget using the given track.
 *
 * 1 track has 1 track widget.
 * The track widget must always have at least 1 automation track in the automation
 * paned.
 */
MarkerTrackWidget *
marker_track_widget_new (
  Track * track)
{
  MarkerTrackWidget * self = g_object_new (
                            MARKER_TRACK_WIDGET_TYPE,
                            NULL);
  TRACK_WIDGET_GET_PRIVATE (self);

  self->track = track;

  track_widget_set_name (
    Z_TRACK_WIDGET (self), track->name);

  /* setup color */
  color_area_widget_set_color (tw_prv->color,
                               &track->color);

  return self;
}

/**
 * FIXME delete
 */
void
marker_track_widget_refresh_buttons (
  MarkerTrackWidget * self)
{
  /* TODO*/

}

void
marker_track_widget_refresh (
  MarkerTrackWidget * self)
{
}

static void
marker_track_widget_init (MarkerTrackWidget * self)
{
  TRACK_WIDGET_GET_PRIVATE (self);

  /* set icon */
  SET_TRACK_ICON ("kdenlive-show-markers");

  gtk_widget_set_visible (GTK_WIDGET (self), 1);
}

static void
marker_track_widget_class_init (
  MarkerTrackWidgetClass * _klass)
{
}

