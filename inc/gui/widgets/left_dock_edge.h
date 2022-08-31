// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Left dock.
 */

#ifndef __GUI_WIDGETS_LEFT_DOCK_EDGE_H__
#define __GUI_WIDGETS_LEFT_DOCK_EDGE_H__

#include <gtk/gtk.h>
#include <libpanel.h>

typedef struct _InspectorWidget        InspectorWidget;
typedef struct _FoldableNotebookWidget FoldableNotebookWidget;
typedef struct _InspectorTrackWidget   InspectorTrackWidget;
typedef struct _InspectorPluginWidget  InspectorPluginWidget;

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

#define MW_LEFT_DOCK_EDGE MW_CENTER_DOCK->left_dock_edge

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
  GtkWidget    parent_instance;
  PanelFrame * panel_frame;

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
left_dock_edge_widget_refresh (LeftDockEdgeWidget * self);

/**
 * Refreshes the widget and switches to the given
 * page.
 */
void
left_dock_edge_widget_refresh_with_page (
  LeftDockEdgeWidget * self,
  LeftDockEdgeTab      page);

void
left_dock_edge_widget_setup (LeftDockEdgeWidget * self);

/**
 * Prepare for finalization.
 */
void
left_dock_edge_widget_tear_down (LeftDockEdgeWidget * self);

/**
 * @}
 */

#endif
