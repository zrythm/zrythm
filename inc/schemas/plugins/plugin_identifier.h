// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Plugin identifier.
 */

#ifndef __SCHEMA_PLUGINS_PLUGIN_IDENTIFIER_H__
#define __SCHEMA_PLUGINS_PLUGIN_IDENTIFIER_H__

#include "utils/yaml.h"

// typedef struct PluginIdentifier PluginIdentifier;

typedef enum ZPluginSlotType_v1
{
  Z_PLUGIN_SLOT_INVALID_v1,
  Z_PLUGIN_SLOT_INSERT_v1,
  Z_PLUGIN_SLOT_MIDI_FX_v1,
  Z_PLUGIN_SLOT_INSTRUMENT_v1,
  Z_PLUGIN_SLOT_MODULATOR_v1,
} ZPluginSlotType_v1;

static const cyaml_strval_t plugin_slot_type_strings_v1[] = {
  {"Invalid",     Z_PLUGIN_SLOT_INVALID_v1   },
  { "Insert",     Z_PLUGIN_SLOT_INSERT_v1    },
  { "MIDI FX",    Z_PLUGIN_SLOT_MIDI_FX_v1   },
  { "Instrument", Z_PLUGIN_SLOT_INSTRUMENT_v1},
  { "Modulator",  Z_PLUGIN_SLOT_MODULATOR_v1 },
};

typedef struct PluginIdentifier_v1
{
  int                schema_version;
  ZPluginSlotType_v1 slot_type;
  unsigned int       track_name_hash;
  int                slot;
} PluginIdentifier_v1;

static const cyaml_schema_field_t plugin_identifier_fields_schema_v1[] = {
  YAML_FIELD_INT (PluginIdentifier_v1, schema_version),
  YAML_FIELD_ENUM (PluginIdentifier_v1, slot_type, plugin_slot_type_strings_v1),
  YAML_FIELD_UINT (PluginIdentifier_v1, track_name_hash),
  YAML_FIELD_INT (PluginIdentifier_v1, slot),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t plugin_identifier_schema_v1 = {
  YAML_VALUE_PTR (PluginIdentifier_v1, plugin_identifier_fields_schema_v1),
};

#endif
