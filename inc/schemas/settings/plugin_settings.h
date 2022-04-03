/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Plugin settings schemas.
 */

#ifndef __SCHEMAS_SETTINGS_PLUGIN_SETTINGS_H__
#define __SCHEMAS_SETTINGS_PLUGIN_SETTINGS_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "utils/yaml.h"

#include "schemas/plugins/plugin_descriptor.h"

typedef enum CarlaBridgeMode_v1
{
  CARLA_BRIDGE_NONE_v1,
  CARLA_BRIDGE_UI_v1,
  CARLA_BRIDGE_FULL_v1,
} CarlaBridgeMode_v1;

static const cyaml_strval_t
  carla_bridge_mode_strings_v1[] = {
    {"None",  CARLA_BRIDGE_NONE_v1},
    { "UI",   CARLA_BRIDGE_UI_v1  },
    { "Full", CARLA_BRIDGE_FULL_v1},
};

typedef struct PluginSetting_v1
{
  int                   schema_version;
  PluginDescriptor_v1 * descr;
  bool                  open_with_carla;
  bool                  force_generic_ui;
  CarlaBridgeMode_v1    bridge_mode;
  char *                ui_uri;
} PluginSetting_v1;

typedef struct PluginSettings_v3
{
  int                schema_version;
  PluginSetting_v1 * settings[90000];
  int                num_settings;
} PluginSettings_v3;

static const cyaml_schema_field_t
  plugin_setting_fields_schema_v1[] = {
    YAML_FIELD_INT (PluginSetting_v1, schema_version),
    YAML_FIELD_MAPPING_PTR (
      PluginSetting_v1,
      descr,
      plugin_descriptor_fields_schema_v1),
    YAML_FIELD_INT (PluginSetting_v1, open_with_carla),
    YAML_FIELD_INT (PluginSetting_v1, force_generic_ui),
    YAML_FIELD_ENUM (
      PluginSetting_v1,
      bridge_mode,
      carla_bridge_mode_strings_v1),
    YAML_FIELD_STRING_PTR_OPTIONAL (
      PluginSetting_v1,
      ui_uri),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  plugin_setting_schema_v1 = {
    YAML_VALUE_PTR (
      PluginSetting_v1,
      plugin_setting_fields_schema_v1),
  };

static const cyaml_schema_field_t
  plugin_settings_fields_schema_v3[] = {
    YAML_FIELD_INT (PluginSettings_v3, schema_version),
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
      PluginSettings_v3,
      settings,
      plugin_setting_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  plugin_settings_schema_v3 = {
    YAML_VALUE_PTR (
      PluginSettings_v3,
      plugin_settings_fields_schema_v3),
  };

#endif
