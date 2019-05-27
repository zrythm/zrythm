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

#ifndef __GUI_WIDGETS_TRACK_LANE_H__
#define __GUI_WIDGETS_TRACK_LANE_H__

#include <gtk/gtk.h>

#define TRACK_LANE_WIDGET_TYPE \
  (track_lane_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackLaneWidget,
  track_lane_widget,
  Z, TRACK_LANE_WIDGET,
  GtkGrid);

typedef struct _AutomationTracklistWidget
  AutomationTracklistWidget;
typedef struct TrackLane TrackLane;

/**
 * Top is the track part and bot is the automation part
 */
typedef struct _TrackLaneWidget
{
  GtkGrid              parent_instance;

  GtkLabel *           label;
  GtkToggleButton *    solo;
  GtkToggleButton *    mute;

  /** Pointer to backend. */
  TrackLane *          lane;
} TrackLaneWidget;

/**
 * Creates a new track widget from the given track.
 */
TrackLaneWidget *
track_lane_widget_new (
  TrackLane * lane);

/**
 * Updates changes in the backend to the ui
 */
void
track_lane_widget_refresh (
  TrackLaneWidget * self);

#endif
