// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin schema.
 */

#ifndef __SCHEMAS_PLUGINS_PLUGIN_H__
#define __SCHEMAS_PLUGINS_PLUGIN_H__

#include "zrythm-config.h"

#include "utils/types.h"

#include "schemas/audio/port.h"
#include "schemas/plugins/plugin_descriptor.h"
#include "schemas/plugins/plugin_identifier.h"
#include "schemas/plugins/plugin_preset.h"
#include "schemas/settings/plugin_settings.h"

typedef struct Plugin_v1
{
  int                       schema_version;
  PluginIdentifier_v1       id;
  PluginSetting_v1 *        setting;
  Port_v1 **                in_ports;
  int                       num_in_ports;
  Port_v1 **                out_ports;
  int                       num_out_ports;
  PluginBank_v1 **          banks;
  int                       num_banks;
  PluginPresetIdentifier_v1 selected_bank;
  PluginPresetIdentifier_v1 selected_preset;
  bool                      visible;
  char *                    state_dir;
} Plugin_v1;

static const cyaml_schema_field_t plugin_fields_schema_v1[] = {
  YAML_FIELD_INT (Plugin_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin_v1,
    id,
    plugin_identifier_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Plugin_v1,
    setting,
    plugin_setting_fields_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Plugin_v1,
    in_ports,
    port_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT (
    Plugin_v1,
    out_ports,
    port_schema_v1),
  YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
    Plugin_v1,
    banks,
    plugin_bank_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin_v1,
    selected_bank,
    plugin_preset_identifier_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    Plugin_v1,
    selected_preset,
    plugin_preset_identifier_fields_schema_v1),
  YAML_FIELD_INT (Plugin_v1, visible),
  YAML_FIELD_STRING_PTR_OPTIONAL (Plugin_v1, state_dir),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER_NULL_STR,
    Plugin_v1,
    plugin_fields_schema_v1),
};

Plugin *
plugin_upgrade_from_v1 (Plugin_v1 * old);

#endif
