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

#include "actions/channel_send_action.h"
#include "audio/channel.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/objects.h"

#include <glib/gi18n.h>

void
channel_send_action_init_loaded (
  ChannelSendAction * self)
{
}

/**
 * Creates a new action.
 *
 * @param midi MIDI port, if connecting MIDI.
 * @param stereo Stereo ports, if connecting audio.
 */
UndoableAction *
channel_send_action_new (
  ChannelSend *         send,
  ChannelSendActionType type,
  Port *                midi,
  StereoPorts *         stereo,
  float                 amount)
{
  ChannelSendAction * self =
    object_new (ChannelSendAction);
  UndoableAction * ua = (UndoableAction *) self;
  ua->type = UA_CHANNEL_SEND;

  self->type = type;
  self->send_before = channel_send_clone (send);
  self->send_after = channel_send_clone (send);
  self->send_after->amount = amount;

  if (midi)
    {
      self->send_after->dest_midi_id = midi->id;
    }
  if (stereo)
    {
      self->send_after->dest_l_id = stereo->l->id;
      self->send_after->dest_r_id = stereo->r->id;
    }

  return ua;
}

static void
connect_or_disconnect (
  ChannelSendAction * self,
  bool                connect,
  bool                after)
{
  /* get the actual channel send from the project */
  ChannelSend * send =
    channel_send_find (self->send_before);

  channel_send_disconnect (send);

  if (connect)
    {
      Track * track = channel_send_get_track (send);
      switch (track->out_signal_type)
        {
        case TYPE_EVENT:
          {
            Port * port =
              port_find_from_identifier (
                after ?
                  &self->send_after->dest_midi_id :
                  &self->send_before->dest_midi_id);
            channel_send_connect_midi (send, port);
          }
          break;
        case TYPE_AUDIO:
          {
            Port * l =
              port_find_from_identifier (
                after ?
                  &self->send_after->dest_l_id :
                  &self->send_before->dest_l_id);
            Port * r =
              port_find_from_identifier (
                after ?
                  &self->send_after->dest_r_id :
                  &self->send_before->dest_r_id);
            channel_send_connect_stereo (
              send, NULL, l, r);
          }
          break;
        default:
          break;
        }
    }
}

int
channel_send_action_do (
  ChannelSendAction * self)
{
  /* get the actual channel send from the project */
  ChannelSend * send =
    channel_send_find (self->send_before);

  switch (self->type)
    {
    case CHANNEL_SEND_ACTION_CONNECT_MIDI:
    case CHANNEL_SEND_ACTION_CONNECT_STEREO:
    case CHANNEL_SEND_ACTION_CHANGE_PORTS:
      connect_or_disconnect (self, true, true);
      break;
    case CHANNEL_SEND_ACTION_DISCONNECT:
      connect_or_disconnect (self, false, true);
      break;
    case CHANNEL_SEND_ACTION_CHANGE_AMOUNT:
      channel_send_set_amount (
        send, self->send_after->amount);
      break;
    default:
      break;
    }

  EVENTS_PUSH (ET_CHANNEL_SEND_CHANGED, send);

  return 0;
}

/**
 * Edits the plugin.
 */
int
channel_send_action_undo (
  ChannelSendAction * self)
{
  /* get the actual channel send from the project */
  ChannelSend * send =
    channel_send_find (self->send_before);

  switch (self->type)
    {
    case CHANNEL_SEND_ACTION_CONNECT_MIDI:
    case CHANNEL_SEND_ACTION_CONNECT_STEREO:
      connect_or_disconnect (self, false, true);
      break;
    case CHANNEL_SEND_ACTION_CHANGE_PORTS:
    case CHANNEL_SEND_ACTION_DISCONNECT:
      connect_or_disconnect (self, true, false);
      break;
    case CHANNEL_SEND_ACTION_CHANGE_AMOUNT:
      channel_send_set_amount (
        send, self->send_before->amount);
      break;
    default:
      break;
    }

  EVENTS_PUSH (ET_CHANNEL_SEND_CHANGED, send);

  return 0;
}

char *
channel_send_action_stringize (
  ChannelSendAction * self)
{
  return g_strdup (_("Channel send connection"));
}

void
channel_send_action_free (
  ChannelSendAction * self)
{
  object_free_w_func_and_null (
    channel_send_free, self->send_before);
  object_free_w_func_and_null (
    channel_send_free, self->send_after);

  object_zero_and_free (self);
}
