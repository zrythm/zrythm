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

#ifndef __GUI_WIDGETS_ACTIVE_HARDWARE_POPOVER_H__
#define __GUI_WIDGETS_ACTIVE_HARDWARE_POPOVER_H__

#include <gtk/gtk.h>

#define ACTIVE_HARDWARE_POPOVER_WIDGET_TYPE \
  (active_hardware_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ActiveHardwarePopoverWidget,
  active_hardware_popover_widget,
  Z,
  ACTIVE_HARDWARE_POPOVER_WIDGET,
  GtkPopover)

typedef struct _ActiveHardwareMbWidget ActiveHardwareMbWidget;

typedef struct _ActiveHardwarePopoverWidget
{
  GtkPopover               parent_instance;
  ActiveHardwareMbWidget * owner; ///< the owner
  GtkBox *                 controllers_box;
  GtkButton *              rescan;
} ActiveHardwarePopoverWidget;

/**
 * Creates the popover.
 */
ActiveHardwarePopoverWidget *
active_hardware_popover_widget_new (
  ActiveHardwareMbWidget * owner);

#endif
