// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SCHEMAS_AUDIO_PORT_CONNECTION_H__
#define __SCHEMAS_AUDIO_PORT_CONNECTION_H__

#include "utils/yaml.h"

#include "schemas/dsp/port_identifier.h"

typedef struct PortConnection_v1
{
  int                 schema_version;
  PortIdentifier_v1 * src_id;
  PortIdentifier_v1 * dest_id;
  float               multiplier;
  bool                locked;
  bool                enabled;
  float               base_value;
} PortConnection_v1;

static const cyaml_schema_field_t
  port_connection_fields_schema_v1[] = {
    YAML_FIELD_INT (PortConnection_v1, schema_version),
    YAML_FIELD_MAPPING_PTR (
      PortConnection_v1,
      src_id,
      port_identifier_fields_schema_v1),
    YAML_FIELD_MAPPING_PTR (
      PortConnection_v1,
      dest_id,
      port_identifier_fields_schema_v1),
    YAML_FIELD_FLOAT (PortConnection_v1, multiplier),
    YAML_FIELD_INT (PortConnection_v1, locked),
    YAML_FIELD_INT (PortConnection_v1, enabled),
    YAML_FIELD_FLOAT (PortConnection_v1, base_value),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t port_connection_schema_v1 = {
  YAML_VALUE_PTR (
    PortConnection_v1,
    port_connection_fields_schema_v1),
};

#endif
