/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
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
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/tracklist.h"
#include "gui/widgets/track.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MixerWidget,
               mixer_widget,
               GTK_TYPE_BOX)

void
mixer_widget_refresh (MixerWidget * self)
{
  g_object_ref (self->ddbox);
  g_object_ref (self->channels_add);

  /* remove all things in the container */
  z_gtk_container_remove_all_children (
    GTK_CONTAINER (self->channels_box));

  /* add all channels */
  for (int i = -1; i < MIXER->num_channels; i++)
    {
      Channel * channel;
      if (i == -1)
        channel = MIXER->master;
      else
        channel = MIXER->channels[i];

      /* create chan widget if necessary */
      if (!channel->widget)
        channel->widget = channel_widget_new (channel);

      channel_widget_refresh (channel->widget);

      if (i != -1 &&
          !gtk_widget_get_parent (
            GTK_WIDGET (channel->widget))) /* not master */
        {
          gtk_box_pack_start (
            self->channels_box,
            GTK_WIDGET (channel->widget),
            Z_GTK_NO_EXPAND,
            Z_GTK_NO_FILL,
            0);
        }
    }

  /* add the add button */
  gtk_box_pack_start (
    self->channels_box,
    GTK_WIDGET (self->channels_add),
    Z_GTK_NO_EXPAND,
    Z_GTK_NO_FILL,
    0);

  /* re-add dummy box for dnd */
  if (!GTK_IS_WIDGET (self->ddbox))
    self->ddbox =
      drag_dest_box_widget_new (
        GTK_ORIENTATION_HORIZONTAL,
        0,
        DRAG_DEST_BOX_TYPE_MIXER);
  gtk_box_pack_start (
    self->channels_box,
    GTK_WIDGET (self->ddbox),
    1, 1, 0);
  g_object_unref (self->ddbox);
  g_object_unref (self->channels_add);
}

void
mixer_widget_setup (MixerWidget * self,
                    Channel *     master)
{
  if (!master->widget)
    master->widget = channel_widget_new (master);
  gtk_container_add (
    GTK_CONTAINER (self->master_box),
    GTK_WIDGET (master->widget));

  mixer_widget_refresh (self);
}

static void
mixer_widget_class_init (MixerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "mixer.ui");

  gtk_widget_class_bind_template_child (
    klass,
    MixerWidget,
    channels_box);
  gtk_widget_class_bind_template_child (
    klass,
    MixerWidget,
    channels_add);
  gtk_widget_class_bind_template_child (
    klass,
    MixerWidget,
    master_box);
}

static void
mixer_widget_init (MixerWidget * self)
{
  gtk_widget_init_template (GTK_WIDGET (self));

  /* add dummy box for dnd */
  self->ddbox = drag_dest_box_widget_new (
    GTK_ORIENTATION_HORIZONTAL,
    0,
    DRAG_DEST_BOX_TYPE_MIXER);
  gtk_box_pack_start (self->channels_box,
                      GTK_WIDGET (self->ddbox),
                      1, 1, 0);
}
