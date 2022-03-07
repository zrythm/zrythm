/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_SPLASH_H__
#define __GUI_WIDGETS_SPLASH_H__

#include <gtk/gtk.h>

#define SPLASH_WINDOW_WIDGET_TYPE \
  (splash_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SplashWindowWidget,
  splash_window_widget,
  Z, SPLASH_WINDOW_WIDGET,
  GtkWindow)

typedef struct _ZrythmApp ZrythmApp;
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

  guint               tick_cb_id;
} SplashWindowWidget;

/**
 * Creates a splash_window widget.
 */
SplashWindowWidget *
splash_window_widget_new (
  ZrythmApp * app);

void
splash_window_widget_close (
  SplashWindowWidget * self);

/**
 * @}
 */

#endif
