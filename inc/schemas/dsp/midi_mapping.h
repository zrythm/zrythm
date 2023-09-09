// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * MIDI mapping schema.
 */

#ifndef __SCHEMAS_AUDIO_MIDI_MAPPING_H__
#define __SCHEMAS_AUDIO_MIDI_MAPPING_H__

#include "utils/midi.h"

#include "schemas/dsp/ext_port.h"
#include "schemas/dsp/port_identifier.h"

typedef struct MidiMapping_v1
{
  int               schema_version;
  midi_byte_t       key[3];
  ExtPort_v1 *      device_port;
  PortIdentifier_v1 dest_id;
  volatile int      enabled;
} MidiMapping_v1;

typedef struct MidiMappings_v1
{
  MidiMapping_v1 mappings[2046];
  int            num_mappings;
} MidiMappings_v1;

static const cyaml_schema_field_t midi_mapping_fields_schema_v1[] = {
  YAML_FIELD_INT (MidiMapping_v1, schema_version),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (MidiMapping_v1, key, uint8_t_schema, 3),
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
  YAML_VALUE_DEFAULT (MidiMapping_v1, midi_mapping_fields_schema_v1),
};

static const cyaml_schema_field_t midi_mappings_fields_schema_v1[] = {
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    MidiMappings_v1,
    mappings,
    midi_mapping_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t midi_mappings_schema_v1 = {
  YAML_VALUE_PTR (MidiMappings_v1, midi_mappings_fields_schema_v1),
};

#endif
