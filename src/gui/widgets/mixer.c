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
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/track.h"
#include "utils/gtk.h"

G_DEFINE_TYPE (MixerWidget, mixer_widget, GTK_TYPE_BOX)

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
  self->ddbox = drag_dest_box_widget_new (GTK_ORIENTATION_HORIZONTAL,
                                          0,
                                          DRAG_DEST_BOX_TYPE_MIXER);
  gtk_widget_set_size_request (GTK_WIDGET (self->ddbox),
                               20, -1);
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->ddbox),
                      1, 1, 0);

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

/**
 * Adds channel to mixer widget.
 */
void
mixer_widget_add_channel (MixerWidget * self, Channel * channel)
{
  /* remove dummy box for dnd */
  g_object_ref (self->ddbox);
  gtk_container_remove (GTK_CONTAINER (self->channels_box),
                        GTK_WIDGET (self->ddbox));

  /* add widget */
  gtk_container_remove (GTK_CONTAINER (self->channels_box),
                        GTK_WIDGET (self->channels_add));
  gtk_box_pack_start (self->channels_box,
                    GTK_WIDGET (channel->widget),
                    Z_GTK_NO_EXPAND,
                    Z_GTK_NO_FILL,
                    0);
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->channels_add),
                      Z_GTK_NO_EXPAND,
                      Z_GTK_NO_FILL,
                      0);

  /* update the slots on the channel to show correct names */
  channel_update_slots (channel->widget);

  /* re-add dummy box for dnd */
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->ddbox),
                      1, 1, 0);
  g_object_unref (self->ddbox);

}

static void
mixer_widget_init (MixerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));
}

/**
 * Removes channel from mixer widget.
 */
void
mixer_widget_remove_channel (Channel * channel)
{
  MixerWidget * self = MIXERW;

  /* remove all channels starting from channel */
  for (int i = channel->id; i < MIXER->num_channels; i++)
    {
      Channel * c = MIXER->channels[i];

      gtk_container_remove (GTK_CONTAINER (self->channels_box),
                            GTK_WIDGET (c->widget));
    }

  /* remove dnd box & channels add button */
  g_object_ref (self->ddbox);
  g_object_ref (self->channels_add);
  gtk_container_remove (GTK_CONTAINER (self->channels_box),
                        GTK_WIDGET (self->ddbox));
  gtk_container_remove (GTK_CONTAINER (self->channels_box),
                        GTK_WIDGET (self->channels_add));

  /* re-add all channels after channel */
  for (int i = channel->id; i < MIXER->num_channels; i++)
    {
      Channel * c = MIXER->channels[i];

      gtk_box_pack_start (self->channels_box,
                        GTK_WIDGET (c->widget),
                        0, 0, 0);
    }

  /* re-add dnd box & channels add button */
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->channels_add),
                      0, 0, 0);
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->ddbox),
                      1, 1, 0);
  g_object_unref (self->ddbox);
  g_object_unref (self->channels_add);

  gtk_widget_destroy (GTK_WIDGET (channel->widget));
}

