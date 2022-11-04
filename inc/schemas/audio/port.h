// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Port schema.
 */

#ifndef __SCHEMAS_AUDIO_PORTS_H__
#define __SCHEMAS_AUDIO_PORTS_H__

#include "zrythm-config.h"

#include "schemas/audio/port_identifier.h"

typedef struct Port_v1
{
  int               schema_version;
  PortIdentifier_v1 id;
  int               exposed_to_backend;
  float             control;
  float             minf;
  float             maxf;
  float             zerof;
  float             deff;
  int               carla_param_id;
} Port_v1;

static const cyaml_schema_field_t port_fields_schema_v1[] = {
  YAML_FIELD_INT (Port_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    Port_v1,
    id,
    port_identifier_fields_schema_v1),
  YAML_FIELD_INT (Port_v1, exposed_to_backend),
  YAML_FIELD_FLOAT (Port_v1, control),
  YAML_FIELD_FLOAT (Port_v1, minf),
  YAML_FIELD_FLOAT (Port_v1, maxf),
  YAML_FIELD_FLOAT (Port_v1, zerof),
  YAML_FIELD_FLOAT (Port_v1, deff),
  YAML_FIELD_INT (Port_v1, carla_param_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t port_schema_v1 = {
  YAML_VALUE_PTR_NULLABLE (Port_v1, port_fields_schema_v1),
};

/**
 * L & R port, for convenience.
 *
 * Must ONLY be created via stereo_ports_new()
 */
typedef struct StereoPorts_v1
{
  int       schema_version;
  Port_v1 * l;
  Port_v1 * r;
} StereoPorts_v1;

static const cyaml_schema_field_t stereo_ports_fields_schema_v1[] = {
  YAML_FIELD_INT (StereoPorts_v1, schema_version),
  YAML_FIELD_MAPPING_PTR (
    StereoPorts_v1,
    l,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    StereoPorts_v1,
    r,
    port_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t stereo_ports_schema_v1 = {
  YAML_VALUE_PTR (StereoPorts_v1, stereo_ports_fields_schema_v1),
};

Port *
port_upgrade_from_v1 (Port_v1 * old);

StereoPorts *
stereo_ports_upgrade_from_v1 (StereoPorts_v1 * old);

#endif
