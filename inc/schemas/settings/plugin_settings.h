// SPDX-FileCopyrightText: © 2021-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin settings schemas.
 */

#ifndef __SCHEMAS_SETTINGS_PLUGIN_SETTINGS_H__
#define __SCHEMAS_SETTINGS_PLUGIN_SETTINGS_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "utils/yaml.h"

#include "schemas/plugins/plugin_descriptor.h"

typedef struct PluginSetting_v1
{
  int                   schema_version;
  PluginDescriptor_v1 * descr;
  bool                  open_with_carla;
  bool                  force_generic_ui;
  CarlaBridgeMode_v1    bridge_mode;
  char *                ui_uri;
  gint64                last_instantiated_time;
  int                   num_instantiations;
} PluginSetting_v1;

static const cyaml_schema_field_t plugin_setting_fields_schema_v1[] = {
  YAML_FIELD_INT (PluginSetting_v1, schema_version),
  YAML_FIELD_MAPPING_PTR (
    PluginSetting_v1,
    descr,
    plugin_descriptor_fields_schema_v1),
  YAML_FIELD_INT (PluginSetting_v1, open_with_carla),
  YAML_FIELD_INT (PluginSetting_v1, force_generic_ui),
  YAML_FIELD_ENUM (PluginSetting_v1, bridge_mode, carla_bridge_mode_strings_v1),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginSetting_v1, ui_uri),
  YAML_FIELD_INT_OPT (PluginSetting, last_instantiated_time),
  YAML_FIELD_INT_OPT (PluginSetting, num_instantiations),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_setting_schema_v1 = {
  YAML_VALUE_PTR (PluginSetting_v1, plugin_setting_fields_schema_v1),
};

PluginSetting *
plugin_setting_upgrade_from_v1 (PluginSetting_v1 * old);

#if 0
typedef struct PluginSettings_v3
{
  int                schema_version;
  PluginSetting_v1 * settings[90000];
  int                num_settings;
} PluginSettings_v3;

static const cyaml_schema_field_t plugin_settings_fields_schema_v3[] = {
  YAML_FIELD_INT (PluginSettings_v3, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    PluginSettings_v3,
    settings,
    plugin_setting_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_settings_schema_v3 = {
  YAML_VALUE_PTR (PluginSettings_v3, plugin_settings_fields_schema_v3),
};
#endif

typedef struct PluginSetting_v2
{
  int                   schema_version;
  PluginDescriptor_v2 * descr;
  bool                  open_with_carla;
  bool                  force_generic_ui;
  CarlaBridgeMode    bridge_mode;
  char *                ui_uri;
  gint64                last_instantiated_time;
  int                   num_instantiations;
} PluginSetting_v2;

static const cyaml_schema_field_t plugin_setting_fields_schema_v2[] = {
  YAML_FIELD_INT (PluginSetting_v2, schema_version),
  YAML_FIELD_MAPPING_PTR (
    PluginSetting_v2,
    descr,
    plugin_descriptor_fields_schema_v2),
  YAML_FIELD_INT (PluginSetting_v2, open_with_carla),
  YAML_FIELD_INT (PluginSetting_v2, force_generic_ui),
  YAML_FIELD_ENUM (PluginSetting_v2, bridge_mode, carla_bridge_mode_strings),
  YAML_FIELD_STRING_PTR_OPTIONAL (PluginSetting_v2, ui_uri),
  YAML_FIELD_INT_OPT (PluginSetting, last_instantiated_time),
  YAML_FIELD_INT_OPT (PluginSetting, num_instantiations),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_setting_schema_v2 = {
  YAML_VALUE_PTR (PluginSetting_v2, plugin_setting_fields_schema_v2),
};

PluginSetting *
plugin_setting_upgrade_from_v2 (PluginSetting_v2 * old);

#endif
