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
#include "project.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "audio/transport.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bpm.h"
#include "gui/widgets/browser.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/connections.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/inspector_region.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/piano_roll_page.h"
#include "gui/widgets/rack.h"
#include "gui/widgets/ruler.h"
#include "gui/widgets/snap_grid.h"
#include "gui/widgets/timeline_arranger.h"
#include "gui/widgets/timeline_bg.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/transport_controls.h"
#include "utils/gtk.h"
#include "utils/io.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MainWindowWidget,
               main_window_widget,
               GTK_TYPE_APPLICATION_WINDOW)

void
main_window_widget_quit (MainWindowWidget * self)
{
  g_application_quit (G_APPLICATION (zrythm_app));
}

void
main_window_widget_toggle_maximize (MainWindowWidget * self)
{
  if (self->is_maximized)
    {
      gtk_window_unmaximize (GTK_WINDOW (self));
      self->is_maximized = 0;
    }
  else
    {
      gtk_window_maximize (GTK_WINDOW (self));
    }
}

static void
on_main_window_destroy (MainWindowWidget * self,
                        gpointer user_data)
{
  main_window_widget_quit (self);
}

static void
on_state_changed (MainWindowWidget * self,
                  GdkEventWindowState  * event,
                  gpointer    user_data)
{
  if (event->new_window_state &
        GDK_WINDOW_STATE_MAXIMIZED)
    self->is_maximized = 1;
}

/**
 * FIXME move to timeline
 */
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



void
main_window_widget_open (MainWindowWidget *win,
                         GFile            *file)
{
}

MainWindowWidget *
main_window_widget_new (ZrythmApp * _app)
{
  GtkWidget * img;

  MainWindowWidget * self = g_object_new (
    MAIN_WINDOW_WIDGET_TYPE,
    "application",
    G_APPLICATION (_app),
    NULL);
  WIDGET_MANAGER->main_window = self;
  project_set_title ("Untitled Project");
  gtk_window_set_title (GTK_WINDOW (self), "Zrythm");


  // set default css provider
  GtkCssProvider * css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/online/alextee/zrythm/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);

  /* setup top toolbar */
  self->snap_grid_timeline = snap_grid_widget_new (&PROJECT->snap_grid_timeline);
  gtk_box_pack_start (GTK_BOX (self->snap_grid_timeline_box),
                      GTK_WIDGET (self->snap_grid_timeline),
                      1, 1, 0);

  /* setup tracklist */
  self->tracklist = tracklist_widget_new (PROJECT->tracklist);
  gtk_container_add (GTK_CONTAINER (self->tracklist_viewport),
                     GTK_WIDGET (self->tracklist));
  gtk_widget_set_size_request (GTK_WIDGET (self->tracklist_header),
                               -1,
                               60);
  tracklist_widget_show (self->tracklist);

  /* setup ruler */
  self->ruler = ruler_widget_new ();
  gtk_container_add (GTK_CONTAINER (self->ruler_viewport),
                     GTK_WIDGET (self->ruler));
  gtk_scrolled_window_set_hadjustment (self->ruler_scroll,
                                       gtk_scrolled_window_get_hadjustment (
                                           GTK_SCROLLED_WINDOW (
                                               self->timeline_scroll)));
  gtk_widget_show_all (GTK_WIDGET (self->ruler_viewport));

  /* setup inspector */
  self->inspector = inspector_widget_new ();
  img = resources_get_icon ("gnome-builder/ui-section-symbolic-light.svg");
  gtk_notebook_prepend_page (self->inspector_notebook,
                             GTK_WIDGET (self->inspector),
                             img);

  /* setup timeline */
  self->timeline = timeline_arranger_widget_new (
    &PROJECT->snap_grid_timeline);
  gtk_container_add (GTK_CONTAINER (self->timeline_viewport),
                     GTK_WIDGET (self->timeline));
  gtk_scrolled_window_set_min_content_width (self->timeline_scroll, 400);
  gtk_scrolled_window_set_vadjustment (self->timeline_scroll,
            gtk_scrolled_window_get_vadjustment (self->tracklist_scroll));
  gtk_widget_show_all (GTK_WIDGET (self->timeline_viewport));
  gtk_widget_show_all (GTK_WIDGET (self->timeline));
  /*gtk_widget_show_all (GTK_WIDGET (self->timeline->bg));*/

  /* setup browser */
  self->browser = browser_widget_new ();
  gtk_notebook_prepend_page (
    self->right_notebook,
    GTK_WIDGET (self->browser),
    resources_get_icon ("plugins.svg"));
  gtk_widget_show_all (GTK_WIDGET (self->right_notebook));

  /* setup bot toolbar */
  self->snap_grid_midi = snap_grid_widget_new (&PROJECT->snap_grid_midi);
  gtk_box_pack_start (GTK_BOX (self->snap_grid_midi_box),
                      GTK_WIDGET (self->snap_grid_midi),
                      1, 1, 0);

  /* setup bot half region */
  self->mixer = mixer_widget_new ();
  self->piano_roll_page = piano_roll_page_widget_new ();
  self->connections = connections_widget_new ();
  self->rack = rack_widget_new ();
  GtkWidget * box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box),
                      resources_get_icon ("piano_roll.svg"),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      2);
  gtk_box_pack_end (GTK_BOX (box),
                    gtk_label_new ("Piano Roll"),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    2);
  gtk_widget_show_all (box);
  gtk_notebook_prepend_page (
    self->bot_notebook,
    GTK_WIDGET (self->piano_roll_page),
    box);
  gtk_notebook_append_page (self->bot_notebook,
                            GTK_WIDGET (self->rack),
                            gtk_label_new ("Rack"));
  gtk_notebook_append_page (self->bot_notebook,
                            GTK_WIDGET (self->connections),
                            gtk_label_new ("Connections"));
  box = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_pack_start (GTK_BOX (box),
                      resources_get_icon ("mixer.svg"),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      2);
  gtk_box_pack_end (GTK_BOX (box),
                    gtk_label_new ("Mixer"),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    2);
  gtk_widget_show_all (box);
  gtk_notebook_append_page (self->bot_notebook,
                            GTK_WIDGET (self->mixer),
                            box);
  gtk_widget_show_all (GTK_WIDGET (MAIN_WINDOW->bot_notebook));

  // set icons
  GtkWidget * image = resources_get_icon (
          "z.svg");
  gtk_window_set_icon (
          GTK_WINDOW (self),
          gtk_image_get_pixbuf (GTK_IMAGE (image)));
  gtk_tool_button_set_icon_widget (
    self->instrument_add,
    resources_get_icon ("plus.svg"));
  gtk_button_set_image (
    GTK_BUTTON (self->mixer->channels_add),
    resources_get_icon ("plus.svg"));

  /* setup digital meters */
  self->digital_bpm = digital_meter_widget_new (DIGITAL_METER_TYPE_BPM, NULL);
  self->digital_transport = digital_meter_widget_new (DIGITAL_METER_TYPE_POSITION,
                                                      NULL);
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

  /* show regions */
  /*for (int i = 0; i < MIXER->num_channels; i++)*/
    /*{*/
      /*Channel * channel = MIXER->channels[i];*/
      /*for (int j = 0; j < channel->track->num_regions; j++)*/
        /*{*/
          /*gtk_overlay_add_overlay (GTK_OVERLAY (self->timeline),*/
                                   /*GTK_WIDGET (channel->track->regions[j]->widget));*/
        /*}*/
    /*}*/
  /*for (int j = 0; j < MIXER->master->track->num_regions; j++)*/
    /*{*/
      /*gtk_overlay_add_overlay (GTK_OVERLAY (self->timeline),*/
                               /*GTK_WIDGET (MIXER->master->track->regions[j]->widget));*/
    /*}*/
  gtk_widget_show_all (GTK_WIDGET (self->timeline));

  return self;
}

static void
main_window_widget_class_init (MainWindowWidgetClass * klass)
{
  resources_set_class_template (GTK_WIDGET_CLASS (klass),
                                "main_window.ui");

  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    MainWindowWidget,
    main_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    MainWindowWidget,
    header_bar_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    MainWindowWidget,
    header_bar);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    MainWindowWidget,
    top_toolbar);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    MainWindowWidget,
    snap_grid_timeline_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget,
                                        center_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget,
                                        inspector_notebook);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, editor_plus_browser);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, editor);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, editor_top);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, tracklist_timeline);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, tracklist_top);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, tracklist_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, tracklist_viewport);
  /*gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),*/
                                                /*MainWindowWidget, tracks_paned);*/
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, tracklist_header);
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
                                        MainWindowWidget,
                                        snap_grid_midi_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, instrument_add);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, bot_notebook);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                        MainWindowWidget, right_notebook);
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
}

static void
main_window_widget_init (MainWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
