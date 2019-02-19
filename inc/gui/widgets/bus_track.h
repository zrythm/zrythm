/*
 * gui/widgets/track.h - Track view
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_BUS_TRACK_H__
#define __GUI_WIDGETS_BUS_TRACK_H__

#include "audio/channel.h"
#include "gui/widgets/track.h"

#include <gtk/gtk.h>

#define BUS_TRACK_WIDGET_TYPE \
  (bus_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (BusTrackWidget,
                      bus_track_widget,
                      Z,
                      BUS_TRACK_WIDGET,
                      TrackWidget)

typedef struct _AutomationTracklistWidget
  AutomationTracklistWidget;
typedef struct BusTrack BusTrack;

/**
 * Top is the track part and bot is the automation part
 */
typedef struct _BusTrackWidget
{
  TrackWidget                parent_instance;
  GtkToggleButton *          record;
  GtkToggleButton *          solo;
  GtkToggleButton *          mute;
  GtkToggleButton *          show_automation;
} BusTrackWidget;

/**
 * Creates a new track widget from the given track.
 */
BusTrackWidget *
bus_track_widget_new (Track * track);

/**
 * Updates changes in the backend to the ui
 */
void
bus_track_widget_refresh (BusTrackWidget * self);

void
bus_track_widget_refresh_buttons (
  BusTrackWidget * self);

#endif
