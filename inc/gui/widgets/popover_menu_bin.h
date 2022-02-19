/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * Plugin browser.
 */

#ifndef __GUI_WIDGETS_POPOVER_MENU_BIN_H__
#define __GUI_WIDGETS_POPOVER_MENU_BIN_H__

#include <gtk/gtk.h>

#define POPOVER_MENU_BIN_WIDGET_TYPE \
  (popover_menu_bin_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PopoverMenuBinWidget, popover_menu_bin_widget,
  Z, POPOVER_MENU_BIN_WIDGET, GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Container for widgets that wish to have a right
 * click menu.
 */
typedef struct _PopoverMenuBinWidget
{
  GtkWidget        parent_instance;

  GtkWidget *      child;

  GMenuModel *     menu_model;

  /** Context menu. */
  GtkPopoverMenu * popover_menu;
} PopoverMenuBinWidget;

GMenuModel *
popover_menu_bin_widget_get_menu_model (
  PopoverMenuBinWidget * self);

void
popover_menu_bin_widget_set_menu_model (
  PopoverMenuBinWidget * self,
  GMenuModel *           model);

void
popover_menu_bin_widget_set_child (
  PopoverMenuBinWidget * self,
  GtkWidget *            child);

GtkWidget *
popover_menu_bin_widget_get_child (
  PopoverMenuBinWidget * self);

PopoverMenuBinWidget *
popover_menu_bin_widget_new (void);

/**
 * @}
 */

#endif
