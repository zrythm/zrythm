/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __ACTION_PORT_CONNECTION_ACTION_H__
#define __ACTION_PORT_CONNECTION_ACTION_H__

#include "actions/undoable_action.h"
#include "audio/port_identifier.h"

/**
 * @addtogroup actions
 *
 * @{
 */

typedef enum PortConnectionActionType
{
  PORT_CONNECTION_CONNECT,
  PORT_CONNECTION_DISCONNECT,
  PORT_CONNECTION_ENABLE,
  PORT_CONNECTION_DISABLE,
  PORT_CONNECTION_CHANGE_MULTIPLIER,
} PortConnectionActionType;

static const cyaml_strval_t
port_connection_action_type_strings[] =
{
  { "connect",     PORT_CONNECTION_CONNECT },
  { "disconnect",  PORT_CONNECTION_CONNECT },
  { "enable",      PORT_CONNECTION_ENABLE },
  { "disable",     PORT_CONNECTION_DISABLE },
  { "change multiplier",
    PORT_CONNECTION_CHANGE_MULTIPLIER },
};

typedef struct PortConnectionAction
{
  UndoableAction  parent_instance;

  PortConnectionActionType type;

  PortIdentifier  src_port_id;
  PortIdentifier  dest_port_id;

  /**
   * Value before/after the change.
   *
   * To be swapped on undo/redo.
   */
  float           val;
} PortConnectionAction;

static const cyaml_schema_field_t
  port_connection_action_fields_schema[] =
{
  YAML_FIELD_MAPPING_EMBEDDED (
    PortConnectionAction, parent_instance,
    undoable_action_fields_schema),
  YAML_FIELD_ENUM (
    PortConnectionAction, type,
    port_connection_action_type_strings),
  YAML_FIELD_MAPPING_EMBEDDED (
    PortConnectionAction, src_port_id,
    port_identifier_fields_schema),
  YAML_FIELD_MAPPING_EMBEDDED (
    PortConnectionAction, dest_port_id,
    port_identifier_fields_schema),
  YAML_FIELD_FLOAT (
    PortConnectionAction, val),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
  port_connection_action_schema =
{
  YAML_VALUE_PTR (
    PortConnectionAction,
    port_connection_action_fields_schema),
};

void
port_connection_action_init_loaded (
  PortConnectionAction * self);

/**
 * Create a new action.
 */
UndoableAction *
port_connection_action_new (
  PortConnectionActionType type,
  PortIdentifier *         src_id,
  PortIdentifier *         dest_id,
  float                    new_val);

#define port_connection_action_new_connect( \
  src_id,dest_id) \
  port_connection_action_new ( \
    PORT_CONNECTION_CONNECT, \
    src_id, dest_id, 0.f)

#define port_connection_action_new_disconnect( \
  src_id,dest_id) \
  port_connection_action_new ( \
    PORT_CONNECTION_DISCONNECT, \
    src_id, dest_id, 0.f)

#define port_connection_action_new_enable( \
  src_id,dest_id,enable) \
  port_connection_action_new ( \
    enable ? \
      PORT_CONNECTION_ENABLE : \
      PORT_CONNECTION_DISABLE, \
    src_id, dest_id, 0.f)

#define port_connection_action_new_change_multiplier( \
  src_id,dest_id, new_multiplier) \
  port_connection_action_new ( \
    PORT_CONNECTION_CHANGE_MULTIPLIER, src_id, \
    dest_id, new_multiplier)

int
port_connection_action_do (
  PortConnectionAction * self);

int
port_connection_action_undo (
  PortConnectionAction * self);

char *
port_connection_action_stringize (
  PortConnectionAction * self);

void
port_connection_action_free (
  PortConnectionAction * self);

/**
 * @}
 */

#endif
