/*
 * Copyright (C) 2018-2019 Alexandros Theodotou
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

/**
 * \file
 *
 * GTK utils.
 * TODO merge with UI
 */

#ifndef __UTILS_GTK_H__
#define __UTILS_GTK_H__

#include <gtk/gtk.h>

#define DESTROY_LATER(x) \
  g_idle_add ( \
    (GSourceFunc) z_gtk_widget_destroy_idle, \
      GTK_WIDGET (x));

#define DEFAULT_CLIPBOARD \
  gtk_clipboard_get_default ( \
    gdk_display_get_default ())

#define CREATE_CUT_MENU_ITEM \
  z_gtk_create_menu_item ( \
    "Cu_t", \
    "z-edit-cut", \
    0, \
    NULL, \
    0, \
    "win.cut")

#define CREATE_COPY_MENU_ITEM \
  z_gtk_create_menu_item ( \
    "_Copy", \
    "z-edit-copy", \
    0, \
    NULL, \
    0, \
    "win.copy")

#define CREATE_PASTE_MENU_ITEM \
  z_gtk_create_menu_item ( \
    "_Paste", \
    "z-edit-paste", \
    0, \
    NULL, \
    0, \
    "win.paste")

#define CREATE_DELETE_MENU_ITEM \
  z_gtk_create_menu_item ( \
    "_Delete", \
    "z-edit-delete", \
    0, \
    NULL, \
    0, \
    "win.delete")

#define CREATE_CLEAR_SELECTION_MENU_ITEM \
  z_gtk_create_menu_item ( \
    "Cle_ar Selection", \
    "z-edit-clear", \
    0, \
    NULL, \
    0, \
    "win.clear-selection")

#define CREATE_SELECT_ALL_MENU_ITEM \
  z_gtk_create_menu_item ( \
    "Select A_ll", \
    "z-edit-select-all", \
    0, \
    NULL, \
    0, \
    "win.select-all")

#define CREATE_DUPLICATE_MENU_ITEM \
  z_gtk_create_menu_item ( \
    "Duplicate", \
    "z-document-duplicate", \
    0, \
    NULL, \
    0, \
    "win.duplicate")

#define z_gtk_assistant_set_current_page_complete( \
  assistant, complete) \
  gtk_assistant_set_page_complete ( \
    GTK_ASSISTANT (assistant), \
    gtk_assistant_get_nth_page ( \
      GTK_ASSISTANT (assistant), \
      gtk_assistant_get_current_page ( \
        GTK_ASSISTANT (assistant))), \
    complete);

typedef enum IconType IconType;
/**
 * For readability, instead of using 0s and 1s.
 */
enum ZGtkFill
{
  Z_GTK_NO_FILL,
  Z_GTK_FILL
};

enum ZGtkResize
{
  Z_GTK_NO_RESIZE,
  Z_GTK_RESIZE
};

enum ZGtkExpand
{
  Z_GTK_NO_EXPAND,
  Z_GTK_EXPAND
};

enum ZGtkShrink
{
  Z_GTK_NO_SHRINK,
  Z_GTK_SHRINK
};

int
z_gtk_widget_destroy_idle (
  GtkWidget * widget);

void
z_gtk_container_remove_all_children (
  GtkContainer * container);

void
z_gtk_container_destroy_all_children (
  GtkContainer * container);

void
z_gtk_container_remove_children_of_type (
  GtkContainer * container,
  GType          type);

void
z_gtk_overlay_add_if_not_exists (
  GtkOverlay * overlay,
  GtkWidget *  widget);

void
z_gtk_button_set_icon_name (GtkButton * btn,
                            const char * name);

/**
 * Creates a button with the given icon name.
 */
GtkButton *
z_gtk_button_new_with_icon (const char * name);

/**
 * Creates a toggle button with the given icon name.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_icon (const char * name);

/**
 * Creates a button with the given resource name as icon.
 */
GtkButton *
z_gtk_button_new_with_resource (IconType  icon_type,
                                const char * name);

/**
 * Creates a toggle button with the given resource name as
 * icon.
 */
GtkToggleButton *
z_gtk_toggle_button_new_with_resource (
  IconType  icon_type,
  const char * name);

GtkMenuItem *
z_gtk_create_menu_item (gchar *     label_name,
                  gchar *         icon_name,
                  IconType        resource_icon_type,
                  gchar *         resource,
                  int             is_toggle,
                  const char *    action_name);

#endif
