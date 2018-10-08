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
#include "audio/track.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "gui/widget_manager.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/tracks.h"

G_DEFINE_TYPE (MixerWidget, mixer_widget, GTK_TYPE_BOX)

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
  MixerWidget * self = MAIN_WINDOW->mixer;
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
  gtk_container_remove (GTK_CONTAINER (self->channels_box),
                        GTK_WIDGET (self->dummy_mixer_box));

  /* add widget */
  gtk_container_remove (GTK_CONTAINER (self->channels_box),
                        GTK_WIDGET (self->channels_add));
  gtk_box_pack_start (self->channels_box,
                    GTK_WIDGET (new_channel->widget),
                    0, 0, 0);
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->channels_add),
                      0, 0, 0);

  /* update the slots on the channel to show correct names */
  channel_update_slots (new_channel->widget);

  /* re-add dummy box for dnd */
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->dummy_mixer_box),
                      1, 1, 0);

  /* create track widget */
  tracks_widget_add_channel (MAIN_WINDOW->tracks, new_channel);

  gtk_widget_show_all (GTK_WIDGET (new_channel->widget));
  gtk_widget_show_all (GTK_WIDGET (new_channel->track->widget));
  gtk_widget_queue_draw (GTK_WIDGET (new_channel->widget));
}

MixerWidget *
mixer_widget_new ()
{
  MixerWidget * self = g_object_new (MIXER_WIDGET_TYPE, NULL);

  gtk_box_pack_end (GTK_BOX (self),
                    GTK_WIDGET (MIXER->master->widget),
                    0, 0, 0);
  for (int i = 0; i < MIXER->num_channels; i++)
    {
      ChannelWidget * chw = MIXER->channels[i]->widget;
      gtk_container_remove (GTK_CONTAINER (self->channels_box),
                            GTK_WIDGET (self->channels_add));
      gtk_box_pack_start (self->channels_box,
                        GTK_WIDGET (chw),
                        0, 0, 0);
      gtk_box_pack_start (self->channels_box,
                          GTK_WIDGET (self->channels_add),
                          0, 0, 0);

    }
  /* add dummy box for dnd */
  self->dummy_mixer_box = GTK_BOX (
                              gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  gtk_widget_set_size_request (GTK_WIDGET (self->dummy_mixer_box),
                               20, -1);
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->dummy_mixer_box),
                      1, 1, 0);
  gtk_drag_dest_set (GTK_WIDGET (self->dummy_mixer_box),
                            GTK_DEST_DEFAULT_ALL,
                            WIDGET_MANAGER->entries,
                            WIDGET_MANAGER->num_entries,
                            GDK_ACTION_COPY);

  g_signal_connect (GTK_WIDGET (self->dummy_mixer_box),
                    "drag-data-received",
                    G_CALLBACK(on_drag_data_received), NULL);

  return self;
}


static void
mixer_widget_class_init (MixerWidgetClass * klass)
{
  gtk_widget_class_set_template_from_resource (
                        GTK_WIDGET_CLASS (klass),
                        "/online/alextee/zrythm/ui/mixer.ui");

  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MixerWidget, channels_scroll);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MixerWidget, channels_viewport);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MixerWidget, channels_box);
  gtk_widget_class_bind_template_child (GTK_WIDGET_CLASS (klass),
                                                MixerWidget, channels_add);
}

static void
mixer_widget_init (MixerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}
