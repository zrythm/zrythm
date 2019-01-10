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
#include "audio/tracklist.h"
#include "audio/transport.h"
#include "gui/accel.h"
#include "gui/widgets/arranger.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/browser.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/center_dock_bot_box.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/connections.h"
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
#include "gui/widgets/top_bar.h"
#include "gui/widgets/tracklist.h"
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
  g_application_quit (G_APPLICATION (ZRYTHM));
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

  gtk_window_set_title (GTK_WINDOW (self), "Zrythm");

  return self;
}

void
main_window_widget_refresh (MainWindowWidget * self)
{
  header_bar_widget_setup (MW_HEADER_BAR,
                           self,
                           PROJECT->title);

  connections_widget_refresh (MW_CONNECTIONS);
  tracklist_widget_setup (MW_TRACKLIST,
                          TRACKLIST);

  /* setup ruler */
  gtk_scrolled_window_set_hadjustment (
    MW_CENTER_DOCK->ruler_scroll,
    gtk_scrolled_window_get_hadjustment (
      MW_CENTER_DOCK->timeline_scroll));

  /* setup timeline */
  arranger_widget_setup (
    Z_ARRANGER_WIDGET (MW_CENTER_DOCK->timeline),
    ZRYTHM->snap_grid_timeline,
    ARRANGER_TYPE_TIMELINE);
  gtk_scrolled_window_set_vadjustment (
    MW_CENTER_DOCK->timeline_scroll,
    gtk_scrolled_window_get_vadjustment (
      MW_CENTER_DOCK->tracklist_scroll));
  gtk_widget_show_all (
    GTK_WIDGET (MW_CENTER_DOCK->timeline));

  /* setup bot toolbar */
  snap_grid_widget_setup (MW_CENTER_DOCK->bot_box->snap_grid_midi,
                          ZRYTHM->snap_grid_midi);

  /* setup piano roll */
  if (MW_BOT_DOCK_EDGE && PIANO_ROLL)
    {
      piano_roll_widget_setup (PIANO_ROLL,
                               ZRYTHM->piano_roll);
    }

  // set icons
  GtkWidget * image =
    resources_get_icon (ICON_TYPE_ZRYTHM,
                        "z.svg");
  gtk_window_set_icon (
          GTK_WINDOW (self),
          gtk_image_get_pixbuf (GTK_IMAGE (image)));

  /* setup top and bot bars */
  top_bar_widget_refresh (self->top_bar);
  bot_bar_widget_refresh (self->bot_bar);

  /* setup mixer */
  mixer_widget_setup (MW_MIXER,
                      MIXER->master);
}

void
main_window_widget_minimize (MainWindowWidget * self)
{
  gtk_window_iconify (GTK_WINDOW (self));
}

static void
main_window_widget_class_init (MainWindowWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass,
                                "main_window.ui");

  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    main_box);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    header_bar_box);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    header_bar);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    top_bar);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    center_box);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    center_dock);
  gtk_widget_class_bind_template_child (
    klass,
    MainWindowWidget,
    bot_bar);
  gtk_widget_class_bind_template_callback (
    klass,
    on_main_window_destroy);
  gtk_widget_class_bind_template_callback (
    klass,
    on_state_changed);
}

/*static gboolean*/
/*on_export (GtkAccelGroup *accel_group,*/
           /*GObject *acceleratable,*/
           /*guint keyval,*/
           /*GdkModifierType modifier)*/
/*{*/
  /*g_message ("exporting");*/

/*}*/

static void
main_window_widget_init (MainWindowWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  accel_add_all ();
  self->accel_group = gtk_accel_group_new ();
  gtk_window_add_accel_group (GTK_WINDOW (self),
                              self->accel_group);
  /*GClosure * closure =*/
    /*g_cclosure_new (G_CALLBACK (on_export),*/
                    /*NULL,*/
                    /*NULL);*/
  /*gtk_accel_group_connect_by_path (self->accel_group,*/
                                   /*"<MainWindow>/File/Export As",*/
                                   /*closure);*/

}
