/*
 * gui/widgets/top_dock_edge.h - Main window widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_TOP_DOCK_EDGE_H__
#define __GUI_WIDGETS_TOP_DOCK_EDGE_H__


#include <gtk/gtk.h>
#include <dazzle.h>

#define TOP_DOCK_EDGE_WIDGET_TYPE \
  (top_dock_edge_widget_get_type ())
G_DECLARE_FINAL_TYPE (TopDockEdgeWidget,
                      top_dock_edge_widget,
                      Z,
                      TOP_DOCK_EDGE_WIDGET,
                      GtkBox)

#define MW_TOP_DOCK_EDGE MW_CENTER_DOCK->top_dock_edge

typedef struct _TopDockEdgeWidget
{
  GtkBox                   parent_instance;
  GtkToolbar *             top_toolbar;
  SnapGridWidget *         snap_grid_timeline;
  GtkToggleToolButton *        snap_to_grid;
  GtkToggleToolButton *        snap_to_grid_keep_offset;
  GtkToggleToolButton *        snap_to_events;
  GtkToggleToolButton *          original_size;
  GtkToggleToolButton *          best_fit;
  GtkToggleToolButton *          zoom_out;
  GtkToggleToolButton *          zoom_in;
} TopDockEdgeWidget;

#endif
