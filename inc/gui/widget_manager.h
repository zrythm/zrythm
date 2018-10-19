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

#define WIDGET_MANAGER zrythm_app->widget_manager

typedef struct _MainWindowWidget MainWindowWidget;

typedef struct Widget_Manager
{
  //GHashTable   * widgets;
  GtkTargetEntry        entries[10];        ///< dnd entries
  int                   num_entries;        ///< count
  MainWindowWidget      * main_window;      ///< main window
} Widget_Manager;

/**
 * Initializes the widget manager
 */
void
init_widget_manager ();

#endif
