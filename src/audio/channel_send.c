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

#include "audio/channel_send.h"
#include "audio/control_port.h"
#include "audio/router.h"
#include "audio/track.h"
#include "audio/tracklist.h"
#include "gui/widgets/center_dock.h"
#include "gui/widgets/channel_send.h"
#include "gui/widgets/channel_sends_expander.h"
#include "gui/widgets/inspector_track.h"
#include "gui/widgets/left_dock_edge.h"
#include "gui/widgets/main_window.h"
#include "project.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

static PortType
get_signal_type (
  ChannelSend * self)
{
  Track * track = channel_send_get_track (self);
  g_return_val_if_fail (track, TYPE_AUDIO);
  return track->out_signal_type;
}

void
channel_send_init_loaded (
  ChannelSend * self,
  bool          is_project)
{
  port_init_loaded (self->enabled, is_project);
  port_init_loaded (self->amount, is_project);
}

/**
 * Creates a channel send instance.
 */
ChannelSend *
channel_send_new (
  int           track_pos,
  int           slot)
{
  ChannelSend * self = object_new (ChannelSend);

  self->schema_version =
    CHANNEL_SEND_SCHEMA_VERSION;
  self->track_pos = track_pos;
  self->slot = slot;

  char name[600];
  sprintf (
    name, _("Channel Send %d enabled"), slot + 1);
  self->enabled =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, name);
  self->enabled->id.flags |= PORT_FLAG_TOGGLE;
  self->enabled->id.flags2 |=
    PORT_FLAG2_CHANNEL_SEND_ENABLED;
  port_set_owner_channel_send (self->enabled, self);
  port_set_control_value (
    self->enabled, 0.f, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  sprintf (
    name, _("Channel Send %d amount"), slot + 1);
  self->amount =
    port_new_with_type (
      TYPE_CONTROL, FLOW_INPUT, name);
  self->amount->id.flags |= PORT_FLAG_AMPLITUDE;
  self->amount->id.flags |= PORT_FLAG_AUTOMATABLE;
  self->amount->id.flags2 |=
    PORT_FLAG2_CHANNEL_SEND_AMOUNT;
  port_set_owner_channel_send (self->amount, self);
  port_set_control_value (
    self->amount, 1.f, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  port_identifier_init (&self->dest_l_id);
  port_identifier_init (&self->dest_r_id);
  port_identifier_init (&self->dest_midi_id);

  return self;
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
 * Returns whether the channel send target is a
 * sidechain port (rather than a target track).
 */
bool
channel_send_is_target_sidechain (
  ChannelSend * self)
{
  return
    channel_send_is_enabled (self) &&
    self->is_sidechain;
}

void
channel_send_copy_values (
  ChannelSend * dest,
  ChannelSend * src)
{
  dest->track_pos = src->track_pos;
  dest->slot = src->slot;
  port_set_control_value (
    dest->enabled, src->enabled->control,
    F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  port_set_control_value (
    dest->amount, src->amount->control,
    F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  dest->is_sidechain = src->is_sidechain;
  dest->dest_l_id = src->dest_l_id;
  dest->dest_r_id = src->dest_r_id;
  dest->dest_midi_id = src->dest_midi_id;
}

/**
 * Gets the target track.
 */
Track *
channel_send_get_target_track (
  ChannelSend * self)
{
  if (channel_send_is_empty (self))
    return NULL;

  Track * track = channel_send_get_track (self);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), NULL);

  Port * port;
  switch (track->out_signal_type)
    {
    case TYPE_AUDIO:
      port =
        port_find_from_identifier (
          &self->dest_l_id);
      return
        port_get_track (port, true);
    case TYPE_EVENT:
      port =
        port_find_from_identifier (
          &self->dest_midi_id);
      return
        port_get_track (port, true);
    default:
      break;
    }

  g_return_val_if_reached (NULL);
}

/**
 * Gets the target sidechain port.
 *
 * Returned StereoPorts instance must be free'd.
 */
StereoPorts *
channel_send_get_target_sidechain (
  ChannelSend * self)
{
  g_return_val_if_fail (self->is_sidechain, NULL);

  Port * l =
    port_find_from_identifier (
      &self->dest_l_id);
  Port * r =
    port_find_from_identifier (
      &self->dest_r_id);

  StereoPorts * sp =
    stereo_ports_new_from_existing (l, r);

  return sp;
}

static void
update_connections (
  ChannelSend * self)
{
  if (channel_send_is_empty (self))
    return;

  Track * track = channel_send_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  Port * self_port, * dest_port;
  int idx;
  switch (track->out_signal_type)
    {
    case TYPE_AUDIO:
      for (int i = 0; i < 2; i++)
        {
          self_port =
            channel_send_is_prefader (self) ?
              (i == 0 ?
                track->channel->prefader->
                  stereo_out->l :
                track->channel->prefader->
                  stereo_out->r) :
              (i == 0 ?
                track->channel->fader->stereo_out->l :
                track->channel->fader->stereo_out->r);
          dest_port =
            port_find_from_identifier (
              i == 0 ?
                &self->dest_l_id :
                &self->dest_r_id);
          idx =
            port_get_dest_index (
              self_port, dest_port);
          port_set_multiplier_by_index (
            self_port, idx, self->amount->control);
          idx =
            port_get_src_index (
              dest_port, self_port);
          port_set_src_multiplier_by_index (
            dest_port, idx, self->amount->control);
        }
      break;
    case TYPE_EVENT:
      /* TODO */
      break;
    default:
      break;
    }
}

/**
 * Gets the amount to be used in widgets (0.0-1.0).
 */
float
channel_send_get_amount_for_widgets (
  ChannelSend * self)
{
  g_return_val_if_fail (
    channel_send_is_enabled (self), 0.f);

  Track * track = channel_send_get_track (self);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), 0.f);

  switch (track->out_signal_type)
    {
    case TYPE_AUDIO:
    case TYPE_EVENT:
      return
        math_get_fader_val_from_amp (
          self->amount->control);
    default:
      break;
    }
  g_return_val_if_reached (0.f);
}

/**
 * Sets the amount from a widget amount (0.0-1.0).
 */
void
channel_send_set_amount_from_widget (
  ChannelSend * self,
  float         val)
{
  g_return_if_fail (channel_send_is_enabled (self));

  Track * track = channel_send_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  switch (track->out_signal_type)
    {
    case TYPE_AUDIO:
    case TYPE_EVENT:
      channel_send_set_amount (
        self, math_get_amp_val_from_fader (val));
      break;
    default:
      break;
    }
  update_connections (self);
}

/**
 * Connects a send to stereo ports.
 *
 * This function takes either \ref stereo or both
 * \ref l and \ref r.
 */
void
channel_send_connect_stereo (
  ChannelSend * self,
  StereoPorts * stereo,
  Port *        l,
  Port *        r,
  bool          sidechain)
{
  channel_send_disconnect (self);

  if (stereo)
    {
      port_identifier_copy (
        &self->dest_l_id, &stereo->l->id);
      port_identifier_copy (
        &self->dest_r_id, &stereo->r->id);
    }
  else
    {
      port_identifier_copy (
        &self->dest_l_id, &l->id);
      port_identifier_copy (
        &self->dest_r_id, &r->id);
    }

  Track * track = channel_send_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  StereoPorts * self_stereo =
    channel_send_is_prefader (self) ?
      track->channel->prefader->stereo_out :
      track->channel->fader->stereo_out;

  /* connect */
  if (stereo)
    {
      stereo_ports_connect (
        self_stereo, stereo, true);
    }
  else
    {
      port_connect (self_stereo->l, l, true);
      port_connect (self_stereo->r, r, true);
    }

  port_set_control_value (
    self->enabled, 1.f, F_NOT_NORMALIZED,
    F_PUBLISH_EVENTS);
  self->is_sidechain = sidechain;

  /* set multipliers */
  update_connections (self);

  router_recalc_graph (ROUTER, F_NOT_SOFT);
}

/**
 * Connects a send to a midi port.
 */
void
channel_send_connect_midi (
  ChannelSend * self,
  Port *        port)
{
  channel_send_disconnect (self);

  port_identifier_copy (
    &self->dest_midi_id, &port->id);

  Track * track = channel_send_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  Port * self_port =
    channel_send_is_prefader (self) ?
      track->channel->prefader->midi_out :
      track->channel->fader->midi_out;
  port_connect (self_port, port, true);

  port_set_control_value (
    self->enabled, 1.f, F_NOT_NORMALIZED,
    F_PUBLISH_EVENTS);

  router_recalc_graph (ROUTER, F_NOT_SOFT);
}

static void
disconnect_midi (
  ChannelSend * self)
{
  Track * track = channel_send_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  Port * self_port =
    channel_send_is_prefader (self) ?
      track->channel->prefader->midi_out :
      track->channel->fader->midi_out;
  Port * dest_port =
    port_find_from_identifier (&self->dest_midi_id);
  port_disconnect (self_port, dest_port);
}

static void
disconnect_audio (
  ChannelSend * self)
{
  Track * track = channel_send_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  StereoPorts * self_stereo =
    channel_send_is_prefader (self) ?
      track->channel->prefader->stereo_out :
      track->channel->fader->stereo_out;
  Port * port = self_stereo->l;
  Port * dest_port =
    port_find_from_identifier (&self->dest_l_id);
  port_disconnect (port, dest_port);
  port = self_stereo->r;
  dest_port =
    port_find_from_identifier (&self->dest_r_id);
  port_disconnect (port, dest_port);
}

/**
 * Removes the connection at the given send.
 */
void
channel_send_disconnect (
  ChannelSend * self)
{
  if (channel_send_is_empty (self))
    return;

  g_message ("disconnecting send %p", self);

  Track * track = channel_send_get_track (self);
  g_return_if_fail (IS_TRACK_AND_NONNULL (track));

  if (self->is_sidechain)
    {
      disconnect_audio (self);
    }
  else
    {
      switch (track->out_signal_type)
        {
        case TYPE_AUDIO:
          disconnect_audio (self);
          break;
        case TYPE_EVENT:
          disconnect_midi (self);
          break;
        default:
          break;
        }
    }

  port_set_control_value (
    self->enabled, 0.f, F_NOT_NORMALIZED,
    F_PUBLISH_EVENTS);
  self->is_sidechain = false;

  router_recalc_graph (ROUTER, F_NOT_SOFT);
}

void
channel_send_set_amount (
  ChannelSend * self,
  float         amount)
{
  port_set_control_value (
    self->amount, amount, F_NOT_NORMALIZED,
    F_PUBLISH_EVENTS);
}

/**
 * Get the name of the destination.
 */
void
channel_send_get_dest_name (
  ChannelSend * self,
  char *        buf)
{
  if (channel_send_is_empty (self))
    {
      if (channel_send_is_prefader (self))
        strcpy (buf, _("Pre-fader send"));
      else
        strcpy (buf, _("Post-fader send"));
    }
  else
    {
      PortType type = get_signal_type (self);
      PortIdentifier * dest;
      if (type == TYPE_AUDIO)
        {
          dest = &self->dest_l_id;
        }
      else
        {
          dest = &self->dest_midi_id;
        }

      Port * port =
        port_find_from_identifier (dest);
      if (self->is_sidechain)
        {
          Plugin * pl =
            port_get_plugin (port, true);
          g_return_if_fail (IS_PLUGIN (pl));
          plugin_get_full_port_group_designation (
            pl, port->id.port_group, buf);
        }
      else
        {
          Track * track;
          switch (dest->owner_type)
            {
            case PORT_OWNER_TYPE_TRACK_PROCESSOR:
              track = port_get_track (port, true);
              g_return_if_fail (
                IS_TRACK_AND_NONNULL (track));
              sprintf (
                buf, _("%s input"),
                track->name);
              break;
            default:
              break;
            }
        }
    }
}

ChannelSend *
channel_send_clone (
  ChannelSend * self)
{
  ChannelSend * clone =
    channel_send_new (self->track_pos, self->slot);

  clone->amount->control = self->amount->control;
  clone->enabled->control = self->enabled->control;
  clone->is_sidechain = self->is_sidechain;
  clone->dest_l_id = self->dest_l_id;
  clone->dest_r_id = self->dest_r_id;
  clone->dest_midi_id = self->dest_midi_id;

  return clone;
}

bool
channel_send_is_enabled (
  ChannelSend * self)
{
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (self->enabled), false);

  return control_port_is_toggled (self->enabled);
}

ChannelSendWidget *
channel_send_find_widget (
  ChannelSend * self)
{
  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      return
        MW_TRACK_INSPECTOR->sends->slots[self->slot];
    }
  return NULL;
}

/**
 * Finds the project send from a given send instance.
 */
ChannelSend *
channel_send_find (
  ChannelSend * self)
{
  g_return_val_if_fail (
    TRACKLIST->num_tracks > self->track_pos, NULL);

  Track * track = channel_send_get_track (self);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), NULL);

  return track->channel->sends[self->slot];
}

void
channel_send_free (
  ChannelSend * self)
{
  object_free_w_func_and_null (
    port_free, self->amount);
  object_free_w_func_and_null (
    port_free, self->enabled);

  object_zero_and_free (self);
}
