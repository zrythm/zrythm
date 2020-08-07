/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Button with an arrow on the right side for a menu.
 */

#ifndef __GUI_WIDGETS_BUTTON_WITH_MENU_H__
#define __GUI_WIDGETS_BUTTON_WITH_MENU_H__

#include <stdbool.h>

#include <gtk/gtk.h>

#define BUTTON_WITH_MENU_WIDGET_TYPE \
  (button_with_menu_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ButtonWithMenuWidget,
  button_with_menu_widget,
  Z, BUTTON_WITH_MENU_WIDGET,
  GtkButtonBox)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Button with a separator and an arrow for a menu.
 */
typedef struct _ButtonWithMenuWidget
{
  GtkButtonBox     parent_instance;

  GtkBox *         button_box;

  /** True for downward arrow, false for upward
   * arrow. */
  bool             downard_arrow;

  GtkMenuButton *  menu_btn;

  /** The menu to show when arrow is clicked. */
  GtkMenu *        menu;
} ButtonWithMenuWidget;

/**
 * @}
 */

/**
 * This must only be called once to set up the
 * widget.
 */
void
button_with_menu_widget_setup (
  ButtonWithMenuWidget * self,
  GtkButton *            btn,
  GtkWidget *            menu_or_popover,
  bool                   downward_arrow,
  int                    height,
  const char *           btn_tooltip_text,
  const char *           menu_tooltip_text);

#endif
