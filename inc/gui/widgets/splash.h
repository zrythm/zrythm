/*
 * gui/widgets/splash.h - Splash window
 *
 * Copyright (C) 2019 Alexandros Theodotou
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

#include "zrythm.h"

#include <gtk/gtk.h>

#define SPLASH_WINDOW_WIDGET_TYPE \
  (splash_window_widget_get_type ())
G_DECLARE_FINAL_TYPE (SplashWindowWidget,
                      splash_window_widget,
                      Z,
                      SPLASH_WINDOW_WIDGET,
                      GtkWindow)

typedef struct _SplashWindowWidget
{
  GtkWindow     parent_instance;
  GtkLabel                 * label;
  GtkProgressBar           * progress_bar;
} SplashWindowWidget;

/**
 * Creates a splash_window widget using the given splash_window data.
 */
SplashWindowWidget *
splash_window_widget_new (ZrythmApp * app);

void
splash_widget_update (SplashWindowWidget * self,
                      const char         * message,
                      gdouble            progress);

#endif

