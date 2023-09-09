/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "dsp/channel.h"
#include "dsp/track.h"
#include "gui/widgets/add_track_menu_button.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/folder_channel.h"
#include "gui/widgets/main_window.h"
#include "gui/widgets/mixer.h"
#include "gui/widgets/track.h"
#include "gui/widgets/tracklist.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/gtk.h"
#include "utils/resources.h"
#include "zrythm_app.h"

#include <gtk/gtk.h>

G_DEFINE_TYPE (MixerWidget, mixer_widget, GTK_TYPE_BOX)

void
mixer_widget_soft_refresh (MixerWidget * self)
{
  Track *   track;
  Channel * ch;
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      track = TRACKLIST->tracks[i];
      if (!track_type_has_channel (track->type))
        continue;

      ch = track->channel;
      g_return_if_fail (ch);

      if (GTK_IS_WIDGET (ch->widget))
        channel_widget_refresh (ch->widget);
    }
}

void
mixer_widget_hard_refresh (MixerWidget * self)
{
#define REF_AND_ADD_TO_ARRAY(x) \
  g_object_ref (x); \
  g_ptr_array_add (refed_widgets, x)

  GPtrArray * refed_widgets = g_ptr_array_new ();

  REF_AND_ADD_TO_ARRAY (self->ddbox);
  REF_AND_ADD_TO_ARRAY (self->channels_add);

  /* ref the channel widgets to make this faster */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (track->folder_ch_widget)
        {
          REF_AND_ADD_TO_ARRAY (track->folder_ch_widget);
        }

      if (track_type_has_channel (track->type))
        {
          if (track->channel->widget)
            {
              REF_AND_ADD_TO_ARRAY (track->channel->widget);
            }
        }
    }

  /* remove all things in the container */
  z_gtk_widget_remove_all_children (GTK_WIDGET (self->channels_box));
  g_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (self->ddbox)) == NULL);

  /* add all channels */
  for (int i = 0; i < TRACKLIST->num_tracks; i++)
    {
      Track * track = TRACKLIST->tracks[i];

      if (!track_get_should_be_visible (track))
        continue;

      if (
        track_type_is_foldable (track->type) && track->type != TRACK_TYPE_MASTER)
        {
          if (!track->folder_ch_widget)
            {
              track->folder_ch_widget = folder_channel_widget_new (track);
            }

          folder_channel_widget_refresh (track->folder_ch_widget);

          gtk_box_append (
            self->channels_box, GTK_WIDGET (track->folder_ch_widget));
        }

      if (!track_type_has_channel (track->type))
        continue;

      Channel * ch = track->channel;
      g_return_if_fail (ch);

      /* create chan widget if necessary */
      if (!ch->widget)
        ch->widget = channel_widget_new (ch);

      channel_widget_refresh (ch->widget);

      if (
        track->type != TRACK_TYPE_MASTER
        && !gtk_widget_get_parent (GTK_WIDGET (ch->widget))) /* not master */
        {
          gtk_box_append (self->channels_box, GTK_WIDGET (ch->widget));
        }
    }

  /* add the add button */
  gtk_box_append (self->channels_box, GTK_WIDGET (self->channels_add));

  /* re-add dummy box for dnd */
  gtk_box_append (self->channels_box, GTK_WIDGET (self->ddbox));

  /* unref refed widgets */
  for (size_t i = 0; i < refed_widgets->len; i++)
    {
      g_object_unref (g_ptr_array_index (refed_widgets, i));
    }
  g_ptr_array_unref (refed_widgets);

#undef REF_AND_ADD_TO_ARRAY
}

void
mixer_widget_setup (MixerWidget * self, Channel * master)
{
  g_message ("Setting up...");

  if (!master->widget)
    {
      master->widget = channel_widget_new (master);
    }

  if (!self->setup)
    {
      gtk_box_append (GTK_BOX (self->master_box), GTK_WIDGET (master->widget));
    }
  gtk_widget_set_hexpand (GTK_WIDGET (self->master_box), false);

  mixer_widget_hard_refresh (self);

  self->setup = true;

  g_message ("done");
}

MixerWidget *
mixer_widget_new (void)
{
  MixerWidget * self = g_object_new (MIXER_WIDGET_TYPE, NULL);

  return self;
}

static void
mixer_widget_class_init (MixerWidgetClass * _klass)
{
  GtkWidgetClass * klass = GTK_WIDGET_CLASS (_klass);
  resources_set_class_template (klass, "mixer.ui");

#define BIND_CHILD(x) \
  gtk_widget_class_bind_template_child (klass, MixerWidget, x)

  BIND_CHILD (channels_box);
  BIND_CHILD (channels_add);
  BIND_CHILD (master_box);

#undef BIND_CHILD
}

static void
mixer_widget_init (MixerWidget * self)
{
  g_type_ensure (DRAG_DEST_BOX_WIDGET_TYPE);
  g_type_ensure (ADD_TRACK_MENU_BUTTON_WIDGET_TYPE);

  gtk_widget_init_template (GTK_WIDGET (self));

  gtk_widget_set_name (GTK_WIDGET (self->channels_add), "mixer-add-channel");

  /* add dummy box for dnd */
  self->ddbox = drag_dest_box_widget_new (
    GTK_ORIENTATION_HORIZONTAL, 0, DRAG_DEST_BOX_TYPE_MIXER);
  gtk_box_append (self->channels_box, GTK_WIDGET (self->ddbox));
}
