/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * MIDI mapping schema.
 */

#ifndef __SCHEMAS_AUDIO_MIDI_MAPPING_H__
#define __SCHEMAS_AUDIO_MIDI_MAPPING_H__

#include "utils/midi.h"

#include "schemas/audio/ext_port.h"
#include "schemas/audio/port_identifier.h"

typedef struct MidiMapping_v1
{
  int               schema_version;
  midi_byte_t       key[3];
  ExtPort_v1 *      device_port;
  PortIdentifier_v1 dest_id;
  void *            dest;
  volatile int      enabled;
} MidiMapping_v1;

typedef struct MidiMappings_v1
{
  MidiMapping_v1 mappings[2046];
  int            num_mappings;
} MidiMappings_v1;

static const cyaml_schema_field_t midi_mapping_fields_schema_v1[] = {
  YAML_FIELD_INT (MidiMapping_v1, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    MidiMapping_v1,
    key,
    uint8_t_schema,
    3),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    MidiMapping_v1,
    device_port,
    ext_port_fields_schema_v1),
  YAML_FIELD_MAPPING_EMBEDDED (
    MidiMapping_v1,
    dest_id,
    port_identifier_fields_schema_v1),
  YAML_FIELD_INT (MidiMapping_v1, enabled),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t midi_mapping_schema_v1 = {
  YAML_VALUE_DEFAULT (
    MidiMapping_v1,
    midi_mapping_fields_schema_v1),
};

static const cyaml_schema_field_t
  midi_mappings_fields_schema_v1[] = {
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
      MidiMappings_v1,
      mappings,
      midi_mapping_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t midi_mappings_schema_v1 = {
  YAML_VALUE_PTR (
    MidiMappings_v1,
    midi_mappings_fields_schema_v1),
};

#endif
