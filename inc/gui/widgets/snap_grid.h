/*
 * inc/gui/widgets/snap_grid.h - Snap and grid selection widget
 *
 * Copyright (C) 2018 Alexandros Theodotou
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_SNAP_GRID_H__
#define __GUI_WIDGETS_SNAP_GRID_H__

#include <gtk/gtk.h>

#define SNAP_GRID_WIDGET_TYPE                  (snap_grid_widget_get_type ())
#define SNAP_GRID_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SNAP_GRID_WIDGET_TYPE, SnapGridWidget))
#define SNAP_GRID_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), SNAP_GRID_WIDGET, SnapGridWidgetClass))
#define IS_SNAP_GRID_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNAP_GRID_WIDGET_TYPE))
#define IS_SNAP_GRID_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), SNAP_GRID_WIDGET_TYPE))
#define SNAP_GRID_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), SNAP_GRID_WIDGET_TYPE, SnapGridWidgetClass))

typedef struct SnapGridPopoverWidget SnapGridPopoverWidget;

typedef struct SnapGridWidget
{
  GtkMenuButton           parent_instance;
  GtkBox *                box; ///< the box
  GtkImage *              img; ///< img to show next to the label
  GtkLabel                * label; ///< label to show
  SnapGridPopoverWidget   * popover; ///< the popover to show
  GtkBox                  * content; ///< popover content holder
  SnapGrid                * snap_grid;
} SnapGridWidget;

typedef struct SnapGridWidgetClass
{
  GtkMenuButtonClass       parent_class;
} SnapGridWidgetClass;

typedef struct SnapGrid SnapGrid;

/**
 * Creates a digital meter with the given type (bpm or position).
 */
SnapGridWidget *
snap_grid_widget_new (SnapGrid * snap_grid);

#endif


