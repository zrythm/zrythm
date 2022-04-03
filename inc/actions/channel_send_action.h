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

#ifndef __UNDO_CHANNEL_SEND_ACTION_H__
#define __UNDO_CHANNEL_SEND_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/channel_send.h"
#include "audio/port_connections_manager.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum ChannelSendActionType
{
  CHANNEL_SEND_ACTION_CONNECT_STEREO,
  CHANNEL_SEND_ACTION_CONNECT_MIDI,
  CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN,
  CHANNEL_SEND_ACTION_CHANGE_AMOUNT,
  CHANNEL_SEND_ACTION_CHANGE_PORTS,
  CHANNEL_SEND_ACTION_DISCONNECT,
} ChannelSendActionType;

static const cyaml_strval_t
  channel_send_action_type_strings[] = {
    {"Connect stereo",
     CHANNEL_SEND_ACTION_CONNECT_STEREO                  },
    { "Connect MIDI",
     CHANNEL_SEND_ACTION_CONNECT_MIDI                    },
    { "Connect sidechain",
     CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN               },
    { "Change amount",
     CHANNEL_SEND_ACTION_CHANGE_AMOUNT                   },
    { "Change ports",
     CHANNEL_SEND_ACTION_CHANGE_PORTS                    },
    { "Disconnect",        CHANNEL_SEND_ACTION_DISCONNECT},
};

/**
 * Action for channel send changes.
 */
typedef struct ChannelSendAction
{
  UndoableAction parent_instance;

  ChannelSend * send_before;

  float amount;

  /** Target port identifiers. */
  PortIdentifier * l_id;
  PortIdentifier * r_id;
  PortIdentifier * midi_id;

  /** A clone of the port connections at the
   * start of the action. */
  PortConnectionsManager * connections_mgr_before;

  /** A clone of the port connections after
   * applying the action. */
  PortConnectionsManager * connections_mgr_after;

  /** Action type. */
  ChannelSendActionType type;

} ChannelSendAction;

static const cyaml_schema_field_t
  channel_send_action_fields_schema[] = {
    YAML_FIELD_MAPPING_EMBEDDED (
      ChannelSendAction,
      parent_instance,
      undoable_action_fields_schema),
    YAML_FIELD_ENUM (
      ChannelSendAction,
      type,
      channel_send_action_type_strings),
    YAML_FIELD_MAPPING_PTR (
      ChannelSendAction,
      send_before,
      channel_send_fields_schema),
    YAML_FIELD_MAPPING_PTR_OPTIONAL (
      ChannelSendAction,
      connections_mgr_before,
      port_connections_manager_fields_schema),
    YAML_FIELD_MAPPING_PTR_OPTIONAL (
      ChannelSendAction,
      connections_mgr_after,
      port_connections_manager_fields_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  channel_send_action_schema = {
    CYAML_VALUE_MAPPING (
      CYAML_FLAG_POINTER,
      ChannelSendAction,
      channel_send_action_fields_schema),
  };

void
channel_send_action_init_loaded (
  ChannelSendAction * self);

/**
 * Creates a new action.
 *
 * @param port MIDI port, if connecting MIDI.
 * @param stereo Stereo ports, if connecting audio.
 * @param port_connections_mgr Port connections
 *   manager at the start of the action, if needed.
 */
WARN_UNUSED_RESULT
UndoableAction *
channel_send_action_new (
  ChannelSend *                  send,
  ChannelSendActionType          type,
  Port *                         port,
  StereoPorts *                  stereo,
  float                          amount,
  const PortConnectionsManager * port_connections_mgr,
  GError **                      error);

#define channel_send_action_new_disconnect( \
  send, error) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_DISCONNECT, NULL, \
    NULL, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_new_connect_midi( \
  send, midi, error) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_CONNECT_MIDI, midi, \
    NULL, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_new_connect_audio( \
  send, stereo, error) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_CONNECT_STEREO, \
    NULL, stereo, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_new_connect_sidechain( \
  send, stereo, error) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN, \
    NULL, stereo, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_new_change_amount( \
  send, amt, error) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_CHANGE_AMOUNT, NULL, \
    NULL, amt, NULL, error)

NONNULL
ChannelSendAction *
channel_send_action_clone (
  const ChannelSendAction * src);

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
  GError **                      error);

#define channel_send_action_perform_disconnect( \
  send, error) \
  channel_send_action_perform ( \
    send, CHANNEL_SEND_ACTION_DISCONNECT, NULL, \
    NULL, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_perform_connect_midi( \
  send, midi, error) \
  channel_send_action_perform ( \
    send, CHANNEL_SEND_ACTION_CONNECT_MIDI, midi, \
    NULL, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_perform_connect_audio( \
  send, stereo, error) \
  channel_send_action_perform ( \
    send, CHANNEL_SEND_ACTION_CONNECT_STEREO, \
    NULL, stereo, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_perform_connect_sidechain( \
  send, stereo, error) \
  channel_send_action_perform ( \
    send, CHANNEL_SEND_ACTION_CONNECT_SIDECHAIN, \
    NULL, stereo, 0.f, PORT_CONNECTIONS_MGR, error)

#define channel_send_action_perform_change_amount( \
  send, amt, error) \
  channel_send_action_perform ( \
    send, CHANNEL_SEND_ACTION_CHANGE_AMOUNT, NULL, \
    NULL, amt, NULL, error)

int
channel_send_action_do (
  ChannelSendAction * self,
  GError **           error);

int
channel_send_action_undo (
  ChannelSendAction * self,
  GError **           error);

char *
channel_send_action_stringize (
  ChannelSendAction * self);

void
channel_send_action_free (ChannelSendAction * self);

/**
 * @}
 */

#endif
