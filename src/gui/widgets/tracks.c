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
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/tracks.h"

/* FIXME find an algorithm to name new channels */
static int counter = 0;

G_DEFINE_TYPE (TracksWidget, tracks_widget, GTK_TYPE_PANED)

static void
on_drag_data_received (GtkWidget        *widget,
               GdkDragContext   *context,
               gint              x,
               gint              y,
               GtkSelectionData *data,
               guint             info,
               guint             time,
               gpointer          user_data)
{
  MixerWidget * mixer = MAIN_WINDOW->mixer;
  Plugin_Descriptor * descr = *(gpointer *) gtk_selection_data_get_data (data);
  Plugin * plugin = plugin_create_from_descr (descr);

  if (plugin_instantiate (plugin) < 0)
    {
      GtkDialogFlags flags = GTK_DIALOG_DESTROY_WITH_PARENT;
      GtkWidget * dialog = gtk_message_dialog_new (GTK_WINDOW (MAIN_WINDOW),
                                       flags,
                                       GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE,
                                       "Error instantiating plugin “%s”. Please see log for details.",
                                       plugin->descr->name);
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
      plugin_free (plugin);
      return;
    }

  Channel * new_channel = channel_create (CT_MIDI,
                                          g_strdup_printf ("%s %d",
                                                           descr->name,
                                                           counter++));
  channel_add_plugin (new_channel,
                                  0,
                                  plugin);
  MIXER->channels[MIXER->num_channels++] = new_channel;

  /* remove dummy box for dnd */
  g_object_ref (mixer->dummy_mixer_box);
  gtk_container_remove (GTK_CONTAINER (mixer->channels_box),
                        GTK_WIDGET (mixer->dummy_mixer_box));

  /* add widget */
  gtk_container_remove (GTK_CONTAINER (mixer->channels_box),
                        GTK_WIDGET (mixer->channels_add));
  gtk_box_pack_start (mixer->channels_box,
                    GTK_WIDGET (new_channel->widget),
                    0, 0, 0);
  gtk_box_pack_start (mixer->channels_box,
                      GTK_WIDGET (mixer->channels_add),
                      0, 0, 0);

  /* update the slots on the channel to show correct names */
  channel_update_slots (new_channel->widget);

  /* re-add dummy box for dnd */
  gtk_box_pack_start (mixer->channels_box,
                      GTK_WIDGET (mixer->dummy_mixer_box),
                      1, 1, 0);
  g_object_unref (mixer->dummy_mixer_box);

  /* create track widget */
  tracks_widget_add_channel (MAIN_WINDOW->tracks, new_channel);

  gtk_widget_show_all (GTK_WIDGET (new_channel->widget));
  gtk_widget_show_all (GTK_WIDGET (new_channel->track->widget));
  gtk_widget_queue_draw (GTK_WIDGET (new_channel->widget));
}

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
                   0,
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
  gtk_drag_dest_set (GTK_WIDGET (last_box),
                    GTK_DEST_DEFAULT_ALL,
                    WIDGET_MANAGER->entries,
                    WIDGET_MANAGER->num_entries,
                    GDK_ACTION_COPY);
  g_signal_connect (GTK_WIDGET (last_box),
                    "drag-data-received",
                    G_CALLBACK(on_drag_data_received), NULL);

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
