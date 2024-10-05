// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Track processor schema.
 */

#ifndef __SCHEMAS_AUDIO_TRACK_PROCESSOR_H__
#define __SCHEMAS_AUDIO_TRACK_PROCESSOR_H__

#include "common/utils/types.h"
#include "common/utils/yaml.h"
#include "gui/cpp/backend/cyaml_schemas/dsp/port.h"

typedef enum TrackProcessorMidiAutomatable_v1
{
  MIDI_AUTOMATABLE_MOD_WHEEL_v1,
  MIDI_AUTOMATABLE_PITCH_BEND_v1,
  NUM_MIDI_AUTOMATABLES_v1,
} TrackProcessorMidiAutomatable_v1;

typedef struct TrackProcessor_v1
{
  int              schema_version;
  Port_v1 *        mono;
  Port_v1 *        input_gain;
  Port_v1 *        output_gain;
  Port_v1 *        midi_in;
  Port_v1 *        midi_out;
  Port_v1 *        piano_roll;
  Port_v1 *        monitor_audio;
  StereoPorts_v1 * stereo_in;
  StereoPorts_v1 * stereo_out;
  Port_v1 *        midi_cc[128 * 16];
  Port_v1 *        pitch_bend[16];
  Port_v1 *        poly_key_pressure[16];
  Port_v1 *        channel_pressure[16];
} TrackProcessor_v1;

static const cyaml_schema_field_t track_processor_fields_schema_v1[] = {
  YAML_FIELD_INT (TrackProcessor_v1, schema_version),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (TrackProcessor_v1, mono, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    input_gain,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    output_gain,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    midi_in,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    midi_out,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    piano_roll,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    monitor_audio,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    stereo_in,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    TrackProcessor_v1,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    TrackProcessor_v1,
    midi_cc,
    port_schema_v1,
    128 * 16),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    TrackProcessor_v1,
    pitch_bend,
    port_schema_v1,
    16),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    TrackProcessor_v1,
    poly_key_pressure,
    port_schema_v1,
    16),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
    TrackProcessor_v1,
    channel_pressure,
    port_schema_v1,
    16),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t track_processor_schema_v1 = {
  YAML_VALUE_PTR (TrackProcessor_v1, track_processor_fields_schema_v1),
};

#endif
