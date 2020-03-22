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
 * GTK window management for plugin UIs.
 */

#ifndef __PLUGINS_PLUGIN_GTK_H__
#define __PLUGINS_PLUGIN_GTK_H__

#include <gtk/gtk.h>

typedef struct Plugin Plugin;

/**
 * @addtogroup plugins
 *
 * @{
 */

/**
 * Widget for a control.
 */
typedef struct PluginGtkController
{
  GtkSpinButton* spin;
  GtkWidget*     control;

  /** Port this control is for. */
  Port *         port;
} PluginGtkController;

typedef struct PluginGtkPresetMenu
{
  GtkMenuItem* item;
  char*        label;
  GtkMenu*     menu;
  GSequence*   banks;
} PluginGtkPresetMenu;

typedef struct PluginGtkPresetRecord
{
  Plugin*     plugin;
  /** This will be a LilvNode * for LV2. */
  void * preset;
} PluginGtkPresetRecord;

/**
 * Creates a new GtkWindow that will be used to
 * either wrap plugin UIs or create generic UIs in.
 */
void
plugin_gtk_create_window (
  Plugin * plugin);

/**
 * Closes the window of the plugin.
 */
void
plugin_gtk_close_window (
  Plugin * plugin);

void
plugin_gtk_set_window_title (
  Plugin *    plugin,
  GtkWindow * window);

void
plugin_gtk_add_control_row (
  GtkWidget*  table,
  int         row,
  const char* name,
  PluginGtkController* controller);

void
plugin_gtk_on_preset_activate (
  GtkWidget* widget,
  PluginGtkPresetRecord * record);

void
plugin_gtk_on_preset_destroy (
  PluginGtkPresetRecord * record,
  GClosure* closure);

gint
plugin_gtk_menu_cmp (
  gconstpointer a, gconstpointer b, gpointer data);

PluginGtkPresetMenu*
plugin_gtk_preset_menu_new (
  const char* label);

void
plugin_gtk_rebuild_preset_menu (
  Plugin* plugin,
  GtkContainer* pset_menu);

/**
 * Creates a label for a control.
 *
 * @param title Whether this is a title text (makes
 *   it bold).
 * @param preformatted Whether the text is
 *   preformatted.
 */
GtkWidget*
plugin_gtk_new_label (
  const char * text,
  int          title,
  int          preformatted,
  float        xalign,
  float        yalign);

/**
 * @}
 */

#endif
