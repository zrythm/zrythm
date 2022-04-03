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
 * Track processor schema.
 */

#ifndef __SCHEMAS_AUDIO_TRACK_PROCESSOR_H__
#define __SCHEMAS_AUDIO_TRACK_PROCESSOR_H__

#include "utils/types.h"
#include "utils/yaml.h"

#include "schemas/audio/port.h"

typedef enum TrackProcessorMidiAutomatable_v1
{
  MIDI_AUTOMATABLE_MOD_WHEEL_v1,
  MIDI_AUTOMATABLE_PITCH_BEND_v1,
  NUM_MIDI_AUTOMATABLES_v1,
} TrackProcessorMidiAutomatable_v1;

typedef struct TrackProcessor_v1
{
  int              schema_version;
  StereoPorts_v1 * stereo_in;
  Port_v1 *        mono;
  Port_v1 *        input_gain;
  StereoPorts_v1 * stereo_out;
  Port_v1 *        midi_in;
  Port_v1 *        midi_out;
  Port_v1 *        piano_roll;
  Port_v1 *
    midi_automatables[NUM_MIDI_AUTOMATABLES * 16];
  float last_automatable_vals
    [NUM_MIDI_AUTOMATABLES * 16];
  float  l_port_db;
  float  r_port_db;
  int    track_pos;
  void * track;
  bool   is_project;
  int    magic;
} TrackProcessor_v1;

static const cyaml_schema_field_t
  track_processor_fields_schema_v1[] = {
    YAML_FIELD_INT (TrackProcessor_v1, schema_version),
    YAML_FIELD_MAPPING_PTR_OPTIONAL (
      TrackProcessor_v1,
      mono,
      port_fields_schema_v1),
    YAML_FIELD_MAPPING_PTR_OPTIONAL (
      TrackProcessor_v1,
      input_gain,
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
      stereo_in,
      stereo_ports_fields_schema_v1),
    YAML_FIELD_MAPPING_PTR_OPTIONAL (
      TrackProcessor_v1,
      stereo_out,
      stereo_ports_fields_schema_v1),
    YAML_FIELD_FIXED_SIZE_PTR_ARRAY (
      TrackProcessor_v1,
      midi_automatables,
      port_schema_v1,
      NUM_MIDI_AUTOMATABLES * 16),
    YAML_FIELD_INT (TrackProcessor_v1, track_pos),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t
  track_processor_schema_v1 = {
    YAML_VALUE_PTR (
      TrackProcessor_v1,
      track_processor_fields_schema_v1),
  };

#endif
