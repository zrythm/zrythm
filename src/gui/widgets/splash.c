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

#include "gui/widgets/splash.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (SplashWindowWidget,
               splash_window_widget,
               GTK_TYPE_WINDOW)


SplashWindowWidget *
splash_window_widget_new (ZrythmApp * app)
{
  SplashWindowWidget * self =
    g_object_new (SPLASH_WINDOW_WIDGET_TYPE,
                  "application", app,
                  "visible", 1,
                  NULL);
  gtk_progress_bar_set_fraction (self->progress_bar,
                                 0.0);
  gtk_window_set_keep_below (
    GTK_WINDOW (self), 1);

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

static void
splash_window_widget_class_init (
  SplashWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "splash.ui");

  gtk_widget_class_bind_template_child (
    klass,
    SplashWindowWidget,
    label);
  gtk_widget_class_bind_template_child (
    klass,
    SplashWindowWidget,
    progress_bar);
}

static void
splash_window_widget_init (SplashWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

