// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SCHEMAS_AUDIO_PORT_CONNECTIONS_MANAGER_H__
#define __SCHEMAS_AUDIO_PORT_CONNECTIONS_MANAGER_H__

#include "common/utils/yaml.h"
#include "gui/cpp/backend/cyaml_schemas/dsp/port_connection.h"

typedef struct PortConnectionsManager_v1
{
  int                  schema_version;
  PortConnection_v1 ** connections;
  int                  num_connections;
  size_t               connections_size;
} PortConnectionsManager_v1;

static const cyaml_schema_field_t port_connections_manager_fields_schema_v1[] = {
  YAML_FIELD_INT (PortConnectionsManager_v1, schema_version),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    PortConnectionsManager_v1,
    connections,
    port_connection_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t port_connections_manager_schema_v1 = {
  YAML_VALUE_PTR (
    PortConnectionsManager_v1,
    port_connections_manager_fields_schema_v1),
};
#endif
