/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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
 * Wrapper for a FileBrowserWidget.
 */

#ifndef __GUI_WIDGETS_FILE_BROWSER_WINDOW_WINDOW_H__
#define __GUI_WIDGETS_FILE_BROWSER_WINDOW_WINDOW_H__

#include <gtk/gtk.h>

#define FILE_BROWSER_WINDOW_WIDGET_TYPE (file_browser_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  FileBrowserWindowWidget,
  file_browser_window_widget,
  Z,
  FILE_BROWSER_WINDOW_WINDOW_WIDGET,
  GtkWindow)

typedef struct _FileBrowserWidget FileBrowserWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * The file browser window.
 */
typedef struct _FileBrowserWindowWidget
{
  GtkWindow parent_instance;

  FileBrowserWidget * file_browser;
} FileBrowserWindowWidget;

/**
 * Creates a new FileBrowserWindowWidget.
 */
FileBrowserWindowWidget *
file_browser_window_widget_new (void);

/**
 * @}
 */

#endif
