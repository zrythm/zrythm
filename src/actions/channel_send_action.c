// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/channel_send_action.h"
#include "audio/channel.h"
#include "audio/router.h"
#include "gui/backend/event.h"
#include "gui/backend/event_manager.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "project.h"
#include "utils/error.h"
#include "utils/flags.h"
#include "utils/objects.h"
#include "zrythm_app.h"

#include <glib/gi18n.h>

void
channel_send_action_init_loaded (ChannelSendAction * self)
{
}

/**
 * Creates a new action.
 *
 * @param port MIDI port, if connecting MIDI.
 * @param stereo Stereo ports, if connecting audio.
 * @param port_connections_mgr Port connections
 *   manager at the start of the action, if needed.
 */
UndoableAction *
channel_send_action_new (
  ChannelSend *                  send,
  ChannelSendActionType          type,
  Port *                         port,
  StereoPorts *                  stereo,
  float                          amount,
  const PortConnectionsManager * port_connections_mgr,
  GError **                      error)
{
  ChannelSendAction * self = object_new (ChannelSendAction);
  UndoableAction *    ua = (UndoableAction *) self;
  undoable_action_init (ua, UA_CHANNEL_SEND);

  self->type = type;
  self->send_before = channel_send_clone (send);
  self->amount = amount;

  if (port)
    self->midi_id = port_identifier_clone (&port->id);

  if (stereo)
    {
      self->l_id = port_identifier_clone (&stereo->l->id);
      self->r_id = port_identifier_clone (&stereo->r->id);
    }

  if (port_connections_mgr)
    {
      self->connections_mgr_before =
        port_connections_manager_clone (port_connections_mgr);
    }

  return ua;
}

ChannelSendAction *
channel_send_action_clone (const ChannelSendAction * src)
{
  ChannelSendAction * self = object_new (ChannelSendAction);
  self->parent_instance = src->parent_instance;

  self->send_before = channel_send_clone (src->send_before);
  self->type = src->type;
  self->amount = src->amount;

  if (src->connections_mgr_before)
    self->connections_mgr_before =
      port_connections_manager_clone (
        src->connections_mgr_before);
  if (src->connections_mgr_after)
    self->connections_mgr_after =
      port_connections_manager_clone (
        src->connections_mgr_after);

  return self;
}

/**
 * Wrapper to create action and perform it.
 *
 * @param port_connections_mgr Port connections
 *   manager at the start of the action, if needed.
 */
bool
channel_send_action_perform (
  ChannelSend *                  send,
  ChannelSendActionType          type,
  Port *                         port,
  StereoPorts *                  stereo,
  float                          amount,
  const PortConnectionsManager * port_connections_mgr,
  GError **                      error)
{
  UNDO_MANAGER_PERFORM_AND_PROPAGATE_ERR (
    channel_send_action_new, error, send, type, port, stereo,
    amount, port_connections_mgr, error);
}

static bool
connect_or_disconnect (
  ChannelSendAction * self,
  bool                connect,
  bool                _do,
  GError **           error)
{
  /* get the actual channel send from the project */
  ChannelSend * send = channel_send_find (self->send_before);

  channel_send_disconnect (send, F_NO_RECALC_GRAPH);

  if (_do)
    {
      if (connect)
        {
          Track * track = channel_send_get_track (send);
          switch (track->out_signal_type)
            {
            case TYPE_EVENT:
              {
                Port * port =
                  port_find_from_identifier (self->midi_id);
                GError * err = NULL;
                bool connected = channel_send_connect_midi (
                  send, port, F_NO_RECALC_GRAPH, F_VALIDATE,
                  &err);
                if (!connected)
                  {
                    PROPAGATE_PREFIXED_ERROR (
                      error, err, "%s",
                      _ ("Failed to connect MIDI "
                         "send"));
                    return false;
                  }
              }
              break;
            case TYPE_AUDIO:
              {
                Port * l =
                  port_find_from_identifier (self->l_id);
                Port * r =
                  port_find_from_identifier (self->r_id);
                GError * err = NULL;
                bool connected = channel_send_connect_stereo (
                  send, NULL, l, r,
                  self->type
                    == CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN,
                  F_NO_RECALC_GRAPH, F_VALIDATE, &err);
                if (!connected)
                  {
                    PROPAGATE_PREFIXED_ERROR (
                      error, err, "%s",
                      _ ("Failed to connect audio "
                         "send"));
                    return false;
                  }
              }
              break;
            default:
              break;
            }
        }
    }
  /* else if not doing */
  else
    {
      /* copy the values - connections will be
       * reverted later when resetting the
       * connections manager */
      channel_send_copy_values (send, self->send_before);
    }

  return true;
}

int
channel_send_action_do (
  ChannelSendAction * self,
  GError **           error)
{
  /* get the actual channel send from the project */
  ChannelSend * send = channel_send_find (self->send_before);

  bool need_restore_and_recalc = false;

  bool     successful = false;
  GError * err = NULL;
  switch (self->type)
    {
    case CHANNEL_SEND_ACTION_CONNECT_MIDI:
    case CHANNEL_SEND_ACTION_CONNECT_STEREO:
    case CHANNEL_SEND_ACTION_CHANGE_PORTS:
    case CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN:
      successful =
        connect_or_disconnect (self, true, true, &err);
      need_restore_and_recalc = true;
      break;
    case CHANNEL_SEND_ACTION_DISCONNECT:
      successful =
        connect_or_disconnect (self, false, true, &err);
      need_restore_and_recalc = true;
      break;
    case CHANNEL_SEND_ACTION_CHANGE_AMOUNT:
      successful = true;
      channel_send_set_amount (send, self->amount);
      break;
    default:
      break;
    }

  if (!successful)
    {
      g_return_val_if_fail (err, -1);

      PROPAGATE_PREFIXED_ERROR (
        error, err,
        _ ("Failed to perform channel send action: %s"),
        err->message);
      return -1;
    }

  if (need_restore_and_recalc)
    {
      undoable_action_save_or_load_port_connections (
        (UndoableAction *) self, true,
        &self->connections_mgr_before,
        &self->connections_mgr_after);

      router_recalc_graph (ROUTER, F_NOT_SOFT);
    }

  EVENTS_PUSH (ET_CHANNEL_SEND_CHANGED, send);

  return 0;
}

/**
 * Edits the plugin.
 */
int
channel_send_action_undo (
  ChannelSendAction * self,
  GError **           error)
{
  /* get the actual channel send from the project */
  ChannelSend * send = channel_send_find (self->send_before);

  bool need_restore_and_recalc = false;

  bool     successful = false;
  GError * err = NULL;
  switch (self->type)
    {
    case CHANNEL_SEND_ACTION_CONNECT_MIDI:
    case CHANNEL_SEND_ACTION_CONNECT_STEREO:
    case CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN:
      successful =
        connect_or_disconnect (self, false, true, &err);
      need_restore_and_recalc = true;
      break;
    case CHANNEL_SEND_ACTION_CHANGE_PORTS:
    case CHANNEL_SEND_ACTION_DISCONNECT:
      successful =
        connect_or_disconnect (self, true, false, &err);
      need_restore_and_recalc = true;
      break;
    case CHANNEL_SEND_ACTION_CHANGE_AMOUNT:
      channel_send_set_amount (
        send, self->send_before->amount->control);
      successful = true;
      break;
    default:
      break;
    }

  if (!successful)
    {
      g_return_val_if_fail (err, -1);

      PROPAGATE_PREFIXED_ERROR (
        error, err,
        _ ("Failed to perform channel send action: %s"),
        err->message);
      return -1;
    }

  if (need_restore_and_recalc)
    {
      undoable_action_save_or_load_port_connections (
        (UndoableAction *) self, false,
        &self->connections_mgr_before,
        &self->connections_mgr_after);

      router_recalc_graph (ROUTER, F_NOT_SOFT);

      tracklist_validate (TRACKLIST);
    }

  EVENTS_PUSH (ET_CHANNEL_SEND_CHANGED, send);

  return 0;
}

char *
channel_send_action_stringize (ChannelSendAction * self)
{
  switch (self->type)
    {
    case CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN:
      return g_strdup (_ ("Connect sidechain"));
    case CHANNEL_SEND_ACTION_CONNECT_STEREO:
      return g_strdup (_ ("Connect stereo"));
    case CHANNEL_SEND_ACTION_CONNECT_MIDI:
      return g_strdup (_ ("Connect MIDI"));
    case CHANNEL_SEND_ACTION_DISCONNECT:
      return g_strdup (_ ("Disconnect"));
    case CHANNEL_SEND_ACTION_CHANGE_AMOUNT:
      return g_strdup (_ ("Change amount"));
    case CHANNEL_SEND_ACTION_CHANGE_PORTS:
      return g_strdup (_ ("Change ports"));
    }
  g_return_val_if_reached (
    g_strdup (_ ("Channel send connection")));
}

void
channel_send_action_free (ChannelSendAction * self)
{
  object_free_w_func_and_null (
    channel_send_free, self->send_before);
  object_free_w_func_and_null (
    port_identifier_free, self->l_id);
  object_free_w_func_and_null (
    port_identifier_free, self->r_id);
  object_free_w_func_and_null (
    port_identifier_free, self->midi_id);
  object_free_w_func_and_null (
    port_connections_manager_free,
    self->connections_mgr_before);
  object_free_w_func_and_null (
    port_connections_manager_free,
    self->connections_mgr_after);

  object_zero_and_free (self);
}
