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

/**
 * \file
 *
 * Bus track for effects.
 */

#ifndef __GUI_WIDGETS_MIDI_BUS_TRACK_H__
#define __GUI_WIDGETS_MIDI_BUS_TRACK_H__

#include "audio/channel.h"
#include "gui/widgets/track.h"

#include <gtk/gtk.h>

#define MIDI_BUS_TRACK_WIDGET_TYPE \
  (midi_bus_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  MidiBusTrackWidget,
  midi_bus_track_widget,
  Z, MIDI_BUS_TRACK_WIDGET,
  TrackWidget)

typedef struct _AutomationTracklistWidget
  AutomationTracklistWidget;
typedef struct Track MidiBusTrack;

/**
 * Top is the track part and bot is the automation part
 */
typedef struct _MidiBusTrackWidget
{
  TrackWidget                parent_instance;
  GtkToggleButton *          solo;
  GtkToggleButton *          mute;
  GtkToggleButton *          show_automation;
} MidiBusTrackWidget;

/**
 * Creates a new track widget from the given track.
 */
MidiBusTrackWidget *
midi_bus_track_widget_new (Track * track);

/**
 * Updates changes in the backend to the ui
 */
void
midi_bus_track_widget_refresh (
  MidiBusTrackWidget * self);

void
midi_bus_track_widget_refresh_buttons (
  MidiBusTrackWidget * self);

#endif
