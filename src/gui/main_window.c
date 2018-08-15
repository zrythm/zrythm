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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <dazzle.h>
#include "gui/main_window.h"
#include "gui/widgets/instrument_timeline_view.h"
#include "gui/widgets/ruler.h"
#include "config.h"

void
on_main_window_destroy(GtkWidget *widget, gpointer user_data)
{
    gtk_main_quit();
}

/**
 * Generates main window and returns it
 */
GtkWidget*
create_main_window()
{
  GtkWidget  *window, *instruments_plus_top_panel;
  GtkBuilder *builder;
  GError     *err = NULL;

  // get window from ui file
  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder,
                                 "/online/alextee/zrythm/main-window.ui",
                                 &err);
  if (err != NULL) {
        g_error("Failed loading file: %s", err->message);
        g_error_free(err);
        return NULL;
  }
  window = GTK_WIDGET (gtk_builder_get_object (builder,
                                               "gapplicationwindow-main"));
  if (window == NULL || !GTK_IS_WINDOW(window)) {
        g_error ("Unable to get window. (window == NULL || window != GtkWindow)");
        return NULL;
  }

  // set title to the package string from config.h
  gtk_window_set_title (GTK_WINDOW (window), PACKAGE_STRING);

  // set css
  GtkCssProvider * css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource (css_provider, "/online/alextee/zrythm/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);

  // add the instruments GtkPaned
  instruments_plus_top_panel = GTK_WIDGET (
          gtk_builder_get_object (builder,
                                 "gbox-v-instruments-plus-top-panel"));
  GtkWidget * multi_paned = dzl_multi_paned_new ();
  gtk_box_pack_start (GTK_BOX (instruments_plus_top_panel),
                     multi_paned,
                     1,
                     1,
                     0);

  // add a few test instruments
  set_instrument_timeline_view (multi_paned);
  set_instrument_timeline_view (multi_paned);
  set_instrument_timeline_view (multi_paned);

  // set ruler
  GtkWidget * ruler_drawing_area = GTK_WIDGET (
          gtk_builder_get_object (builder,
                                 "gdrawingarea-ruler"));
  set_ruler (ruler_drawing_area);

  // set signals
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);

  g_object_unref(builder);

  return window;
}


