/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * @file
 *
 * Left dock.
 */

#ifndef __GUI_WIDGETS_LEFT_DOCK_EDGE_H__
#define __GUI_WIDGETS_LEFT_DOCK_EDGE_H__

#include <gtk/gtk.h>

typedef struct _InspectorWidget  InspectorWidget;
typedef struct _VisibilityWidget VisibilityWidget;
typedef struct _FoldableNotebookWidget
  FoldableNotebookWidget;
typedef struct _InspectorTrackWidget
  InspectorTrackWidget;
typedef struct _InspectorPluginWidget
  InspectorPluginWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define LEFT_DOCK_EDGE_WIDGET_TYPE \
  (left_dock_edge_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  LeftDockEdgeWidget,
  left_dock_edge_widget,
  Z,
  LEFT_DOCK_EDGE_WIDGET,
  GtkWidget)

#define MW_LEFT_DOCK_EDGE \
  MW_CENTER_DOCK->left_dock_edge

/**
 * Left panel tabs.
 */
typedef enum LeftDockEdgeTab
{
  LEFT_DOCK_EDGE_TAB_TRACK,
  LEFT_DOCK_EDGE_TAB_PLUGIN,
  LEFT_DOCK_EDGE_TAB_VISIBILITY,
  LEFT_DOCK_EDGE_TAB_CC_BINDINGS,
  LEFT_DOCK_EDGE_TAB_PORT_CONNECTIONS,
} LeftDockEdgeTab;

/**
 * Left dock widget.
 */
typedef struct _LeftDockEdgeWidget
{
  GtkWidget                parent_instance;
  FoldableNotebookWidget * inspector_notebook;

  /** Track visibility. */
  GtkBox *           visibility_box;
  VisibilityWidget * visibility;

  /** For TracklistSelections. */
  GtkScrolledWindow *    track_inspector_scroll;
  InspectorTrackWidget * track_inspector;

  /** For MixerSelections. */
  GtkScrolledWindow *     plugin_inspector_scroll;
  InspectorPluginWidget * plugin_inspector;

  /** Mouse button press handler. */
  GtkGestureClick * mp;
} LeftDockEdgeWidget;

void
left_dock_edge_widget_refresh (
  LeftDockEdgeWidget * self);

/**
 * Refreshes the widget and switches to the given
 * page.
 */
void
left_dock_edge_widget_refresh_with_page (
  LeftDockEdgeWidget * self,
  LeftDockEdgeTab      page);

void
left_dock_edge_widget_setup (
  LeftDockEdgeWidget * self);

/**
 * Prepare for finalization.
 */
void
left_dock_edge_widget_tear_down (
  LeftDockEdgeWidget * self);

/**
 * @}
 */

#endif
