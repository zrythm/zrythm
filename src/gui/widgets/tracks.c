/*
 * gui/widgets/tracks.c - Tracks
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

/** \file
 */

#include "audio/channel.h"
#include "audio/mixer.h"
#include "audio/track.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/tracks.h"

G_DEFINE_TYPE (TracksWidget, tracks_widget, GTK_TYPE_PANED)

static void
move_handle (GtkPaned * widget,
              gpointer     user_data)
{
  g_message ("check resize");
}

/**
 * Creates a new Fader widget and binds it to the given value.
 */
void
tracks_widget_setup ()
{
  TracksWidget * self = g_object_new (
                            TRACKS_WIDGET_TYPE,
                            "orientation",
                            GTK_ORIENTATION_VERTICAL,
                            NULL);
  g_signal_connect (GTK_WIDGET (self), "toggle-handle-focus",
                    G_CALLBACK (move_handle), NULL);
  MAIN_WINDOW->tracks = self;

  gtk_widget_set_size_request (GTK_WIDGET (self),
                               -1,
                               6000);
  gtk_paned_set_wide_handle (GTK_PANED (self),
                             USE_WIDE_HANDLE);
  gtk_widget_set_valign (GTK_WIDGET (self),
                         GTK_ALIGN_FILL);
  gtk_paned_pack1 (GTK_PANED (self),
                   GTK_WIDGET (MIXER->master->track->widget),
                   FALSE,
                   FALSE);
  GtkWidget * last_box =
    gtk_box_new (GTK_ORIENTATION_VERTICAL,
                 0);
  gtk_paned_pack2 (GTK_PANED (self),
                   last_box,
                   TRUE,
                   TRUE);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      Channel * channel = MIXER->channels[i];
      tracks_widget_add_channel (self, channel);
    }

  gtk_widget_show_all (GTK_WIDGET (self));
}


void
tracks_widget_add_channel (TracksWidget * self, Channel * channel)
{
  int index = channel_get_index (channel);
  GtkPaned * container = index == 0 ? GTK_PANED (self) :
                  MAIN_WINDOW->tracks->tracks[index - 1];

  /* create new paned */
  GtkWidget * new_paned =
    gtk_paned_new (GTK_ORIENTATION_VERTICAL);
  MAIN_WINDOW->tracks->tracks[index] = GTK_PANED (new_paned);
  gtk_paned_set_wide_handle (GTK_PANED (new_paned),
                             USE_WIDE_HANDLE);
  gtk_widget_set_valign (new_paned,
                         GTK_ALIGN_FILL);

  /* add track widget to new paned */
  gtk_paned_pack1 (GTK_PANED (new_paned),
                   GTK_WIDGET (channel->track->widget),
                   FALSE,
                   FALSE);

  /* add new last box to new gpaned */
  GtkWidget * last_box = gtk_box_new (
                            GTK_ORIENTATION_VERTICAL,
                            0);
  gtk_container_remove (GTK_CONTAINER (container),
                        gtk_paned_get_child2 (
                                GTK_PANED (container)));
  gtk_paned_pack2 (GTK_PANED (new_paned),
                   last_box,
                   TRUE,
                   TRUE);

  /* put new paned where the box was in the parent */
  gtk_paned_pack2 (GTK_PANED (container),
                   new_paned,
                   TRUE,
                   FALSE);

  g_signal_connect (GTK_WIDGET (new_paned), "toggle-handle-focus",
                    G_CALLBACK (move_handle), NULL);

  gtk_widget_show_all (GTK_WIDGET (self));
}


static void
tracks_widget_init (TracksWidget * self)
{
}

static void
tracks_widget_class_init (TracksWidgetClass * klass)
{
}
