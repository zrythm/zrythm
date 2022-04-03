/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel.h"
#include "audio/track.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_sends_expander.h"
#include "project.h"
#include "settings/settings.h"
#include "utils/gtk.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>
#include <gtk/gtk.h>

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
channel_sends_expander_widget_refresh (
  ChannelSendsExpanderWidget * self)
{
  for (int i = 0; i < STRIP_SIZE; i++)
    {
      gtk_widget_queue_draw (
        GTK_WIDGET (self->slots[i]));
    }
}

static void
on_reveal_changed (
  ExpanderBoxWidget *          expander_box,
  bool                         revealed,
  ChannelSendsExpanderWidget * self)
{
  if (self->position == CSE_POSITION_CHANNEL)
    {
      g_settings_set_boolean (
        S_UI_MIXER, "sends-expanded", revealed);
      EVENTS_PUSH (
        ET_MIXER_CHANNEL_SENDS_EXPANDED_CHANGED,
        self->track->channel);
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
  expander_box_widget_set_label (
    Z_EXPANDER_BOX_WIDGET (self), _ ("Sends"));

  if (track)
    {
      switch (track->out_signal_type)
        {
        case TYPE_AUDIO:
          expander_box_widget_set_icon_name (
            Z_EXPANDER_BOX_WIDGET (self),
            "audio-send");
          break;
        case TYPE_EVENT:
          expander_box_widget_set_icon_name (
            Z_EXPANDER_BOX_WIDGET (self),
            "midi-send");
          break;
        default:
          break;
        }
    }

  if (track != self->track || position != self->position)
    {
      /* remove children */
      z_gtk_widget_destroy_all_children (
        GTK_WIDGET (self->box));

      Channel * ch = track_get_channel (track);
      g_return_if_fail (ch);
      for (int i = 0; i < STRIP_SIZE; i++)
        {
          GtkBox * strip_box = GTK_BOX (gtk_box_new (
            GTK_ORIENTATION_HORIZONTAL, 0));
          gtk_widget_set_name (
            GTK_WIDGET (strip_box),
            "channel-sends-expander-strip-box");
          self->strip_boxes[i] = strip_box;
          ChannelSendWidget * csw =
            channel_send_widget_new (ch->sends[i]);
          self->slots[i] = csw;
          gtk_box_append (
            strip_box, GTK_WIDGET (csw));

          gtk_box_append (
            self->box, GTK_WIDGET (strip_box));
        }
    }

  self->track = track;
  self->position = position;

  switch (position)
    {
    case CSE_POSITION_INSPECTOR:
      gtk_widget_set_size_request (
        GTK_WIDGET (self->scroll), -1, 124);
      break;
    case CSE_POSITION_CHANNEL:
      gtk_widget_set_size_request (
        GTK_WIDGET (self->scroll), -1, 68);
      expander_box_widget_set_reveal_callback (
        Z_EXPANDER_BOX_WIDGET (self),
        (ExpanderBoxRevealFunc) on_reveal_changed,
        self);
      break;
    }

  channel_sends_expander_widget_refresh (self);
}

static void
channel_sends_expander_widget_class_init (
  ChannelSendsExpanderWidgetClass * klass)
{
}

static void
channel_sends_expander_widget_init (
  ChannelSendsExpanderWidget * self)
{
  self->scroll = GTK_SCROLLED_WINDOW (
    gtk_scrolled_window_new ());
  gtk_widget_set_name (
    GTK_WIDGET (self->scroll),
    "channel-sends-expander-scroll");
  gtk_widget_set_vexpand (
    GTK_WIDGET (self->scroll), true);

  self->viewport =
    GTK_VIEWPORT (gtk_viewport_new (NULL, NULL));
  gtk_viewport_set_scroll_to_focus (
    self->viewport, false);
  gtk_widget_set_name (
    GTK_WIDGET (self->viewport),
    "channel-sends-expander-viewport");
  gtk_scrolled_window_set_child (
    GTK_SCROLLED_WINDOW (self->scroll),
    GTK_WIDGET (self->viewport));

  self->box = GTK_BOX (
    gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
  gtk_widget_set_name (
    GTK_WIDGET (self->box),
    "channel-sends-expander-box");
  gtk_viewport_set_child (
    GTK_VIEWPORT (self->viewport),
    GTK_WIDGET (self->box));

  expander_box_widget_add_content (
    Z_EXPANDER_BOX_WIDGET (self),
    GTK_WIDGET (self->scroll));
}
