/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#include <gtksourceview/gtksource.h>
#pragma GCC diagnostic pop

#define LOG_VIEWER_WIDGET_TYPE (log_viewer_widget_get_type ())
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
