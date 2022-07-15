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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_SNAP_GRID_H__
#define __GUI_WIDGETS_SNAP_GRID_H__

#include <gtk/gtk.h>

#define SNAP_GRID_WIDGET_TYPE (snap_grid_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SnapGridWidget,
  snap_grid_widget,
  Z,
  SNAP_GRID_WIDGET,
  GtkBox)

typedef struct _SnapGridPopoverWidget SnapGridPopoverWidget;
typedef struct SnapGrid               SnapGrid;

typedef struct _SnapGridWidget
{
  GtkBox parent_instance;

  GtkMenuButton * menu_btn;

  /** The box. */
  GtkBox * box;

  /** Image to show next to the label. */
  GtkImage * img;

  /** Label to show. */
  GtkLabel * label;

  /** Popover. */
  SnapGridPopoverWidget * popover;

  /** Popover content holder. */
  GtkBox * content;

  /** Associated snap grid options. */
  SnapGrid * snap_grid;
} SnapGridWidget;

void
snap_grid_widget_setup (
  SnapGridWidget * self,
  SnapGrid *       snap_grid);

void
snap_grid_widget_refresh (SnapGridWidget * self);

#endif
