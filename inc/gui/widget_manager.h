/*
 * gui/widget_manager.h - Manages GUI widgets
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

#ifndef __GUI_WIDGET_MANAGER_H__
#define __GUI_WIDGET_MANAGER_H__

#include "project.h"

#include <gtk/gtk.h>

/**
 * pixels to draw between each beat,
 * before being adjusted for zoom.
 * used by the ruler and timeline
 */
#define PX_PER_BEAT 20

/**
 * Convenience macro to get any registered widget by ID.
 * The widgets must first be registered
 */
#define GET_WIDGET(key) g_hash_table_lookup ( \
                      project->widget_manager->widgets, \
                      key)

typedef struct _Widget_Manager
{
  GHashTable   * widgets;
} Widget_Manager;

/**
 * Initializes the widget manager
 */
void
init_widget_manager ();

/**
 * Registers widgets to the widget manager
 */
void
register_widgets ();

/**
 * Registers a widget
 */
void
register_widget_from_builder (GtkBuilder * builder,
                              gchar      * key);

#endif
