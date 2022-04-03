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
 * Log viewer.
 */

#ifndef __GUI_WIDGETS_LOG_VIEWER_H__
#define __GUI_WIDGETS_LOG_VIEWER_H__

#include "zrythm-config.h"

#include <gtk/gtk.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored \
  "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

#define LOG_VIEWER_WIDGET_TYPE \
  (log_viewer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  LogViewerWidget,
  log_viewer_widget,
  Z,
  LOG_VIEWER_WIDGET,
  GtkWindow)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Log viewer.
 */
typedef struct _LogViewerWidget
{
  GtkWindow parent_instance;

  GtkScrolledWindow * scrolled_win;
  GtkBox *            source_view_box;
  GtkSourceView *     src_view;

} LogViewerWidget;

/**
 * Creates a log viewer widget.
 */
LogViewerWidget *
log_viewer_widget_new (void);

/**
 * @}
 */

#endif
