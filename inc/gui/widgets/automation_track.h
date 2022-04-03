/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_AUTOMATION_TRACK_H__
#define __GUI_WIDGETS_AUTOMATION_TRACK_H__

#include <gtk/gtk.h>

#define AUTOMATION_TRACK_WIDGET_TYPE \
  (automation_track_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomationTrackWidget,
  automation_track_widget,
  Z,
  AUTOMATION_TRACK_WIDGET,
  GtkGrid)

typedef struct _TrackWidget        TrackWidget;
typedef struct AutomationTrack     AutomationTrack;
typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct Track               Track;
typedef struct _AutomationPointWidget
  AutomationPointWidget;
typedef struct AutomationTrack AutomationTrack;
typedef struct _AutomatableSelectorButtonWidget
  AutomatableSelectorButtonWidget;

typedef struct _AutomationTrackWidget
{
  GtkGrid                           parent_instance;
  AutomatableSelectorButtonWidget * selector;
  GtkTreeModel *                    selector_model;
  GtkBox *                          value_box;
  DigitalMeterWidget *              value;
  GtkToggleButton *                 mute_toggle;

  /**
   * Associated automation track.
   */
  AutomationTrack * at;

  /**
   * Selected automatable path.
   */
  char * path;

  /** Cache for showing the value. */
  float last_val;

  /**
   * For freezing callbacks.
   */
  gulong selector_changed_cb_id;

  GtkLabel * current_val;
} AutomationTrackWidget;

/**
 * Creates a new automation_track widget from the given automation_track.
 */
AutomationTrackWidget *
automation_track_widget_new (AutomationTrack * at);

/**
 * Updates GUI.
 */
void
automation_track_widget_refresh (
  AutomationTrackWidget * self);

void
automation_track_widget_update_current_val (
  AutomationTrackWidget * self);

/**
 * Returns the y pixels from the value based on the
 * allocation of the automation track.
 */
int
automation_track_get_y_px_from_normalized_val (
  AutomationTrackWidget * self,
  float                   fval);

/**
 * Gets the float value at the given Y coordinate
 * relative to the AutomationTrackWidget.
 */
float
automation_track_widget_get_fvalue_at_y (
  AutomationTrackWidget * self,
  double                  _start_y);

double
automation_track_widget_get_y (
  AutomationTrackWidget * self,
  AutomationPointWidget * ap);

#endif
