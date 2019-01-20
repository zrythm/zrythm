/*
 * utils/ui.h - GTK/GDK utils
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __UTILS_UI_H__
#define __UTILS_UI_H__

#include <gtk/gtk.h>

/**
 * Space on the edges to show resize cursors
 */
#define RESIZE_CURSOR_SPACE 9

/**
 * Various cursor states to be shared.
 */
typedef enum UiCursorState
{
  UI_CURSOR_STATE_DEFAULT,
  UI_CURSOR_STATE_RESIZE_L,
  UI_CURSOR_STATE_REPEAT_L,
  UI_CURSOR_STATE_RESIZE_R,
  UI_CURSOR_STATE_REPEAT_R,
} UiCursorState;

void
ui_set_cursor (GtkWidget * widget, char * name);

void
ui_show_error_message (GtkWindow * parent_window,
                       const char * message);

/**
 * Returns if the child is hit or not by the coordinates in
 * parent.
 */
int
ui_is_child_hit (GtkContainer * parent,
                 GtkWidget *    child,
                 double         x, ///< x in parent space
                 double         y); ///< y in parent space

/**
 * Returns the matching hit child, or NULL.
 */
GtkWidget *
ui_get_hit_child (GtkContainer * parent,
                  double         x, ///< x in parent space
                  double         y, ///< y in parent space
                  GType          type); ///< type to look for

#endif
