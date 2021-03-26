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
 * Widget for TrackTopGrid.
 */

#ifndef __GUI_WIDGETS_TRACK_TOP_GRID_H__
#define __GUI_WIDGETS_TRACK_TOP_GRID_H__

#include <gtk/gtk.h>

#define TRACK_TOP_GRID_WIDGET_TYPE \
  (track_top_grid_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  TrackTopGridWidget,
  track_top_grid_widget,
  Z, TRACK_TOP_GRID_WIDGET,
  GtkGrid);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define GET_CHANNEL(x) \
 (track_get_channel (track_widget_get_private (Z_TRACK_WIDGET (x))->track));

typedef struct _AutomationTracklistWidget
  AutomationTracklistWidget;
typedef struct Track TrackTopGrid;
typedef struct _MidiActivityBarWidget
  MidiActivityBarWidget;
typedef struct _EditableLabelWidget
  EditableLabelWidget;

/**
 * Top is the track part and bot is the automation
 * part
 */
typedef struct _TrackTopGridWidget
{
  GtkGrid             parent_instance;

  /** Track name. */
  EditableLabelWidget *  name;

  /**
   * These are boxes to be filled by inheriting
   * widgets.
   */
  GtkBox *            upper_controls;
  GtkBox *            right_activity_box;
  MidiActivityBarWidget * midi_activity;
  GtkBox *            mid_controls;
  GtkBox *            bot_controls;

  /** Owner TrackWidget. */
  TrackWidget *       owner;

} TrackTopGridWidget;

/**
 * Refreshes the widget to reflect the backend.
 */
//void
//track_top_grid_widget_refresh (
  //TrackTopGridWidget * self);

/**
 * Sets up the TrackTopGridWidget.
 */
//void
//track_top_grid_widget_setup (
  //TrackTopGridWidget * self,
  //TrackWidget *        owner);

/**
 * @}
 */

#endif
