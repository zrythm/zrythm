/*
 * gui/init.cpp - GUI initialization logic
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "gui/main_window.h"
#include "config.h"

void on_main_window_destroy(GtkWidget *widget, gpointer user_data)
{
    gtk_main_quit();
}

/**
 * Generates main window and returns it
 */
GtkWidget*
create_main_window()
{
  GtkWidget  *window;
  GtkBuilder *builder;
  GError     *err = NULL;

  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder,
                             "/online/alextee/zrythm/main-window.ui",
                             &err);

  if (err != NULL) {
        g_error("Failed loading file: %s", err->message);
        g_error_free(err);
        return;
    }

  window = GTK_WIDGET (gtk_builder_get_object (builder,
                                               "main_window"));
  if (window == NULL || !GTK_IS_WINDOW(window)) {
        g_error ("Unable to get window. (window == NULL || window != GtkWindow)");
        return NULL;
    }

  // use package string from config.h
  gtk_window_set_title (GTK_WINDOW (window), PACKAGE_STRING);

  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_object_unref(builder);

  return window;
}


