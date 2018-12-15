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
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/bpm.h"
#include "gui/widgets/browser.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/connections.h"
#include "gui/widgets/digital_meter.h"
#include "gui/widgets/header_bar.h"
#include "gui/widgets/inspector.h"
#include "gui/widgets/inspector_region.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/piano_roll.h"
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



void
main_window_widget_open (MainWindowWidget *win,
                         GFile            *file)
{
}

MainWindowWidget *
main_window_widget_new (ZrythmApp * _app)
{
  MainWindowWidget * self = g_object_new (
    MAIN_WINDOW_WIDGET_TYPE,
    "application",
    G_APPLICATION (_app),
    NULL);

  g_message ("main window initialized");

  WIDGET_MANAGER->main_window = self;
  project_set_title ("Untitled Project");
  gtk_window_set_title (GTK_WINDOW (self), "Zrythm");

  // set default css provider
  GtkCssProvider * css_provider = gtk_css_provider_new();
  gtk_css_provider_load_from_resource (css_provider,
                                       "/org/zrythm/theme.css");
  gtk_style_context_add_provider_for_screen (
          gdk_screen_get_default (),
          GTK_STYLE_PROVIDER (css_provider),
          800);
  g_object_unref (css_provider);

  /* setup top toolbar */
  snap_grid_widget_setup (
    self->snap_grid_timeline,
    &PROJECT->snap_grid_timeline);

  /* setup ruler */
  gtk_scrolled_window_set_hadjustment (
    MW_CENTER_DOCK->ruler_scroll,
    gtk_scrolled_window_get_hadjustment (
      MW_CENTER_DOCK->timeline_scroll));

  /* setup timeline */
  arranger_widget_setup (
    ARRANGER_WIDGET (MW_CENTER_DOCK->timeline),
    &PROJECT->snap_grid_timeline,
    ARRANGER_TYPE_TIMELINE);
  gtk_scrolled_window_set_vadjustment (
    MW_CENTER_DOCK->timeline_scroll,
    gtk_scrolled_window_get_vadjustment (
      MW_CENTER_DOCK->tracklist_scroll));
  gtk_widget_show_all (
    GTK_WIDGET (MW_CENTER_DOCK->timeline));

  /* setup bot toolbar */
  snap_grid_widget_setup (MW_CENTER_DOCK->snap_grid_midi,
                          &PROJECT->snap_grid_midi);

  /* setup piano roll */
  if (MW_BOT_DOCK_EDGE && PIANO_ROLL)
    {
      piano_roll_widget_setup (PIANO_ROLL);
    }

  // set icons
  GtkWidget * image = resources_get_icon (
          "z.svg");
  gtk_window_set_icon (
          GTK_WINDOW (self),
          gtk_image_get_pixbuf (GTK_IMAGE (image)));

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

  /* setup piano roll */

  /*gtk_widget_add_events (GTK_WIDGET (self->main_box),*/
                         /*GDK_KEY_PRESS_MASK);*/
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
  /*gtk_widget_show_all (GTK_WIDGET (self->timeline));*/

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
    snap_grid_timeline);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    MainWindowWidget,
    center_box);
  gtk_widget_class_bind_template_child (
    GTK_WIDGET_CLASS (klass),
    MainWindowWidget,
    center_dock);
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
