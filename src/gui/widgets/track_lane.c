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

#include "audio/track_lane.h"
#include "gui/widgets/track_lane.h"
#include "project.h"
#include "utils/ui.h"

G_DEFINE_TYPE (TrackLaneWidget,
               track_lane_widget,
               GTK_TYPE_GRID)

/**
 * Updates changes in the backend to the ui
 */
void
track_lane_widget_refresh (
  TrackLaneWidget * self)
{

}

TrackLaneWidget *
track_lane_widget_new (
  TrackLane * lane)
{
  TrackLaneWidget * self =
    g_object_new (TRACK_LANE_WIDGET_TYPE, NULL);

  self->lane = lane;

  return self;
}

static void
track_lane_widget_class_init (
  TrackLaneWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  gtk_widget_class_set_css_name (
    klass, "track-lane");
}

static void
track_lane_widget_init (TrackLaneWidget * self)
{
  gtk_widget_set_visible (GTK_WIDGET (self),
                          1);
}
