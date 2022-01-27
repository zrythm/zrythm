/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Right panel.
 */

#ifndef __GUI_WIDGETS_RIGHT_DOCK_EDGE_H__
#define __GUI_WIDGETS_RIGHT_DOCK_EDGE_H__

#include <gtk/gtk.h>

#define RIGHT_DOCK_EDGE_WIDGET_TYPE \
  (right_dock_edge_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RightDockEdgeWidget, right_dock_edge_widget,
  Z, RIGHT_DOCK_EDGE_WIDGET, GtkBox)

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
  GtkBox                   parent_instance;
  FoldableNotebookWidget * right_notebook;

  GtkBox *                 plugin_browser_box;
  PluginBrowserWidget *    plugin_browser;

  GtkBox *                 file_browser_box;
  PanelFileBrowserWidget * file_browser;

  GtkBox *                 monitor_section_box;
  MonitorSectionWidget *   monitor_section;

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
