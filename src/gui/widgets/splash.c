/*
 * gui/widgets/splash.c - Splash window
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

#include "gui/widgets/splash.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (SplashWindowWidget, splash_window_widget,
               GTK_TYPE_WINDOW)

static void
splash_window_widget_class_init (SplashWindowWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
                          GTK_WIDGET_CLASS (klass),
                          "/org/zrythm/ui/splash.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        SplashWindowWidget,
                                        label);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        SplashWindowWidget,
                                        progress_bar);
}

static void
splash_window_widget_init (SplashWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}


SplashWindowWidget *
splash_window_widget_new (ZrythmApp * app)
{
  SplashWindowWidget * self = g_object_new (SPLASH_WINDOW_WIDGET_TYPE,
                                            "application",
                                            app,
                                            NULL);
  gtk_progress_bar_set_fraction (self->progress_bar,
                                 0.0);
  return self;
}

void
splash_widget_update (SplashWindowWidget * self,
                      const char         * message,
                      gdouble            progress)
{
  gtk_label_set_text (self->label,
                      message);
  gtk_progress_bar_set_fraction (self->progress_bar,
                                 progress);
}


