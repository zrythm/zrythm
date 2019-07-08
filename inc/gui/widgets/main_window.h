/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_MAIN_WINDOW_H__
#define __GUI_WIDGETS_MAIN_WINDOW_H__

#include "zrythm.h"

#include <gtk/gtk.h>

/**
 * @defgroup widgets Widgets
 *
 * Widgets are custom GUI elements that reflect the
 * backend.
 *
 * @{
 */

#define MAIN_WINDOW ZRYTHM->main_window
#define MW MAIN_WINDOW

#define MAIN_WINDOW_WIDGET_TYPE \
  (main_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (MainWindowWidget,
                      main_window_widget,
                      Z,
                      MAIN_WINDOW_WIDGET,
                      GtkApplicationWindow)

typedef struct _HeaderNotebookWidget
  HeaderNotebookWidget;
typedef struct _CenterDockWidget CenterDockWidget;
typedef struct _BotBarWidget BotBarWidget;
typedef struct _TopBarWidget TopBarWidget;

/**
 * The main window of Zrythm.
 *
 * Inherits from GtkApplicationWindow, meaning that
 * it is the parent of all other sub-windows of
 * Zrythm.
 */
typedef struct _MainWindowWidget
{
  GtkApplicationWindow     parent_instance;
  GtkBox *                 main_box;
  HeaderNotebookWidget *   header_notebook;
  TopBarWidget *           top_bar;
  GtkBox *                 center_box;
  CenterDockWidget *       center_dock;
  BotBarWidget *           bot_bar;
  int                      is_maximized;
  int                      height;
  int                      width;
  GtkAccelGroup *          accel_group;
  GtkRevealer *            revealer;
  GtkButton *              close_notification_button;
  GtkLabel *               notification_label;

  /**
   * Last focused widget.
   *
   * Used to check what area is focused when doing copy
   * paste, etc. Can either be Arranger or piano roll.
   */
  GtkWidget *              last_focused;
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

/**
 * @}
 */

#endif
