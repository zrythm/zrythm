/*
 * gui/init.c - GUI initialization logic
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

#include "zrythm_system.h"
#include "audio/mixer.h"
#include "gui/main_window.h"
#include "gui/widget_manager.h"
#include "gui/widgets/instrument_timeline_view.h"
#include "gui/widgets/browser.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/knob.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline.h"
#include "config.h"

gboolean is_maximized = 0;

void
on_main_window_destroy (GtkWidget *widget,
                        gpointer user_data)
{
    gtk_main_quit ();
}

void
on_state_changed (GtkWidget * widget,
                  GdkEventWindowState  * event,
                  gpointer    user_data)
{
    if (event->new_window_state &
            GDK_WINDOW_STATE_MAXIMIZED)
        is_maximized = 1;
}

/**
 * close button event
 */
void
close_clicked (GtkButton * button,
                      gpointer user_data)
{
    gtk_main_quit ();
}

/**
 * minimize button event
 */
void
minimize_clicked (GtkButton * button,
                      gpointer user_data)
{
    GtkWindow * window = GTK_WINDOW (user_data);
    gtk_window_iconify (window);
}

/**
 * maximize button event
 */
void
maximize_clicked (GtkButton * button,
                      gpointer user_data)
{
    GtkWindow * window = GTK_WINDOW (user_data);

    if (is_maximized)
    {
        gtk_window_unmaximize (window);
        is_maximized = 0;
    }
    else
        gtk_window_maximize (window);
}

/**
 * Generates main window and returns it
 */
GtkWidget*
create_main_window()
{
  GtkBuilder *builder;
  GError     *err = NULL;

  g_message ("Reading UI file...");
  builder = gtk_builder_new ();
  gtk_builder_add_from_resource (builder,
                                 "/online/alextee/zrythm/main-window.ui",
                                 &err);
  if (err != NULL) {
        g_error("Failed loading file: %s", err->message);
        g_error_free(err);
        return NULL;
  }

  g_message ("Registering widgets...");
  register_widgets (builder);

  GtkWidget * window = GET_WIDGET ("gapplicationwindow-main");
  if (window == NULL || !GTK_IS_WINDOW(window)) {
        g_error ("Unable to get window. (window == NULL || window != GtkWindow)");
        return NULL;
  }

  // set title to the package string from config.h
  gtk_window_set_title (GTK_WINDOW (window), PACKAGE_STRING);

  // set icon
  gtk_window_set_icon (
          GTK_WINDOW (window),
          gtk_image_get_pixbuf (
              GTK_IMAGE (gtk_image_new_from_resource (
                  "/online/alextee/zrythm/z.svg"))));

  // set icons
  GtkWidget * image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/close.svg");
  GtkWidget * button = GET_WIDGET ("gbutton-close-window");
  gtk_button_set_image( GTK_BUTTON (button), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/minimize.svg");
  button = GET_WIDGET ("gbutton-minimize-window");
  gtk_button_set_image( GTK_BUTTON (button), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/maximize.svg");
  button = GET_WIDGET ("gbutton-maximize-window");
  gtk_button_set_image( GTK_BUTTON (button), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/play.svg");
  button = GET_WIDGET ("gbutton-play");
  gtk_button_set_image( GTK_BUTTON (button), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/stop.svg");
  button = GET_WIDGET ("gbutton-stop");
  gtk_button_set_image( GTK_BUTTON (button), image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/plus.svg");
  button = GET_WIDGET ("gtoolbutton-add-instrument");
  gtk_tool_button_set_icon_widget(
            GTK_TOOL_BUTTON (button),
            image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/record.svg");
  button = GET_WIDGET ("gtogglebutton-record");
  gtk_button_set_image( GTK_BUTTON (button), image);


  // set css
  GtkCssProvider * css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource (css_provider, "/online/alextee/zrythm/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);

  // add a few test instruments
  GtkWidget * gpaned_instruments = GET_WIDGET ("gpaned-instruments");
  init_ins_timeline_view (gpaned_instruments);
  GtkWidget * paned1 = add_instrument (gpaned_instruments);
  GtkWidget * paned2 = add_instrument (paned1);
  GtkWidget * paned3 = add_instrument (paned2);

  // set ruler
  GtkWidget * ruler_drawing_area = GET_WIDGET ("gdrawingarea-ruler");
  GtkWidget * timeline_drawing_area = GET_WIDGET ("gdrawingarea-timeline");
  GtkWidget * scrollwindow_ruler = GET_WIDGET ("gscrolledwindow-ruler");
  GtkWidget * ruler_viewport = GET_WIDGET ("gviewport-ruler");
  GtkWidget * timeline_viewport = GET_WIDGET ("gviewport-timeline");
  GtkWidget * timeline_overlay = GET_WIDGET ("goverlay-timeline");
  GtkWidget * scrollwindow_timeline = GET_WIDGET ("gscrolledwindow-timeline");
  set_ruler (ruler_drawing_area);
  gtk_scrolled_window_set_hadjustment (GTK_SCROLLED_WINDOW (scrollwindow_ruler),
                                       gtk_scrolled_window_get_hadjustment (
                                           GTK_SCROLLED_WINDOW (
                                               scrollwindow_timeline)));

  /* set timeline */
  set_timeline (gpaned_instruments,
                timeline_overlay,
                timeline_drawing_area);
  gtk_scrolled_window_set_min_content_width (GTK_SCROLLED_WINDOW (scrollwindow_timeline),
                                             400);
  GtkWidget * gscrollwindow_instruments =
    GET_WIDGET ("gscrolledwindow-instruments");
  gtk_scrolled_window_set_vadjustment (
    GTK_SCROLLED_WINDOW (scrollwindow_timeline),
    gtk_scrolled_window_get_vadjustment (
      GTK_SCROLLED_WINDOW (gscrollwindow_instruments)));

  /* set browser */
  setup_browser ();

  /* set master channel */
  gtk_box_pack_end (GTK_BOX (GET_WIDGET ("gbox-mixer")),
                    GTK_WIDGET (channel_widget_new (MIXER->master)),
                    0, 0, 0);

  // set signals
  g_signal_connect (window, "destroy",
                    G_CALLBACK (gtk_main_quit), NULL);
  g_signal_connect (window, "window_state_event",
                    G_CALLBACK (on_state_changed), NULL);
  gtk_builder_connect_signals( builder, NULL );

  g_object_unref(builder);

  return window;
}


