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
G_DECLARE_FINAL_TYPE (SnapGridPopoverWidget,
                      snap_grid_popover_widget,
                      SNAP_GRID_POPOVER,
                      WIDGET,
                      GtkPopover)
#define IS_SNAP_GRID_POPOVER_WIDGET(obj)               (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SNAP_GRID_POPOVER_WIDGET_TYPE))

typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct _SnapGridWidget SnapGridWidget;

typedef struct _SnapGridPopoverWidget
{
  GtkPopover              parent_instance;
  SnapGridWidget          * owner; ///< the owner
  GtkCheckButton          * grid_adaptive;
  GtkBox *                note_length_box;
  DigitalMeterWidget      * dm_note_length; ///< digital meter for density
  GtkBox *                note_type_box;
  DigitalMeterWidget      * dm_note_type; ///< digital meter for note type
} SnapGridPopoverWidget;

/**
 * Creates the popover.
 */
SnapGridPopoverWidget *
snap_grid_popover_widget_new (SnapGridWidget * owner);

#endif
