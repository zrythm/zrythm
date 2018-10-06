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
#include "audio/transport.h"
#include "gui/widgets/bpm.h"
#include "gui/widgets/browser.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/instrument_timeline_view.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/timeline.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/tracks.h"
#include "gui/widgets/transport_controls.h"

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

static gboolean
key_release_cb (GtkWidget      * widget,
                 GdkEventKey * event,
                 gpointer       data)
{
  if (event && event->keyval == GDK_KEY_space)
    {
      if (TRANSPORT->play_state == PLAYSTATE_ROLLING)
        transport_request_pause ();
      else if (TRANSPORT->play_state == PLAYSTATE_PAUSED)
        transport_request_roll ();
    }

  return FALSE;
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
  /*gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),*/
                                                /*MainWindowWidget, tracks_paned);*/
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
                                                MainWindowWidget, plugin_info);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, bot_bar);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, bot_bar_left);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, digital_meters);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, transport);
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
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MainWindowWidget, loop);
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

void
popup_menu(GtkAction *action, GtkMenu *menu)
{
    gtk_menu_popup(menu, NULL, NULL, NULL, NULL, 1, gtk_get_current_event_time());
}


MainWindowWidget *
main_window_widget_new (ZrythmApp * _app)
{
  app = G_APPLICATION (_app);
  MainWindowWidget * self = g_object_new (MAIN_WINDOW_WIDGET_TYPE,
                                          "application",
                                          app,
                                          NULL);
  WIDGET_MANAGER->main_window = self;
  gtk_window_set_title (GTK_WINDOW (self), PACKAGE_STRING);


  // set default css provider
  GtkCssProvider * css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/online/alextee/zrythm/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);


  /* setup tracks */
  tracks_widget_setup ();
  gtk_container_add (GTK_CONTAINER (self->tracks_viewport),
                     GTK_WIDGET (self->tracks));
  gtk_widget_set_size_request (GTK_WIDGET (self->tracks_header),
                               -1,
                               60);
  gtk_widget_show_all (GTK_WIDGET (self->editor_top));

  /* setup ruler */
  self->ruler = ruler_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->ruler_viewport),
                     GTK_WIDGET (self->ruler));
  gtk_scrolled_window_set_hadjustment (self->ruler_scroll,
                                       gtk_scrolled_window_get_hadjustment (
                                           GTK_SCROLLED_WINDOW (
                                               self->timeline_scroll)));
  gtk_widget_show_all (GTK_WIDGET (self->ruler_viewport));

  /* setup timeline */
  self->timeline = timeline_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->timeline_viewport),
                     GTK_WIDGET (self->timeline));
  gtk_scrolled_window_set_min_content_width (self->timeline_scroll, 400);
  gtk_scrolled_window_set_vadjustment (self->timeline_scroll,
            gtk_scrolled_window_get_vadjustment (self->tracks_scroll));
  gtk_widget_show_all (GTK_WIDGET (self->timeline_viewport));
  gtk_widget_show_all (GTK_WIDGET (self->timeline));
  gtk_widget_show_all (GTK_WIDGET (self->timeline->bg));

  /* setup browser */
  gtk_label_set_xalign (self->plugin_info, 0);
  GTK_LABEL (self->plugin_info);
  setup_browser (GTK_WIDGET (self->browser),
                 GTK_WIDGET (self->collections_exp),
                 GTK_WIDGET (self->types_exp),
                 GTK_WIDGET (self->cat_exp),
                 GTK_WIDGET (self->browser_bot));
  gtk_widget_show_all (GTK_WIDGET (self->browser_notebook));

  /* setup mixer */
  mixer_setup (self->mixer, self->channels);

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
          "/online/alextee/zrythm/plus.svg");
  gtk_tool_button_set_icon_widget (self->instrument_add,
                                   GTK_WIDGET (image));
  image = gtk_image_new_from_resource (
          "/online/alextee/zrythm/plus.svg");
  gtk_button_set_image (GTK_BUTTON (self->channels_add), image);

  /* setup digital meters */
  self->digital_bpm = digital_meter_widget_new (DIGITAL_METER_TYPE_BPM);
  self->digital_transport = digital_meter_widget_new (DIGITAL_METER_TYPE_POSITION);
  gtk_container_add (GTK_CONTAINER (self->digital_meters),
                     GTK_WIDGET (self->digital_bpm));
  gtk_container_add (GTK_CONTAINER (self->digital_meters),
                     GTK_WIDGET (self->digital_transport));
  gtk_widget_show_all (GTK_WIDGET (self->digital_meters));

  /* set transport controls */
  transport_controls_init (self);

  /*gtk_widget_add_events (GTK_WIDGET (self->main_box),*/
                         /*GDK_KEY_PRESS_MASK);*/
  g_signal_connect (G_OBJECT (self->main_box), "key_release_event",
                    G_CALLBACK (key_release_cb), self);
  /*g_signal_connect (G_OBJECT (self), "grab-notify",*/
                    /*G_CALLBACK (key_press_cb), NULL);*/

  return self;
}

