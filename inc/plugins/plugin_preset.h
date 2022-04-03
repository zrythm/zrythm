/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Plugin preset.
 */

#ifndef __PLUGINS_PLUGIN_PRESET_H__
#define __PLUGINS_PLUGIN_PRESET_H__

#include "plugins/plugin_identifier.h"
#include "utils/yaml.h"

/**
 * @addtogroup plugins
 *
 * @{
 */

#define PLUGIN_BANK_SCHEMA_VERSION 1
#define PLUGIN_PRESET_IDENTIFIER_SCHEMA_VERSION 1
#define PLUGIN_PRESET_SCHEMA_VERSION 1

/**
 * Preset identifier.
 */
typedef struct PluginPresetIdentifier
{
  int schema_version;

  /** Index in bank, or -1 if this is used for
   * a bank. */
  int idx;

  /** Bank index in plugin. */
  int bank_idx;

  /** Plugin identifier. */
  PluginIdentifier plugin_id;
} PluginPresetIdentifier;

static const cyaml_schema_field_t
  plugin_preset_identifier_fields_schema[] = {
    YAML_FIELD_INT (
      PluginPresetIdentifier,
      schema_version),
    YAML_FIELD_INT (PluginPresetIdentifier, idx),
    YAML_FIELD_INT (PluginPresetIdentifier, bank_idx),
    YAML_FIELD_MAPPING_EMBEDDED (
      PluginPresetIdentifier,
      plugin_id,
      plugin_identifier_fields_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  plugin_preset_identifier_schema = {
    YAML_VALUE_PTR (
      PluginPresetIdentifier,
      plugin_preset_identifier_fields_schema),
  };

/**
 * Plugin preset.
 */
typedef struct PluginPreset
{
  int schema_version;

  /** Human readable name. */
  char * name;

  /** URI if LV2. */
  char * uri;

  /** Carla program index. */
  int carla_program;

  PluginPresetIdentifier id;
} PluginPreset;

static const cyaml_schema_field_t
  plugin_preset_fields_schema[] = {
    YAML_FIELD_INT (PluginPreset, schema_version),
    YAML_FIELD_STRING_PTR (PluginPreset, name),
    YAML_FIELD_STRING_PTR_OPTIONAL (PluginPreset, uri),
    YAML_FIELD_INT (PluginPreset, carla_program),
    YAML_FIELD_MAPPING_EMBEDDED (
      PluginPreset,
      id,
      plugin_preset_identifier_fields_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  plugin_preset_schema = {
    YAML_VALUE_PTR (
      PluginPreset,
      plugin_preset_fields_schema),
  };

/**
 * A plugin bank containing presets.
 *
 * If the plugin has no banks, there must be a
 * default bank that will contain all the presets.
 */
typedef struct PluginBank
{
  int schema_version;

  /** Presets in this bank. */
  PluginPreset ** presets;
  int             num_presets;
  size_t          presets_size;

  /** URI if LV2. */
  char * uri;

  /** Human readable name. */
  char * name;

  PluginPresetIdentifier id;
} PluginBank;

static const cyaml_schema_field_t
  plugin_bank_fields_schema[] = {
    YAML_FIELD_INT (PluginBank, schema_version),
    YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
      PluginBank,
      presets,
      plugin_preset_schema),
    YAML_FIELD_STRING_PTR (PluginBank, name),
    YAML_FIELD_STRING_PTR_OPTIONAL (PluginBank, uri),
    YAML_FIELD_MAPPING_EMBEDDED (
      PluginBank,
      id,
      plugin_preset_identifier_fields_schema),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t plugin_bank_schema = {
  YAML_VALUE_PTR (
    PluginBank,
    plugin_bank_fields_schema),
};

PluginBank *
plugin_bank_new (void);

PluginPreset *
plugin_preset_new (void);

NONNULL
void
plugin_preset_identifier_init (
  PluginPresetIdentifier * id);

/**
 * @}
 */

#endif
