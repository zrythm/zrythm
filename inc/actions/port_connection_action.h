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
  PORT_CONNECTION_EDIT,
} PortConnectionActionType;

static const cyaml_strval_t
port_connection_action_type_strings[] =
{
  { "connect",     PORT_CONNECTION_CONNECT },
  { "disconnect",  PORT_CONNECTION_CONNECT },
  { "edit",        PORT_CONNECTION_EDIT },
};

typedef struct PortConnectionAction
{
  UndoableAction  parent_instance;

  PortConnectionActionType type;

  PortIdentifier  src_port_id;
  PortIdentifier  dest_port_id;
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

UndoableAction *
port_connection_action_new (
  PortConnectionActionType type,
  PortIdentifier *         src_id,
  PortIdentifier *         dest_id);

#define port_connection_action_new_connect( \
  src_id,dest_id) \
  port_connection_action_new ( \
    PORT_CONNECTION_CONNECT, src_id, dest_id)

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
