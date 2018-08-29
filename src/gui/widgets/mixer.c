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
#include "gui/widgets/channel.h"
#include "gui/widgets/mixer.h"

void
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
  channel_add_plugin (MIXER->channels[0],
                                  0,
                                  plugin);
}

void
mixer_setup (GtkBox * mixer,
             GtkBox * channels)
{
  gtk_box_pack_end (mixer,
                    GTK_WIDGET (channel_widget_new (MIXER->master)),
                    0, 0, 0);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      ChannelWidget * chw = channel_widget_new (MIXER->channels[i]);
      gtk_box_pack_start (channels,
                        GTK_WIDGET (chw),
                        0, 0, 0);
      gtk_drag_dest_set (GTK_WIDGET (mixer),
                                GTK_DEST_DEFAULT_ALL,
                                WIDGET_MANAGER->entries,
                                WIDGET_MANAGER->num_entries,
                                GDK_ACTION_COPY);

    }

  g_signal_connect (GTK_WIDGET (mixer),
                    "drag-data-received",
                    G_CALLBACK(on_drag_data_received), NULL);
  gtk_widget_show_all (GTK_WIDGET (mixer));
}
