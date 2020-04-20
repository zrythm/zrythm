/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "audio/channel_send.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "project.h"

#include <glib/gi18n.h>

static PortType
get_signal_type (
  ChannelSend * self)
{
  Track * track = channel_send_get_track (self);
  g_return_val_if_fail (track, TYPE_AUDIO);
  return track->out_signal_type;
}

/**
 * Inits a channel send.
 */
void
channel_send_init (
  ChannelSend * self,
  int           track_pos,
  int           slot)
{
  self->track_pos = track_pos;
  self->slot = slot;
  self->is_empty = true;
  self->amount = 1.f;
  self->on = true;
}

Track *
channel_send_get_track (
  ChannelSend * self)
{
  Track * track = TRACKLIST->tracks[self->track_pos];
  g_return_val_if_fail (track, NULL);
  return track;
}

/**
 * Connects a send to stereo ports.
 */
void
channel_send_connect_stereo (
  ChannelSend * self,
  StereoPorts * stereo)
{

}

/**
 * Connects a send to a midi port.
 */
void
channel_send_connect_midi (
  ChannelSend * self,
  Port *        port)
{
}

void
channel_send_set_amount (
  ChannelSend * self,
  float         amount)
{
  self->amount = amount;
}

void
channel_send_set_on (
  ChannelSend * self,
  bool          on)
{
  self->on = on;
}

/**
 * Get the name of the destination.
 */
void
channel_send_get_dest_name (
  ChannelSend * self,
  char *        buf)
{
  if (self->is_empty)
    {
      if (channel_send_is_prefader (self))
        strcpy (buf, _("Pre-fader send"));
      else
        strcpy (buf, _("Post-fader send"));
    }
  else
    {
      PortType type = get_signal_type (self);
      if (type == TYPE_AUDIO)
        {
          strcpy (buf, self->dest_l_id.label);
        }
      else
        {
          strcpy (buf, self->dest_midi_id.label);
        }
    }
}
