/*
 * gui/widgets/main_window.c - the main window
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

#include "config.h"
#include "audio/mixer.h"
#include "gui/widgets/browser.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/instrument_timeline_view.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MainWindowWidget, main_window_widget, GTK_TYPE_APPLICATION_WINDOW)

static gboolean is_maximized = 0;
static GApplication *app;

static void
on_main_window_destroy (MainWindowWidget * self,
                        gpointer user_data)
{
    g_application_quit (app);
}

static void
on_state_changed (MainWindowWidget * self,
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
static void
close_clicked (MainWindowWidget * self,
                      gpointer user_data)
{
    g_application_quit (app);
}

/**
 * minimize button event
 */
static void
minimize_clicked (MainWindowWidget * self,
                      gpointer user_data)
{
    gtk_window_iconify (GTK_WINDOW (user_data));
}

/**
 * maximize button event
 */
static void
maximize_clicked (MainWindowWidget * self,
                      gpointer user_data)
{
    if (is_maximized)
    {
        gtk_window_unmaximize (GTK_WINDOW (user_data));
        is_maximized = 0;
    }
    else
        gtk_window_maximize (GTK_WINDOW (user_data));
}

static void
main_window_widget_class_init (MainWindowWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (GTK_WIDGET_CLASS (klass),
                                               "/online/alextee/zrythm/ui/main-window.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, main_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, top_bar);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, top_menubar);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, file);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, edit);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, view);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, help);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, window_buttons);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, minimize);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, maximize);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, close);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, top_toolbar);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, center_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, inspector);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, inspector_button);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, editor_plus_browser);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, editor);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, editor_top);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, tracks_timeline);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, tracks_top);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, tracks_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, tracks_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, tracks_paned);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, tracks_header);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, timeline_ruler);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, ruler_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, ruler_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, timeline_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, timeline_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, timeline_overlay);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, instruments_toolbar);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, instrument_add);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, editor_bot);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, mixer);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, channels_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, channels_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, channels);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, channels_add);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, browser_notebook);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, browser);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, browser_top);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, browser_search);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, collections_exp);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, types_exp);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, cat_exp);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, browser_bot);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, bot_bar);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, bot_bar_left);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, transport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, bpm);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, play);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, stop);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, backward);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, forward);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, trans_record);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           on_main_window_destroy);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           on_state_changed);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           minimize_clicked);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           close_clicked);
  gtk_widget_class_bind_template_callback (GTK_WIDGET_CLASS (klass),
                                           maximize_clicked);

}

static void
main_window_widget_init (MainWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

void
main_window_widget_open (MainWindowWidget *win,
                         GFile            *file)
{
}


MainWindowWidget *
main_window_widget_new (ZrythmApp * _app)
{
  app = G_APPLICATION (_app);
  MainWindowWidget * self = g_object_new (MAIN_WINDOW_WIDGET_TYPE,
                                          "application",
                                          app,
                                          NULL);

  gtk_window_set_title (GTK_WINDOW (self), PACKAGE_STRING);

  // set icons
  GtkWidget * image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/z.svg");
  gtk_window_set_icon (
          GTK_WINDOW (self),
          gtk_image_get_pixbuf (GTK_IMAGE (image)));
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/close.svg");
  gtk_button_set_image (self->close, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/minimize.svg");
  gtk_button_set_image (self->minimize, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/maximize.svg");
  gtk_button_set_image (self->maximize, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/play.svg");
  gtk_button_set_image (self->play, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/stop.svg");
  gtk_button_set_image (self->stop, image);
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/plus.svg");
  gtk_tool_button_set_icon_widget (self->instrument_add,
                                   GTK_WIDGET (image));
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/record.svg");
  gtk_button_set_image (GTK_BUTTON (self->trans_record), image);

  // set default css provider
  GtkCssProvider * css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/online/alextee/zrythm/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);

  // add a few test instruments
  init_ins_timeline_view (GTK_WIDGET (self->tracks_paned));
  GtkWidget * paned1 = add_instrument (GTK_WIDGET (self->tracks_paned));
  GtkWidget * paned2 = add_instrument (paned1);
  GtkWidget * paned3 = add_instrument (paned2);
  gtk_widget_set_size_request (GTK_WIDGET (self->tracks_header),
                               -1,
                               60);
  gtk_widget_show_all (GTK_WIDGET (self->editor_top));

  // set ruler
  self->ruler = ruler_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->ruler_viewport),
                     GTK_WIDGET (self->ruler));
  gtk_scrolled_window_set_hadjustment (self->ruler_scroll,
                                       gtk_scrolled_window_get_hadjustment (
                                           GTK_SCROLLED_WINDOW (
                                               self->timeline_scroll)));
  gtk_widget_show_all (GTK_WIDGET (self->ruler_viewport));

  /* set timeline */
  self->timeline = timeline_widget_new (GTK_WIDGET (self->tracks_paned),
                                        GTK_WIDGET (self->timeline_overlay));
  gtk_scrolled_window_set_min_content_width (self->timeline_scroll, 400);
  gtk_scrolled_window_set_vadjustment (self->timeline_scroll,
            gtk_scrolled_window_get_vadjustment (self->tracks_scroll));
  gtk_widget_show_all (GTK_WIDGET (self->timeline_viewport));

  /* set browser  pass it */
  setup_browser (GTK_WIDGET (self->browser),
                 GTK_WIDGET (self->collections_exp),
                 GTK_WIDGET (self->types_exp),
                 GTK_WIDGET (self->cat_exp),
                 GTK_WIDGET (self->browser_bot));
  gtk_widget_show_all (GTK_WIDGET (self->browser_notebook));

  /* setup mixer */
  mixer_setup (self->mixer, self->channels);

  return self;
}

