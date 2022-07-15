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
 * Plugin identifier.
 */

#ifndef __SCHEMA_PLUGINS_PLUGIN_IDENTIFIER_H__
#define __SCHEMA_PLUGINS_PLUGIN_IDENTIFIER_H__

#include "utils/yaml.h"

typedef enum PluginSlotType_v1
{
  PLUGIN_SLOT_INVALID_v1,
  PLUGIN_SLOT_INSERT_v1,
  PLUGIN_SLOT_MIDI_FX_v1,
  PLUGIN_SLOT_INSTRUMENT_v1,
  PLUGIN_SLOT_MODULATOR_v1,
} PluginSlotType_v1;

static const cyaml_strval_t plugin_slot_type_strings_v1[] = {
  {"Invalid",     PLUGIN_SLOT_INVALID_v1   },
  { "Insert",     PLUGIN_SLOT_INSERT_v1    },
  { "MIDI FX",    PLUGIN_SLOT_MIDI_FX_v1   },
  { "Instrument", PLUGIN_SLOT_INSTRUMENT_v1},
  { "Modulator",  PLUGIN_SLOT_MODULATOR_v1 },
};

typedef struct PluginIdentifier_v1
{
  int               schema_version;
  PluginSlotType_v1 slot_type;
  int               track_pos;
  int               slot;
} PluginIdentifier_v1;

static const cyaml_schema_field_t
  plugin_identifier_fields_schema_v1[] = {
    YAML_FIELD_INT (PluginIdentifier_v1, schema_version),
    YAML_FIELD_ENUM (
      PluginIdentifier_v1,
      slot_type,
      plugin_slot_type_strings) _v1,
    YAML_FIELD_INT (PluginIdentifier_v1, track_pos),
    YAML_FIELD_INT (PluginIdentifier_v1, slot),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t plugin_identifier_schema_v1 = {
  YAML_VALUE_PTR (
    PluginIdentifier_v1,
    plugin_identifier_fields_schema_v1),
};

#endif
