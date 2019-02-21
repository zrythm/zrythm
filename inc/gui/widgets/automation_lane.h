/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_AUTOMATION_LANE_H__
#define __GUI_WIDGETS_AUTOMATION_LANE_H__

#include <gtk/gtk.h>

#define AUTOMATION_LANE_WIDGET_TYPE                  (automation_lane_widget_get_type ())
G_DECLARE_FINAL_TYPE (AutomationLaneWidget,
                      automation_lane_widget,
                      Z,
                      AUTOMATION_LANE_WIDGET,
                      GtkGrid)

typedef struct _TrackWidget TrackWidget;
typedef struct AutomationTrack AutomationTrack;
typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct Track Track;
typedef struct _AutomationPointWidget AutomationPointWidget;
typedef struct AutomationLane AutomationLane;

typedef struct _AutomationLaneWidget
{
  GtkGrid                 parent_instance;
  GtkComboBox *           selector;
  GtkTreeModel *          selector_model;
  GtkBox *                value_box;
  DigitalMeterWidget *    value;
  GtkToggleButton *       mute_toggle;
  AutomationLane *        al; ///< associated automation track

  /**
   * Selected automatable path.
   */
  char *                  path;

  /**
   * For freezing callbacks.
   */
  gulong                  selector_changed_cb_id;
} AutomationLaneWidget;

/**
 * Creates a new automation_track widget from the given automation_track.
 */
AutomationLaneWidget *
automation_lane_widget_new ();

/**
 * Updates GUI.
 */
void
automation_lane_widget_refresh (
  AutomationLaneWidget * self);

/**
 * Gets the float value at the y-point of the automation track.
 */
float
automation_lane_widget_get_fvalue_at_y (
  AutomationLaneWidget * self,
  double                 _start_y);

double
automation_lane_widget_get_y (
  AutomationLaneWidget *  self,
  AutomationPointWidget * ap);

#endif
