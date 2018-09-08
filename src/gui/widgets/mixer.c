/*
 * gui/widgets/mixer.c - Manages plugins
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

#include "audio/channel.h"
#include "audio/mixer.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "gui/widget_manager.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"

static int counter = 0;

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
  Plugin_Descriptor * descr = *(gpointer *) gtk_selection_data_get_data (data);

  /* FIXME if lv2... */
  Plugin * plugin = plugin_new ();
  plugin->descr = descr;
  LV2_Plugin * lv2_plugin = lv2_create_from_uri (plugin, descr->uri);

  plugin_instantiate (plugin);

  /* TODO add to specific channel */
  Channel * new_channel = channel_create (CT_MIDI,
                                          g_strdup_printf ("%s %d",
                                                           descr->name,
                                                           counter++));
  channel_add_plugin (new_channel,
                                  0,
                                  plugin);
  MIXER->channels[MIXER->num_channels++] = new_channel;

  /* remove dummy box for dnd */
  gtk_container_remove (GTK_CONTAINER (MAIN_WINDOW->channels),
                        GTK_WIDGET (MAIN_WINDOW->dummy_mixer_box));

  /* add widget */
  gtk_container_remove (GTK_CONTAINER (MAIN_WINDOW->channels),
                        GTK_WIDGET (MAIN_WINDOW->channels_add));
  gtk_box_pack_start (MAIN_WINDOW->channels,
                    GTK_WIDGET (new_channel->widget),
                    0, 0, 0);
  gtk_box_pack_start (MAIN_WINDOW->channels,
                      GTK_WIDGET (MAIN_WINDOW->channels_add),
                      0, 0, 0);

  /* update the slots on the channel to show correct names */
  channel_update_slots (new_channel->widget);

  /* re-add dummy box for dnd */
  gtk_box_pack_start (MAIN_WINDOW->channels,
                      GTK_WIDGET (MAIN_WINDOW->dummy_mixer_box),
                      1, 1, 0);

  gtk_widget_show_all (GTK_WIDGET (new_channel->widget));
  gtk_widget_queue_draw (GTK_WIDGET (new_channel->widget));
}

void
mixer_setup (GtkBox * mixer,
             GtkBox * channels)
{
  gtk_box_pack_end (mixer,
                    GTK_WIDGET (MIXER->master->widget),
                    0, 0, 0);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      ChannelWidget * chw = MIXER->channels[i]->widget;
      gtk_container_remove (GTK_CONTAINER (channels),
                            GTK_WIDGET (WIDGET_MANAGER->main_window->channels_add));
      gtk_box_pack_start (channels,
                        GTK_WIDGET (chw),
                        0, 0, 0);
      gtk_box_pack_start (channels,
                          GTK_WIDGET (WIDGET_MANAGER->main_window->channels_add),
                          0, 0, 0);

    }
  /* add dummy box for dnd */
  MAIN_WINDOW->dummy_mixer_box = GTK_BOX (
                              gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_widget_set_size_request (GTK_WIDGET (MAIN_WINDOW->dummy_mixer_box),
                               20, -1);
  gtk_box_pack_start (channels,
                      GTK_WIDGET (MAIN_WINDOW->dummy_mixer_box),
                      1, 1, 0);
  gtk_drag_dest_set (GTK_WIDGET (MAIN_WINDOW->dummy_mixer_box),
                            GTK_DEST_DEFAULT_ALL,
                            WIDGET_MANAGER->entries,
                            WIDGET_MANAGER->num_entries,
                            GDK_ACTION_COPY);

  g_signal_connect (GTK_WIDGET (MAIN_WINDOW->dummy_mixer_box),
                    "drag-data-received",
                    G_CALLBACK(on_drag_data_received), NULL);
  gtk_widget_show_all (GTK_WIDGET (mixer));
}
