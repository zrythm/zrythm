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
 * Plugin settings.
 */

#ifndef __SETTINGS_PLUGIN_SETTINGS_H__
#define __SETTINGS_PLUGIN_SETTINGS_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "plugins/plugin_descriptor.h"

#include "utils/yaml.h"

/**
 * @addtogroup settings
 *
 * @{
 */

#define PLUGIN_SETTING_SCHEMA_VERSION 1
#define PLUGIN_SETTINGS_SCHEMA_VERSION 4

/**
 * A setting for a specific plugin descriptor.
 */
typedef struct PluginSetting
{
  int                schema_version;

  /** The descriptor of the plugin this setting is
   * for. */
  PluginDescriptor * descr;

  /** Whether to instantiate this plugin with carla. */
  bool               open_with_carla;

  /** Whether to force a generic UI. */
  bool               force_generic_ui;

  /** Requested carla bridge mode. */
  CarlaBridgeMode    bridge_mode;

  /** Requested UI URI (if LV2 and non-bridged and
   * not forcing a generic UI and have a custom
   * UI */
  char *             ui_uri;
} PluginSetting;

typedef struct PluginSettings
{
  int             schema_version;

  /** Settings. */
  PluginSetting * settings[90000];
  int             num_settings;
} PluginSettings;

static const cyaml_schema_field_t
plugin_setting_fields_schema[] =
{
  YAML_FIELD_INT (PluginSetting, schema_version),
  YAML_FIELD_MAPPING_PTR (
    PluginSetting, descr,
    plugin_descriptor_fields_schema),
  YAML_FIELD_INT (PluginSetting, open_with_carla),
  YAML_FIELD_INT (PluginSetting, force_generic_ui),
  YAML_FIELD_ENUM (
    PluginSetting, bridge_mode,
    carla_bridge_mode_strings),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PluginSetting, ui_uri),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
plugin_setting_schema =
{
  YAML_VALUE_PTR (
    PluginSetting,
    plugin_setting_fields_schema),
};

static const cyaml_schema_field_t
plugin_settings_fields_schema[] =
{
  YAML_FIELD_INT (
    PluginSettings, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    PluginSettings, settings,
    plugin_setting_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
plugin_settings_schema =
{
  YAML_VALUE_PTR (
    PluginSettings,
    plugin_settings_fields_schema),
};

/**
 * Creates a plugin setting with the recommended
 * settings for the given plugin descriptor based
 * on the current setup.
 */
PluginSetting *
NONNULL
plugin_setting_new_default (
  const PluginDescriptor * descr);

PluginSetting *
NONNULL
plugin_setting_clone (
  PluginSetting * src,
  bool            validate);

/**
 * Makes sure the setting is valid in the current
 * run and changes any fields to make it conform.
 *
 * For example, if the setting is set to not open
 * with carla but the descriptor is for a VST plugin,
 * this will set \ref PluginSetting.open_with_carla
 * to true.
 */
NONNULL
void
plugin_setting_validate (
  PluginSetting * self);

void
NONNULL
plugin_setting_print (
  const PluginSetting * self);

/**
 * Frees the plugin setting.
 */
NONNULL
void
plugin_setting_free (
  PluginSetting * self);

/**
 * Reads the file and fills up the object.
 */
PluginSettings *
plugin_settings_new (void);

/**
 * Serializes the current settings.
 */
NONNULL
void
plugin_settings_serialize_to_file (
  PluginSettings * self);

/**
 * Finds a setting for the given plugin descriptor.
 *
 * @return The found setting or NULL.
 */
NONNULL
PluginSetting *
plugin_settings_find (
  PluginSettings *         self,
  const PluginDescriptor * descr);

/**
 * Replaces a setting or appends a setting to the
 * cache.
 *
 * This clones the setting before adding it.
 *
 * @param serialize Whether to serialize the updated
 *   cache now.
 */
NONNULL
void
plugin_settings_set (
  PluginSettings * self,
  PluginSetting *  setting,
  bool             _serialize);

NONNULL
void
plugin_settings_free (
  PluginSettings * self);

/**
 * @}
 */

#endif
