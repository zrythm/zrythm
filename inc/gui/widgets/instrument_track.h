/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
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
 * Widget for InstrumentTrack.
 */

#ifndef __GUI_WIDGETS_INSTRUMENT_TRACK_H__
#define __GUI_WIDGETS_INSTRUMENT_TRACK_H__

#include "audio/channel.h"
#include "gui/widgets/track.h"

#include <gtk/gtk.h>

#define INSTRUMENT_TRACK_WIDGET_TYPE \
  (instrument_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (InstrumentTrackWidget,
                      instrument_track_widget,
                      Z,
                      INSTRUMENT_TRACK_WIDGET,
                      TrackWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define GET_CHANNEL(x) \
 (track_get_channel (track_widget_get_private (Z_TRACK_WIDGET (x))->track));

typedef struct _AutomationTracklistWidget
  AutomationTracklistWidget;
typedef struct Track InstrumentTrack;

/**
 * Top is the track part and bot is the automation
 * part
 */
typedef struct _InstrumentTrackWidget
{
  TrackWidget                   parent_instance;
  GtkToggleButton *             record;
  GtkToggleButton *             solo;
  GtkToggleButton *             mute;
  GtkToggleButton *             show_ui;
  GtkToggleButton *             show_lanes;
  GtkToggleButton *             show_automation;
  GtkToggleButton *             lock;
  GtkToggleButton *             freeze;
  gulong 			gui_toggled_handler_id;
} InstrumentTrackWidget;

/**
 * Creates a new track widget from the given track.
 */
InstrumentTrackWidget *
instrument_track_widget_new (Track * track);

 void
instument_track_ui_toggle (GtkWidget * self, InstrumentTrackWidget * data);

/**
 * Updates ui_active state if instrument window is closed
 *
 */
void
instrument_track_widget_on_plugin_delete_event (GtkWidget *window, GdkEventKey *e, gpointer data);
/**
 * Updates changes in the backend to the ui
 */
void
instrument_track_widget_refresh (
  InstrumentTrackWidget * self);

void
instrument_track_widget_refresh_buttons (
  InstrumentTrackWidget * self);

/**
 * @}
 */

#endif
