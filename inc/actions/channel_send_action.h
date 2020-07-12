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

#ifndef __UNDO_CHANNEL_SEND_ACTION_H__
#define __UNDO_CHANNEL_SEND_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/channel_send.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum ChannelSendActionType
{
  CHANNEL_SEND_ACTION_CONNECT_STEREO,
  CHANNEL_SEND_ACTION_CONNECT_MIDI,
  CHANNEL_SEND_ACTION_CHANGE_AMOUNT,
  CHANNEL_SEND_ACTION_CHANGE_PORTS,
  CHANNEL_SEND_ACTION_DISCONNECT,
} ChannelSendActionType;

static const cyaml_strval_t
  channel_send_action_type_strings[] =
{
  { "Connect stereo",
    CHANNEL_SEND_ACTION_CONNECT_STEREO },
  { "Connect MIDI",
    CHANNEL_SEND_ACTION_CONNECT_MIDI },
  { "Change amount",
    CHANNEL_SEND_ACTION_CHANGE_PORTS },
  { "Change ports",
    CHANNEL_SEND_ACTION_CHANGE_PORTS },
  { "Disconnect",
    CHANNEL_SEND_ACTION_DISCONNECT },
};

typedef struct ChannelSendAction
{
  UndoableAction  parent_instance;

  ChannelSend *   send_before;
  ChannelSend *   send_after;

  /** Action type. */
  ChannelSendActionType type;

} ChannelSendAction;

static const cyaml_schema_field_t
  channel_send_action_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    ChannelSendAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    ChannelSendAction, send_before,
    channel_send_fields_schema),
  YAML_FIELD_MAPPING_PTR (
    ChannelSendAction, send_after,
    channel_send_fields_schema),
  YAML_FIELD_ENUM (
    ChannelSendAction, type,
    channel_send_action_type_strings),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  channel_send_action_schema =
{
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER, ChannelSendAction,
    channel_send_action_fields_schema),
};

void
channel_send_action_init_loaded (
  ChannelSendAction * self);

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
  float                 amount);

#define channel_send_action_new_disconnect(send) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_DISCONNECT, NULL, NULL, \
    0.f)

#define channel_send_action_new_connect_midi( \
  send,midi) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_CONNECT_MIDI, midi, \
    NULL, 0.f)

#define channel_send_action_new_connect_audio( \
  send,stereo) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_CONNECT_STEREO, NULL, \
    stereo, 0.f)

#define channel_send_action_new_change_amount( \
  send,amt) \
  channel_send_action_new ( \
    send, CHANNEL_SEND_ACTION_CHANGE_AMOUNT, NULL, \
    NULL, amt)

int
channel_send_action_do (
  ChannelSendAction * self);

int
channel_send_action_undo (
  ChannelSendAction * self);

char *
channel_send_action_stringize (
  ChannelSendAction * self);

void
channel_send_action_free (
  ChannelSendAction * self);

/**
 * @}
 */

#endif
