/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * Port connection.
 */

#ifndef __AUDIO_PORT_CONNECTION_H__
#define __AUDIO_PORT_CONNECTION_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "audio/port_identifier.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define PORT_CONNECTION_SCHEMA_VERSION 1

/**
 * A connection between two ports.
 */
typedef struct PortConnection
{
  int schema_version;

  PortIdentifier * src_id;
  PortIdentifier * dest_id;

  /**
   * Multiplier to apply, where applicable.
   *
   * Range: 0 to 1.
   * Default: 1.
   */
  float multiplier;

  /**
   * Whether the connection can be removed or the
   * multiplier edited by the user.
   *
   * Ignored when connecting things internally and
   * only used to deter the user from breaking
   * necessary connections.
   */
  bool locked;

  /**
   * Whether the connection is enabled.
   *
   * @note The user can disable port connections only
   * if they are not locked.
   */
  bool enabled;

  /** Used for CV -> control port connections. */
  float base_value;
} PortConnection;

static const cyaml_schema_field_t
  port_connection_fields_schema[] = {
    YAML_FIELD_INT (PortConnection, schema_version),
    YAML_FIELD_MAPPING_PTR (
      PortConnection,
      src_id,
      port_identifier_fields_schema),
    YAML_FIELD_MAPPING_PTR (
      PortConnection,
      dest_id,
      port_identifier_fields_schema),
    YAML_FIELD_FLOAT (PortConnection, multiplier),
    YAML_FIELD_INT (PortConnection, locked),
    YAML_FIELD_INT (PortConnection, enabled),
    YAML_FIELD_FLOAT (PortConnection, base_value),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  port_connection_schema = {
    YAML_VALUE_PTR (
      PortConnection,
      port_connection_fields_schema),
  };

PortConnection *
port_connection_new (
  const PortIdentifier * src,
  const PortIdentifier * dest,
  float                  multiplier,
  bool                   locked,
  bool                   enabled);

NONNULL
void
port_connection_update (
  PortConnection * self,
  float            multiplier,
  bool             locked,
  bool             enabled);

NONNULL
PURE bool
port_connection_is_send (
  const PortConnection * self);

NONNULL
void
port_connection_print_to_str (
  const PortConnection * self,
  char *                 buf,
  size_t                 buf_sz);

NONNULL
void
port_connection_print (const PortConnection * self);

/**
 * To be used during serialization.
 */
NONNULL
PortConnection *
port_connection_clone (const PortConnection * src);

/**
 * Deletes port, doing required cleanup and updating counters.
 */
NONNULL
void
port_connection_free (PortConnection * self);

/**
 * @}
 */

#endif /* __AUDIO_PORT_CONNECTION_H__ */
