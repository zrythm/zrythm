/*
 * inc/gui/widgets/snap_grid_popover_popover.h - Snap and grid_popover popover widget
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

#ifndef __GUI_WIDGETS_SNAP_GRID_POPOVER_H__
#define __GUI_WIDGETS_SNAP_GRID_POPOVER_H__


#include <gtk/gtk.h>

#define SNAP_GRID_POPOVER_WIDGET_TYPE                  (snap_grid_popover_widget_get_type ())
#define SNAP_GRID_POPOVER_WIDGET(obj)                  (G_TYPE_CHECK_INSTANCE_CAST ((obj), SNAP_GRID_POPOVER_WIDGET_TYPE, SnapGridPopoverWidget))
#define SNAP_GRID_POPOVER_WIDGET_CLASS(klass)          (G_TYPE_CHECK_CLASS_CAST  ((klass), SNAP_GRID_POPOVER_WIDGET, SnapGridPopoverWidgetClass))
#define IS_SNAP_GRID_POPOVER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNAP_GRID_POPOVER_WIDGET_TYPE))
#define IS_SNAP_GRID_POPOVER_WIDGET_CLASS(klass)       (G_TYPE_CHECK_CLASS_TYPE  ((klass), SNAP_GRID_POPOVER_WIDGET_TYPE))
#define SNAP_GRID_POPOVER_WIDGET_GET_CLASS(obj)        (G_TYPE_INSTANCE_GET_CLASS  ((obj), SNAP_GRID_POPOVER_WIDGET_TYPE, SnapGridPopoverWidgetClass))

typedef struct DigitalMeterWidget DigitalMeterWidget;
typedef struct _SnapGridWidget SnapGridWidget;

typedef struct SnapGridPopoverWidget
{
  GtkPopover              parent_instance;
  SnapGridWidget          * owner; ///< the owner
  GtkCheckButton          * grid_adaptive;
  GtkBox *                density_box;
  DigitalMeterWidget      * dm_density; ///< digital meter for density
  GtkCheckButton *        snap_grid;
  GtkCheckButton *        snap_offset;
  GtkCheckButton *        snap_events;
} SnapGridPopoverWidget;

typedef struct SnapGridPopoverWidgetClass
{
  GtkPopoverClass       parent_class;
} SnapGridPopoverWidgetClass;

typedef struct SnapGridPopover SnapGridPopover;

/**
 * Creates the popover.
 */
SnapGridPopoverWidget *
snap_grid_popover_widget_new (SnapGridWidget * owner);

#endif

