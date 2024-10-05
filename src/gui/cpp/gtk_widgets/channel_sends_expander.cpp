// SPDX-FileCopyrightText: © 2020-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/cpp/backend/event.h"
#include "gui/cpp/backend/event_manager.h"
#include "gui/cpp/backend/project.h"
#include "gui/cpp/backend/settings/g_settings_manager.h"
#include "gui/cpp/backend/settings/settings.h"
#include "gui/cpp/backend/zrythm.h"
#include "gui/cpp/gtk_widgets/channel_send.h"
#include "gui/cpp/gtk_widgets/channel_sends_expander.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/zrythm_app.h"

#include <glib/gi18n.h>

#include "common/dsp/channel.h"
#include "common/dsp/channel_track.h"
#include "common/dsp/track.h"
#include "common/utils/gtk.h"
#include "common/utils/rt_thread_id.h"

#define CHANNEL_SENDS_EXPANDER_WIDGET_TYPE \
  (channel_sends_expander_widget_get_type ())
G_DEFINE_TYPE (
  ChannelSendsExpanderWidget,
  channel_sends_expander_widget,
  EXPANDER_BOX_WIDGET_TYPE)

/**
 * Refreshes each field.
 */
void
channel_sends_expander_widget_refresh (ChannelSendsExpanderWidget * self)
{
  for (auto &slot : self->slots)
    {
      gtk_widget_queue_draw (GTK_WIDGET (slot));
    }
}

static void
on_reveal_changed (
  ExpanderBoxWidget *          expander_box,
  bool                         revealed,
  ChannelSendsExpanderWidget * self)
{
  if (self->position == ChannelSendsExpanderPosition::CSE_POSITION_CHANNEL)
    {
      g_settings_set_boolean (S_UI_MIXER, "sends-expanded", revealed);
      auto channel_track = dynamic_cast<ChannelTrack *> (self->track);
      EVENTS_PUSH (
        EventType::ET_MIXER_CHANNEL_SENDS_EXPANDED_CHANGED,
        channel_track->channel_.get ());
    }
}

/**
 * Sets up the ChannelSendsExpanderWidget.
 */
void
channel_sends_expander_widget_setup (
  ChannelSendsExpanderWidget * self,
  ChannelSendsExpanderPosition position,
  Track *                      track)
{
  /* set name and icon */
  expander_box_widget_set_label (Z_EXPANDER_BOX_WIDGET (self), _ ("Sends"));

  if (track)
    {
      switch (track->out_signal_type_)
        {
        case PortType::Audio:
          expander_box_widget_set_icon_name (
            Z_EXPANDER_BOX_WIDGET (self), "audio-send");
          break;
        case PortType::Event:
          expander_box_widget_set_icon_name (
            Z_EXPANDER_BOX_WIDGET (self), "midi-send");
          break;
        default:
          break;
        }
    }

  if (track && (track != self->track || position != self->position))
    {
      /* remove children */
      z_gtk_widget_destroy_all_children (GTK_WIDGET (self->box));

      if (auto channel_track = dynamic_cast<ChannelTrack *> (track))
        {
          auto &ch = channel_track->channel_;
          self->strip_boxes.clear ();
          self->slots.clear ();
          for (size_t i = 0; i < STRIP_SIZE; i++)
            {
              GtkBox * strip_box =
                GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
              gtk_widget_set_name (
                GTK_WIDGET (strip_box), "channel-sends-expander-strip-box");
              self->strip_boxes.push_back (strip_box);
              ChannelSendWidget * csw =
                channel_send_widget_new (ch->sends_.at (i).get ());
              self->slots.push_back (csw);
              gtk_box_append (strip_box, GTK_WIDGET (csw));

              gtk_box_append (self->box, GTK_WIDGET (strip_box));
            }
        }
    }

  self->track = track;
  self->position = position;

  switch (position)
    {
    case ChannelSendsExpanderPosition::CSE_POSITION_INSPECTOR:
      gtk_widget_set_size_request (GTK_WIDGET (self->scroll), -1, 124);
      break;
    case ChannelSendsExpanderPosition::CSE_POSITION_CHANNEL:
      gtk_widget_set_size_request (GTK_WIDGET (self->scroll), -1, -1);
      expander_box_widget_set_reveal_callback (
        Z_EXPANDER_BOX_WIDGET (self), (ExpanderBoxRevealFunc) on_reveal_changed,
        self);
      break;
    }

  channel_sends_expander_widget_refresh (self);
}

static void
channel_sends_expander_widget_finalize (GObject * object)
{
  ChannelSendsExpanderWidget * self = Z_CHANNEL_SENDS_EXPANDER_WIDGET (object);
  std::destroy_at (&self->strip_boxes);
  std::destroy_at (&self->slots);
  G_OBJECT_CLASS (channel_sends_expander_widget_parent_class)->finalize (object);
}

static void
channel_sends_expander_widget_class_init (
  ChannelSendsExpanderWidgetClass * klass)
{
  G_OBJECT_CLASS (klass)->finalize = channel_sends_expander_widget_finalize;
}

static void
channel_sends_expander_widget_init (ChannelSendsExpanderWidget * self)
{
  std::construct_at (&self->strip_boxes);
  std::construct_at (&self->slots);

  self->scroll = GTK_SCROLLED_WINDOW (gtk_scrolled_window_new ());
  gtk_widget_set_name (
    GTK_WIDGET (self->scroll), "channel-sends-expander-scroll");
  gtk_widget_set_vexpand (GTK_WIDGET (self->scroll), true);

  self->viewport = GTK_VIEWPORT (gtk_viewport_new (nullptr, nullptr));
  gtk_viewport_set_scroll_to_focus (self->viewport, false);
  gtk_widget_set_name (
    GTK_WIDGET (self->viewport), "channel-sends-expander-viewport");
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (self->scroll), GTK_WIDGET (self->viewport));

  self->box = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_name (GTK_WIDGET (self->box), "channel-sends-expander-box");
  gtk_viewport_set_child (GTK_VIEWPORT (self->viewport), GTK_WIDGET (self->box));

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self), GTK_WIDGET (self->scroll));
}
