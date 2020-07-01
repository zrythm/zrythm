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
 * Center dock.
 */

#ifndef __GUI_WIDGETS_CENTER_DOCK_H__
#define __GUI_WIDGETS_CENTER_DOCK_H__

#include "gui/widgets/timeline_selection_info.h"

#include <gtk/gtk.h>

#define CENTER_DOCK_WIDGET_TYPE \
  (center_dock_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  CenterDockWidget,
  center_dock_widget,
  Z, CENTER_DOCK_WIDGET,
  GtkBox)

typedef struct _LeftDockEdgeWidget
  LeftDockEdgeWidget;
typedef struct _RightDockEdgeWidget
  RightDockEdgeWidget;
typedef struct _BotDockEdgeWidget BotDockEdgeWidget;
typedef struct _TimelinePanelWidget
  TimelinePanelWidget;
typedef struct _EventViewerWidget EventViewerWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_CENTER_DOCK MAIN_WINDOW->center_dock

/**
 * Center dock.
 */
typedef struct _CenterDockWidget
{
  GtkBox               parent_instance;

  /** Event viewr + timeline panel. */
  GtkPaned *
    timeline_plus_event_viewer_paned;

  /** Event viewer. */
  EventViewerWidget *   event_viewer;

  /** Timeline panel. */
  TimelinePanelWidget * timeline_panel;

  GtkPaned *            left_rest_paned;
  GtkPaned *            center_right_paned;

  LeftDockEdgeWidget *  left_dock_edge;
  RightDockEdgeWidget * right_dock_edge;
  BotDockEdgeWidget *   bot_dock_edge;
  GtkPaned *            center_paned;
} CenterDockWidget;

void
center_dock_widget_setup (
  CenterDockWidget * self);

/**
 * Prepare for finalization.
 */
void
center_dock_widget_tear_down (
  CenterDockWidget * self);

/**
 * @}
 */

#endif
