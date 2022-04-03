/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * @file
 *
 * Snap grid popover.
 */

#ifndef __GUI_WIDGETS_SNAP_GRID_POPOVER_H__
#define __GUI_WIDGETS_SNAP_GRID_POPOVER_H__

#include <gtk/gtk.h>

typedef struct _DigitalMeterWidget DigitalMeterWidget;
typedef struct _SnapGridWidget     SnapGridWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define SNAP_GRID_POPOVER_WIDGET_TYPE \
  (snap_grid_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SnapGridPopoverWidget,
  snap_grid_popover_widget,
  Z,
  SNAP_GRID_POPOVER_WIDGET,
  GtkPopover)

/**
 * Snap grid popover.
 */
typedef struct _SnapGridPopoverWidget
{
  GtkPopover parent_instance;

  /** Owner button. */
  SnapGridWidget * owner;

  /* --- snap --- */
  GtkBox *             snap_length_box;
  DigitalMeterWidget * snap_length_dm;
  GtkBox *             snap_type_box;
  //DigitalMeterWidget * snap_type_dm;
  GtkToggleButton * snap_triplet_toggle;
  gulong            snap_triplet_toggle_handler;
  GtkToggleButton * snap_dotted_toggle;
  gulong            snap_dotted_toggle_handler;
  GtkCheckButton *  snap_adaptive;
  gulong            snap_adaptive_handler;

  /* --- default --- */
  GtkBox *             default_length_box;
  DigitalMeterWidget * default_length_dm;
  GtkBox *             default_type_box;
  //DigitalMeterWidget * default_type_dm;
  GtkToggleButton * default_triplet_toggle;
  gulong            default_triplet_toggle_handler;
  GtkToggleButton * default_dotted_toggle;
  gulong            default_dotted_toggle_handler;
  GtkCheckButton *  default_adaptive;
  gulong            default_adaptive_handler;

  /** Toggle to link snap to default. */
  GtkToggleButton * link_toggle;
  gulong            link_handler;

  /** Toggle to use last object's length. */
  GtkToggleButton * last_object_toggle;
  gulong            last_object_handler;

} SnapGridPopoverWidget;

/**
 * Creates the popover.
 */
SnapGridPopoverWidget *
snap_grid_popover_widget_new (
  SnapGridWidget * owner);

/**
 * @}
 */

#endif
