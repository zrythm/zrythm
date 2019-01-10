/*
 * gui/widgets/main_window.h - Main window widget
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#ifndef __GUI_WIDGETS_MAIN_WINDOW_H__
#define __GUI_WIDGETS_MAIN_WINDOW_H__

#include "zrythm.h"

#include <gtk/gtk.h>

#define MAIN_WINDOW ZRYTHM->main_window
#define MW MAIN_WINDOW

#define MAIN_WINDOW_WIDGET_TYPE \
  (main_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (MainWindowWidget,
                      main_window_widget,
                      Z,
                      MAIN_WINDOW_WIDGET,
                      GtkApplicationWindow)

typedef struct _HeaderBarWidget HeaderBarWidget;
typedef struct _CenterDockWidget CenterDockWidget;
typedef struct _BotBarWidget BotBarWidget;
typedef struct _TopBarWidget TopBarWidget;

typedef struct _MainWindowWidget
{
  GtkApplicationWindow     parent_instance;
  GtkBox *                 main_box;
  GtkBox *                 header_bar_box;
  HeaderBarWidget *        header_bar;
  TopBarWidget *           top_bar;
  GtkBox *                 center_box;
  CenterDockWidget *       center_dock;
  BotBarWidget *           bot_bar;
  int                      is_maximized;
  GtkAccelGroup *          accel_group;
} MainWindowWidget;

/**
 * Creates a main_window widget using the given main_window data.
 */
MainWindowWidget *
main_window_widget_new (ZrythmApp * app);

void
main_window_widget_refresh (MainWindowWidget * self);

/**
 * TODO
 */
void
main_window_widget_open (MainWindowWidget  * win,
                         GFile             * file);

void
main_window_widget_quit (MainWindowWidget * self);

void
main_window_widget_toggle_maximize (MainWindowWidget * self);

void
main_window_widget_minimize (MainWindowWidget * self);

#endif
