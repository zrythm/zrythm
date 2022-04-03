/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Plugin schema.
 */

#ifndef __SCHEMAS_PLUGINS_PLUGIN_H__
#define __SCHEMAS_PLUGINS_PLUGIN_H__

#include "zrythm-config.h"

#include "audio/port.h"
#include "plugins/lv2_plugin.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_identifier.h"
#include "plugins/plugin_preset.h"
#include "settings/plugin_settings.h"
#include "utils/types.h"

typedef struct Plugin_v1
{
  int                       schema_version;
  PluginIdentifier_v1       id;
  void *                    lv2;
  void *                    carla;
  PluginSetting_v1 *        setting;
  Port_v1 **                in_ports;
  int                       num_in_ports;
  size_t                    in_ports_size;
  Port_v1 **                out_ports;
  int                       num_out_ports;
  size_t                    out_ports_size;
  void **                   lilv_ports;
  int                       num_lilv_ports;
  void *                    enabled;
  void *                    own_enabled_port;
  void *                    gain;
  PluginBank_v1 **          banks;
  int                       num_banks;
  size_t                    banks_size;
  PluginPresetIdentifier_v1 selected_bank;
  PluginPresetIdentifier_v1 selected_preset;
  bool                      visible;
  nframes_t                 latency;
  bool                      instantiated;
  bool                      instantiation_failed;
  bool                      activated;
  int                       ui_instantiated;
  float                     ui_update_hz;
  float                     ui_scale_factor;
  char *                    state_dir;
  bool                      deleting;
  void *                    active_preset_item;
  void *                    window;
  bool                      external_ui_visible;
  void *                    ev_box;
  void *                    vbox;
  gulong                    destroy_window_id;
  gulong                    delete_event_id;
  int                       magic;
  bool                      is_project;
  void *                    modulator_widget;
  guint                     update_ui_source_id;
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
  YAML_FIELD_STRING_PTR_OPTIONAL (
    Plugin_v1,
    state_dir),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER_NULL_STR,
    Plugin_v1,
    plugin_fields_schema_v1),
};

#endif
