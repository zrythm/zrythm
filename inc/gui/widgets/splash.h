// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_SPLASH_H__
#define __GUI_WIDGETS_SPLASH_H__

#include <gtk/gtk.h>

#define SPLASH_WINDOW_WIDGET_TYPE (splash_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SplashWindowWidget,
  splash_window_widget,
  Z,
  SPLASH_WINDOW_WIDGET,
  GtkWindow)

typedef struct _ZrythmApp         ZrythmApp;
typedef struct _CustomImageWidget CustomImageWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _SplashWindowWidget
{
  GtkWindow           parent_instance;
  GtkLabel *          status_label;
  GtkLabel *          version_label;
  GtkProgressBar *    progress_bar;
  CustomImageWidget * img;

  guint tick_cb_id;
} SplashWindowWidget;

/**
 * Creates a splash_window widget.
 */
SplashWindowWidget *
splash_window_widget_new (ZrythmApp * app);

void
splash_window_widget_close (SplashWindowWidget * self);

/**
 * @}
 */

#endif
