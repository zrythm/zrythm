// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Right panel.
 */

#ifndef __GUI_WIDGETS_RIGHT_DOCK_EDGE_H__
#define __GUI_WIDGETS_RIGHT_DOCK_EDGE_H__

#include <gtk/gtk.h>
#include <libpanel.h>

#define RIGHT_DOCK_EDGE_WIDGET_TYPE \
  (right_dock_edge_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RightDockEdgeWidget,
  right_dock_edge_widget,
  Z,
  RIGHT_DOCK_EDGE_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MW_RIGHT_DOCK_EDGE \
  MW_CENTER_DOCK->right_dock_edge

typedef struct _PluginBrowserWidget
  PluginBrowserWidget;
typedef struct _FileBrowserWidget FileBrowserWidget;
typedef struct _MonitorSectionWidget
  MonitorSectionWidget;
typedef struct _FoldableNotebookWidget
  FoldableNotebookWidget;
typedef struct _PanelFileBrowserWidget
  PanelFileBrowserWidget;
typedef struct _ChordPackBrowserWidget
  ChordPackBrowserWidget;

typedef struct _RightDockEdgeWidget
{
  GtkWidget    parent_instance;
  PanelFrame * panel_frame;

  GtkBox *              plugin_browser_box;
  PluginBrowserWidget * plugin_browser;

  GtkBox *                 file_browser_box;
  PanelFileBrowserWidget * file_browser;

  GtkBox *               monitor_section_box;
  MonitorSectionWidget * monitor_section;

  GtkBox *                 chord_pack_browser_box;
  ChordPackBrowserWidget * chord_pack_browser;
} RightDockEdgeWidget;

/**
 * Sets up the widget.
 */
void
right_dock_edge_widget_setup (
  RightDockEdgeWidget * self);

/**
 * @}
 */

#endif
