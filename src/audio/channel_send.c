// SPDX-License-Identifier: AGPL-3.0-or-later
/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
 */

#include "audio/channel_send.h"
#include "audio/control_port.h"
#include "audio/graph.h"
#include "audio/midi_event.h"
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
#include "utils/dsp.h"
#include "utils/flags.h"
#include "utils/math.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

typedef enum
{
  Z_AUDIO_CHANNEL_SEND_ERROR_FAILED,
} ZAudioChannelSendError;

#define Z_AUDIO_CHANNEL_SEND_ERROR \
  z_audio_channel_send_error_quark ()
GQuark
z_audio_channel_send_error_quark (void);
G_DEFINE_QUARK (
  z-audio-channel-send-error-quark, z_audio_channel_send_error)

static PortType
get_signal_type (const ChannelSend * self)
{
  Track * track = channel_send_get_track (self);
  g_return_val_if_fail (track, TYPE_AUDIO);
  return track->out_signal_type;
}

void
channel_send_init_loaded (
  ChannelSend * self,
  Track *       track)
{
  self->track = track;

#define INIT_LOADED_PORT(x) \
  port_init_loaded (self->x, self)

  INIT_LOADED_PORT (enabled);
  INIT_LOADED_PORT (amount);
  INIT_LOADED_PORT (midi_in);
  INIT_LOADED_PORT (midi_out);
  stereo_ports_init_loaded (self->stereo_in, self);
  stereo_ports_init_loaded (self->stereo_out, self);

#undef INIT_LOADED_PORT
}

/**
 * Creates a channel send instance.
 */
ChannelSend *
channel_send_new (
  unsigned int track_name_hash,
  int          slot,
  Track *      track)
{
  ChannelSend * self = object_new (ChannelSend);
  self->schema_version = CHANNEL_SEND_SCHEMA_VERSION;
  self->track = track;
  self->track_name_hash = track_name_hash;
  self->slot = slot;

#define SET_PORT_OWNER(x) \
  port_set_owner ( \
    self->x, PORT_OWNER_TYPE_CHANNEL_SEND, self)

  char name[600];
  char sym[600];
  sprintf (
    name, _ ("Channel Send %d enabled"), slot + 1);
  self->enabled = port_new_with_type (
    TYPE_CONTROL, FLOW_INPUT, name);
  self->enabled->id.sym = g_strdup_printf (
    "channel_send_%d_enabled", slot + 1);
  self->enabled->id.flags |= PORT_FLAG_TOGGLE;
  self->enabled->id.flags2 |=
    PORT_FLAG2_CHANNEL_SEND_ENABLED;
  SET_PORT_OWNER (enabled);
  port_set_control_value (
    self->enabled, 0.f, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  sprintf (
    name, _ ("Channel Send %d amount"), slot + 1);
  self->amount = port_new_with_type (
    TYPE_CONTROL, FLOW_INPUT, name);
  self->amount->id.sym = g_strdup_printf (
    "channel_send_%d_amount", slot + 1);
  self->amount->id.flags |= PORT_FLAG_AMPLITUDE;
  self->amount->id.flags |= PORT_FLAG_AUTOMATABLE;
  self->amount->id.flags2 |=
    PORT_FLAG2_CHANNEL_SEND_AMOUNT;
  SET_PORT_OWNER (amount);
  port_set_control_value (
    self->amount, 1.f, F_NOT_NORMALIZED,
    F_NO_PUBLISH_EVENTS);

  sprintf (
    name, _ ("Channel Send %d audio in"), slot + 1);
  sprintf (
    sym, "channel_send_%d_audio_in", slot + 1);
  self->stereo_in = stereo_ports_new_generic (
    F_INPUT, name, sym,
    PORT_OWNER_TYPE_CHANNEL_SEND, self);
  SET_PORT_OWNER (stereo_in->l);
  SET_PORT_OWNER (stereo_in->r);

  sprintf (
    name, _ ("Channel Send %d MIDI in"), slot + 1);
  self->midi_in = port_new_with_type (
    TYPE_EVENT, FLOW_INPUT, name);
  self->midi_in->id.sym = g_strdup_printf (
    "channel_send_%d_midi_in", slot + 1);
  SET_PORT_OWNER (midi_in);

  sprintf (
    name, _ ("Channel Send %d audio out"), slot + 1);
  sprintf (
    sym, "channel_send_%d_audio_out", slot + 1);
  self->stereo_out = stereo_ports_new_generic (
    F_NOT_INPUT, name, sym,
    PORT_OWNER_TYPE_CHANNEL_SEND, self);
  SET_PORT_OWNER (stereo_out->l);
  SET_PORT_OWNER (stereo_out->r);

  sprintf (
    name, _ ("Channel Send %d MIDI out"), slot + 1);
  self->midi_out = port_new_with_type (
    TYPE_EVENT, FLOW_OUTPUT, name);
  self->midi_out->id.sym = g_strdup_printf (
    "channel_send_%d_midi_out", slot + 1);
  SET_PORT_OWNER (midi_out);

#undef SET_PORT_OWNER

  return self;
}

Track *
channel_send_get_track (const ChannelSend * self)
{
  Track * track = self->track;
  g_return_val_if_fail (track, NULL);
  return track;
}

/**
 * Returns whether the channel send target is a
 * sidechain port (rather than a target track).
 */
bool
channel_send_is_target_sidechain (ChannelSend * self)
{
  return channel_send_is_enabled (self)
         && self->is_sidechain;
}

void
channel_send_prepare_process (ChannelSend * self)
{
  if (self->midi_in)
    {
      port_clear_buffer (self->midi_in);
      port_clear_buffer (self->midi_out);
    }
  if (self->stereo_in)
    {
      port_clear_buffer (self->stereo_in->l);
      port_clear_buffer (self->stereo_in->r);
      port_clear_buffer (self->stereo_out->l);
      port_clear_buffer (self->stereo_out->r);
    }
}

void
channel_send_process (
  ChannelSend *   self,
  const long      local_offset,
  const nframes_t nframes)
{
  if (channel_send_is_empty (self))
    return;

  Track * track = channel_send_get_track (self);
  if (track->out_signal_type == TYPE_AUDIO)
    {
      if (math_floats_equal_epsilon (
            self->amount->control, 1.f, 0.00001f))
        {
          dsp_copy (
            &self->stereo_out->l->buf[local_offset],
            &self->stereo_in->l->buf[local_offset],
            nframes);
          dsp_copy (
            &self->stereo_out->r->buf[local_offset],
            &self->stereo_in->r->buf[local_offset],
            nframes);
        }
      else
        {
          dsp_mix2 (
            &self->stereo_out->l->buf[local_offset],
            &self->stereo_in->l->buf[local_offset],
            1.f, self->amount->control, nframes);
          dsp_mix2 (
            &self->stereo_out->r->buf[local_offset],
            &self->stereo_in->r->buf[local_offset],
            1.f, self->amount->control, nframes);
        }
    }
  else if (track->out_signal_type == TYPE_EVENT)
    {
      midi_events_append (
        self->midi_out->midi_events,
        self->midi_in->midi_events, local_offset,
        nframes, F_NOT_QUEUED);
    }
}

void
channel_send_copy_values (
  ChannelSend *       dest,
  const ChannelSend * src)
{
  dest->slot = src->slot;
  port_set_control_value (
    dest->enabled, src->enabled->control,
    F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  port_set_control_value (
    dest->amount, src->amount->control,
    F_NOT_NORMALIZED, F_NO_PUBLISH_EVENTS);
  dest->is_sidechain = src->is_sidechain;
}

/**
 * Gets the target track.
 *
 * @param owner The owner track of the send
 *   (optional).
 */
Track *
channel_send_get_target_track (
  ChannelSend * self,
  Track *       owner)
{
  if (channel_send_is_empty (self))
    return NULL;

  PortType signal_type = get_signal_type (self);
  PortConnection * conn = NULL;
  switch (signal_type)
    {
    case TYPE_AUDIO:
      conn =
        port_connections_manager_get_source_or_dest (
          PORT_CONNECTIONS_MGR,
          &self->stereo_out->l->id, false);
      break;
    case TYPE_EVENT:
      conn =
        port_connections_manager_get_source_or_dest (
          PORT_CONNECTIONS_MGR, &self->midi_out->id,
          false);
      break;
    default:
      g_return_val_if_reached (NULL);
      break;
    }

  g_return_val_if_fail (conn, NULL);
  Port * port =
    port_find_from_identifier (conn->dest_id);
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (port), NULL);

  return port_get_track (port, true);
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
  g_return_val_if_fail (
    !channel_send_is_empty (self)
      && self->is_sidechain,
    NULL);

  PortType signal_type = get_signal_type (self);
  g_return_val_if_fail (
    signal_type == TYPE_AUDIO, NULL);

  PortConnection * conn =
    port_connections_manager_get_source_or_dest (
      PORT_CONNECTIONS_MGR,
      &self->stereo_out->l->id, false);
  g_return_val_if_fail (conn, NULL);
  Port * l =
    port_find_from_identifier (conn->dest_id);
  g_return_val_if_fail (l, NULL);

  conn = port_connections_manager_get_source_or_dest (
    PORT_CONNECTIONS_MGR, &self->stereo_out->r->id,
    false);
  g_return_val_if_fail (conn, NULL);
  Port * r =
    port_find_from_identifier (conn->dest_id);
  g_return_val_if_fail (r, NULL);

  StereoPorts * sp =
    stereo_ports_new_from_existing (l, r);

  return sp;
}

/**
 * Connects the ports to the owner track if not
 * connected.
 *
 * Only to be called on project sends.
 */
void
channel_send_connect_to_owner (ChannelSend * self)
{
  PortType signal_type = get_signal_type (self);
  switch (signal_type)
    {
    case TYPE_AUDIO:
      for (int i = 0; i < 2; i++)
        {
          Port * self_port =
            i == 0
              ? self->stereo_in->l
              : self->stereo_in->r;
          Port * src_port = NULL;
          if (channel_send_is_prefader (self))
            {
              src_port =
                i == 0
                  ? self->track->channel->prefader
                      ->stereo_out->l
                  : self->track->channel->prefader
                      ->stereo_out->r;
            }
          else
            {
              src_port =
                i == 0
                  ? self->track->channel->fader
                      ->stereo_out->l
                  : self->track->channel->fader
                      ->stereo_out->r;
            }

          /* make the connection if not exists */
          port_connections_manager_ensure_connect (
            PORT_CONNECTIONS_MGR, &src_port->id,
            &self_port->id, 1.f, F_LOCKED, F_ENABLE);
        }
      break;
    case TYPE_EVENT:
      {
        Port * self_port = self->midi_in;
        Port * src_port = NULL;
        if (channel_send_is_prefader (self))
          {
            src_port =
              self->track->channel->prefader
                ->midi_out;
          }
        else
          {
            src_port =
              self->track->channel->fader->midi_out;
          }

        /* make the connection if not exists */
        port_connections_manager_ensure_connect (
          PORT_CONNECTIONS_MGR, &src_port->id,
          &self_port->id, 1.f, F_LOCKED, F_ENABLE);
      }
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

  return math_get_fader_val_from_amp (
    self->amount->control);
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

  channel_send_set_amount (
    self, math_get_amp_val_from_fader (val));
}

/**
 * Connects a send to stereo ports.
 *
 * This function takes either \ref stereo or both
 * \ref l and \ref r.
 */
bool
channel_send_connect_stereo (
  ChannelSend * self,
  StereoPorts * stereo,
  Port *        l,
  Port *        r,
  bool          sidechain,
  bool          recalc_graph,
  bool          validate,
  GError **     error)
{
  /* verify can be connected */
  if (validate && port_is_in_active_project (l))
    {
      Port * src = port_find_from_identifier (
        &self->stereo_out->l->id);
      if (!ports_can_be_connected (src, l))
        {
          g_set_error_literal (
            error, Z_AUDIO_CHANNEL_SEND_ERROR,
            Z_AUDIO_CHANNEL_SEND_ERROR_FAILED,
            _ ("Ports cannot be connected"));
          return false;
        }
    }

  channel_send_disconnect (self, false);

  /* connect */
  if (stereo)
    {
      l = stereo->l;
      r = stereo->r;
    }

  port_connections_manager_ensure_connect (
    PORT_CONNECTIONS_MGR, &self->stereo_out->l->id,
    &l->id, 1.f, F_LOCKED, F_ENABLE);
  port_connections_manager_ensure_connect (
    PORT_CONNECTIONS_MGR, &self->stereo_out->r->id,
    &r->id, 1.f, F_LOCKED, F_ENABLE);

  port_set_control_value (
    self->enabled, 1.f, F_NOT_NORMALIZED,
    F_PUBLISH_EVENTS);
  self->is_sidechain = sidechain;

#if 0
  /* set multipliers */
  channel_send_update_connections (self);
#endif

  if (recalc_graph)
    router_recalc_graph (ROUTER, F_NOT_SOFT);

  return true;
}

/**
 * Connects a send to a midi port.
 */
bool
channel_send_connect_midi (
  ChannelSend * self,
  Port *        port,
  bool          recalc_graph,
  bool          validate,
  GError **     error)
{
  /* verify can be connected */
  if (validate && port_is_in_active_project (port))
    {
      Port * src = port_find_from_identifier (
        &self->midi_out->id);
      if (!ports_can_be_connected (src, port))
        {
          g_set_error_literal (
            error, Z_AUDIO_CHANNEL_SEND_ERROR,
            Z_AUDIO_CHANNEL_SEND_ERROR_FAILED,
            _ ("Ports cannot be connected"));
          return false;
        }
    }

  channel_send_disconnect (self, false);

  port_connections_manager_ensure_connect (
    PORT_CONNECTIONS_MGR, &self->midi_out->id,
    &port->id, 1.f, F_LOCKED, F_ENABLE);

  port_set_control_value (
    self->enabled, 1.f, F_NOT_NORMALIZED,
    F_PUBLISH_EVENTS);

  if (recalc_graph)
    router_recalc_graph (ROUTER, F_NOT_SOFT);

  return true;
}

static void
disconnect_midi (ChannelSend * self)
{
  PortConnection * conn =
    port_connections_manager_get_source_or_dest (
      PORT_CONNECTIONS_MGR, &self->midi_out->id,
      false);
  if (!conn)
    return;

  Port * dest_port =
    port_find_from_identifier (conn->dest_id);
  g_return_if_fail (dest_port);

  port_connections_manager_ensure_disconnect (
    PORT_CONNECTIONS_MGR, &self->midi_out->id,
    &dest_port->id);
}

static void
disconnect_audio (ChannelSend * self)
{
  for (int i = 0; i < 2; i++)
    {
      Port * src_port =
        i == 0
          ? self->stereo_out->l
          : self->stereo_out->r;
      PortConnection * conn =
        port_connections_manager_get_source_or_dest (
          PORT_CONNECTIONS_MGR, &src_port->id, false);
      if (!conn)
        continue;

      Port * dest_port =
        port_find_from_identifier (conn->dest_id);
      g_return_if_fail (dest_port);
      port_connections_manager_ensure_disconnect (
        PORT_CONNECTIONS_MGR, &src_port->id,
        &dest_port->id);
    }
}

/**
 * Removes the connection at the given send.
 */
void
channel_send_disconnect (
  ChannelSend * self,
  bool          recalc_graph)
{
  if (channel_send_is_empty (self))
    return;

  PortType signal_type = get_signal_type (self);

  switch (signal_type)
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

  port_set_control_value (
    self->enabled, 0.f, F_NOT_NORMALIZED,
    F_PUBLISH_EVENTS);
  self->is_sidechain = false;

  if (recalc_graph)
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
 * Get the name of the destination, or a placeholder
 * text if empty.
 */
void
channel_send_get_dest_name (
  ChannelSend * self,
  char *        buf)
{
  if (channel_send_is_empty (self))
    {
      if (channel_send_is_prefader (self))
        strcpy (buf, _ ("Pre-fader send"));
      else
        strcpy (buf, _ ("Post-fader send"));
    }
  else
    {
      PortType type = get_signal_type (self);
      Port *   search_port =
        (type == TYPE_AUDIO)
            ? self->stereo_out->l
            : self->midi_out;
      PortConnection * conn =
        port_connections_manager_get_source_or_dest (
          PORT_CONNECTIONS_MGR, &search_port->id,
          false);
      g_return_if_fail (conn);
      Port * dest =
        port_find_from_identifier (conn->dest_id);
      g_return_if_fail (IS_PORT_AND_NONNULL (dest));
      if (self->is_sidechain)
        {
          Plugin * pl = port_get_plugin (dest, true);
          g_return_if_fail (IS_PLUGIN (pl));
          plugin_get_full_port_group_designation (
            pl, dest->id.port_group, buf);
        }
      else /* else if not sidechain */
        {
          switch (dest->id.owner_type)
            {
            case PORT_OWNER_TYPE_TRACK_PROCESSOR:
              {
                Track * track =
                  port_get_track (dest, true);
                g_return_if_fail (
                  IS_TRACK_AND_NONNULL (track));
                sprintf (
                  buf, _ ("%s input"), track->name);
              }
              break;
            default:
              break;
            }
        }
    }
}

ChannelSend *
channel_send_clone (const ChannelSend * src)
{
  ChannelSend * self = channel_send_new (
    src->track_name_hash, src->slot, NULL);

  self->amount->control = src->amount->control;
  self->enabled->control = src->enabled->control;
  self->is_sidechain = src->is_sidechain;
  self->track_name_hash = src->track_name_hash;

  g_return_val_if_fail (
    self->track_name_hash != 0
      && self->track_name_hash == src->track_name_hash,
    NULL);

  return self;
}

bool
channel_send_is_enabled (const ChannelSend * self)
{
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (self->enabled), false);

  bool enabled =
    control_port_is_toggled (self->enabled);

  if (!enabled)
    return false;

  PortType signal_type = get_signal_type (self);
  Port *   search_port =
    (signal_type == TYPE_AUDIO)
        ? self->stereo_out->l
        : self->midi_out;

  if (G_LIKELY (router_is_processing_thread (ROUTER)))
    {
      if (search_port->num_dests == 1)
        {
          Port * dest = search_port->dests[0];
          g_return_val_if_fail (
            IS_PORT_AND_NONNULL (dest), false);

          if (
            dest->id.owner_type
            == PORT_OWNER_TYPE_PLUGIN)
            {
              Plugin * pl =
                plugin_find (&dest->id.plugin_id);
              g_return_val_if_fail (
                IS_PLUGIN_AND_NONNULL (pl), false);
              if (pl->instantiation_failed)
                return false;
            }

          return true;
        }
      else
        return false;
    }

  /* get dest port */
  PortConnection * conn =
    port_connections_manager_get_source_or_dest (
      PORT_CONNECTIONS_MGR, &search_port->id, false);
  g_return_val_if_fail (conn, false);
  Port * dest =
    port_find_from_identifier (conn->dest_id);
  g_return_val_if_fail (
    IS_PORT_AND_NONNULL (dest), false);

  /* if dest port is a plugin port and plugin
   * instantiation failed, assume that the send
   * is disabled */
  if (dest->id.owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      Plugin * pl =
        plugin_find (&dest->id.plugin_id);
      if (pl->instantiation_failed)
        enabled = false;
    }

  return enabled;
}

ChannelSendWidget *
channel_send_find_widget (ChannelSend * self)
{
  if (ZRYTHM_HAVE_UI && MAIN_WINDOW)
    {
      return MW_TRACK_INSPECTOR->sends
        ->slots[self->slot];
    }
  return NULL;
}

/**
 * Finds the project send from a given send
 * instance.
 */
ChannelSend *
channel_send_find (ChannelSend * self)
{
  Track * track = tracklist_find_track_by_name_hash (
    TRACKLIST, self->track_name_hash);
  g_return_val_if_fail (
    IS_TRACK_AND_NONNULL (track), NULL);

  return track->channel->sends[self->slot];
}

bool
channel_send_validate (ChannelSend * self)
{
  if (channel_send_is_enabled (self))
    {
      PortType signal_type = get_signal_type (self);
      if (signal_type == TYPE_AUDIO)
        {
          int num_dests =
            port_connections_manager_get_sources_or_dests (
              PORT_CONNECTIONS_MGR, NULL,
              &self->stereo_out->l->id, false);
          g_return_val_if_fail (
            num_dests == 1, false);
          num_dests =
            port_connections_manager_get_sources_or_dests (
              PORT_CONNECTIONS_MGR, NULL,
              &self->stereo_out->r->id, false);
          g_return_val_if_fail (
            num_dests == 1, false);
        }
      else if (signal_type == TYPE_EVENT)
        {
          int num_dests =
            port_connections_manager_get_sources_or_dests (
              PORT_CONNECTIONS_MGR, NULL,
              &self->midi_out->id, false);
          g_return_val_if_fail (
            num_dests == 1, false);
        }
    } /* endif channel send is enabled */

  return true;
}

void
channel_send_append_ports (
  ChannelSend * self,
  GPtrArray *   ports)
{
#define _ADD(port) \
  g_warn_if_fail (port); \
  g_ptr_array_add (ports, port)

  _ADD (self->enabled);
  _ADD (self->amount);
  if (self->midi_in)
    {
      _ADD (self->midi_in);
    }
  if (self->midi_out)
    {
      _ADD (self->midi_out);
    }
  if (self->stereo_in)
    {
      _ADD (self->stereo_in->l);
      _ADD (self->stereo_in->r);
    }
  if (self->stereo_out)
    {
      _ADD (self->stereo_out->l);
      _ADD (self->stereo_out->r);
    }

#undef _ADD
}

/**
 * Appends the connection(s), if non-empty, to the
 * given array (if not NULL) and returns the number
 * of connections added.
 */
int
channel_send_append_connection (
  const ChannelSend *            self,
  const PortConnectionsManager * mgr,
  GPtrArray *                    arr)
{
  if (channel_send_is_empty (self))
    return 0;

  PortType signal_type = get_signal_type (self);
  if (signal_type == TYPE_AUDIO)
    {
      int num_dests =
        port_connections_manager_get_sources_or_dests (
          mgr, arr, &self->stereo_out->l->id, false);
      g_return_val_if_fail (num_dests == 1, false);
      num_dests =
        port_connections_manager_get_sources_or_dests (
          mgr, arr, &self->stereo_out->r->id, false);
      g_return_val_if_fail (num_dests == 1, false);
      return 2;
    }
  else if (signal_type == TYPE_EVENT)
    {
      int num_dests =
        port_connections_manager_get_sources_or_dests (
          mgr, arr, &self->midi_out->id, false);
      g_return_val_if_fail (num_dests == 1, false);
      return 1;
    }

  g_return_val_if_reached (0);
}

/**
 * Returns whether the send is connected to the
 * given ports.
 */
bool
channel_send_is_connected_to (
  const ChannelSend * self,
  const StereoPorts * stereo,
  const Port *        midi)
{
  GPtrArray * conns = g_ptr_array_new ();
  int num_conns = channel_send_append_connection (
    self, PORT_CONNECTIONS_MGR, conns);
  bool ret = false;
  for (int i = 0; i < num_conns; i++)
    {
      PortConnection * conn =
        g_ptr_array_index (conns, (size_t) i);
      if ((stereo
           &&
           (port_identifier_is_equal (
              conn->dest_id, &stereo->l->id)
            ||
            port_identifier_is_equal (
              conn->dest_id, &stereo->r->id)))
          ||
          (midi
           &&
           port_identifier_is_equal (
             conn->dest_id, &midi->id)))
        {
          ret = true;
          break;
        }
    }
  g_ptr_array_unref (conns);

  return ret;
}

void
channel_send_free (ChannelSend * self)
{
  object_free_w_func_and_null (
    port_free, self->amount);
  object_free_w_func_and_null (
    port_free, self->enabled);
  object_free_w_func_and_null (
    stereo_ports_free, self->stereo_in);
  object_free_w_func_and_null (
    port_free, self->midi_in);
  object_free_w_func_and_null (
    stereo_ports_free, self->stereo_out);
  object_free_w_func_and_null (
    port_free, self->midi_out);

  object_zero_and_free (self);
}
