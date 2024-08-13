// SPDX-FileCopyrightText: © 2018-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tracklist.h"
#include "gui/widgets/add_track_menu_button.h"
#include "gui/widgets/bot_dock_edge.h"
#include "gui/widgets/channel.h"
#include "gui/widgets/drag_dest_box.h"
#include "gui/widgets/folder_channel.h"
#include "gui/widgets/mixer.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/gtk.h"
#include "utils/resources.h"

#include "gtk_wrapper.h"

G_DEFINE_TYPE (MixerWidget, mixer_widget, GTK_TYPE_BOX)

void
mixer_widget_soft_refresh (MixerWidget * self)
{
  for (auto track : TRACKLIST->tracks_ | type_is<ChannelTrack> ())
    {
      auto ch = track->channel_;
      if (GTK_IS_WIDGET (ch->widget_))
        channel_widget_refresh (ch->widget_);
    }
}

void
mixer_widget_hard_refresh (MixerWidget * self)
{
  std::vector<GtkWidget *> refed_widgets;
  auto                     ref_and_add_to_array = [&refed_widgets] (auto x) {
    g_object_ref (x);
    refed_widgets.push_back (GTK_WIDGET (x));
  };

  ref_and_add_to_array (self->ddbox);
  ref_and_add_to_array (self->channels_add);

  /* ref the channel widgets to make this faster */
  for (auto &track : TRACKLIST->tracks_)
    {
      if (track->is_folder ())
        {
          auto foldable_tr = dynamic_cast<FolderTrack *> (track.get ());
          if (foldable_tr->folder_ch_widget_)
            {
              ref_and_add_to_array (foldable_tr->folder_ch_widget_);
            }
        }

      if (track->has_channel ())
        {
          auto ch_track = dynamic_cast<ChannelTrack *> (track.get ());
          if (ch_track->channel_->widget_)
            {
              ref_and_add_to_array (ch_track->channel_->widget_);
            }
        }
    }

  /* remove all things in the container */
  z_gtk_widget_remove_all_children (GTK_WIDGET (self->channels_box));
  z_return_if_fail (gtk_widget_get_parent (GTK_WIDGET (self->ddbox)) == nullptr);

  /* add all channels */
  for (auto &track : TRACKLIST->tracks_)
    {
      if (!track->should_be_visible ())
        continue;

      if (track->is_foldable () && !track->is_master ())
        {
          auto foldable_tr = dynamic_cast<FolderTrack *> (track.get ());
          if (!foldable_tr->folder_ch_widget_)
            {
              foldable_tr->folder_ch_widget_ =
                folder_channel_widget_new (foldable_tr);
            }

          folder_channel_widget_refresh (foldable_tr->folder_ch_widget_);

          gtk_box_append (
            self->channels_box, GTK_WIDGET (foldable_tr->folder_ch_widget_));
        }

      if (!track->has_channel ())
        continue;

      auto ch_track = dynamic_cast<ChannelTrack *> (track.get ());
      auto ch = ch_track->channel_;

      /* create chan widget if necessary */
      if (!ch->widget_)
        {
          ch->widget_ = channel_widget_new (ch);
        }

      channel_widget_refresh (ch->widget_);

      if (
        !track->is_master ()
        && !gtk_widget_get_parent (GTK_WIDGET (ch->widget_))) /* not master */
        {
          gtk_box_append (self->channels_box, GTK_WIDGET (ch->widget_));
        }
    }

  /* add the add button */
  gtk_box_append (self->channels_box, GTK_WIDGET (self->channels_add));

  /* re-add dummy box for dnd */
  gtk_box_append (self->channels_box, GTK_WIDGET (self->ddbox));

  /* unref refed widgets */
  for (auto ref_widget : refed_widgets)
    {
      g_object_unref (ref_widget);
    }
}

void
mixer_widget_setup (MixerWidget * self, Channel * master)
{
  z_info ("Setting up...");

  if (!master->widget_)
    {
      master->widget_ = channel_widget_new (master->shared_from_this ());
    }

  if (!self->setup)
    {
      gtk_box_append (GTK_BOX (self->master_box), GTK_WIDGET (master->widget_));
    }
  gtk_widget_set_hexpand (GTK_WIDGET (self->master_box), false);

  mixer_widget_hard_refresh (self);

  self->setup = true;

  z_info ("done");
}

MixerWidget *
mixer_widget_new ()
{
  auto * self =
    static_cast<MixerWidget *> (g_object_new (MIXER_WIDGET_TYPE, nullptr));

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
    GTK_ORIENTATION_HORIZONTAL, 0, DragDestBoxType::DRAG_DEST_BOX_TYPE_MIXER);
  gtk_box_append (self->channels_box, GTK_WIDGET (self->ddbox));
}
