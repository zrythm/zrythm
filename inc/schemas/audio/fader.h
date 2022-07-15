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
 * Fader schema.
 */

#ifndef __SCHEMAS_AUDIO_FADER_H__
#define __SCHEMAS_AUDIO_FADER_H__

#include "utils/types.h"
#include "utils/yaml.h"

#include "schemas/audio/port.h"

typedef enum FaderType_v1
{
  FADER_TYPE_NONE_v1,
  FADER_TYPE_MONITOR_v1,
  FADER_TYPE_AUDIO_CHANNEL_v1,
  FADER_TYPE_MIDI_CHANNEL_v1,
  FADER_TYPE_GENERIC_v1,
} FaderType_v1;

static const cyaml_strval_t fader_type_strings_v1[] = {
  {"none",             FADER_TYPE_NONE_v1         },
  { "monitor channel", FADER_TYPE_MONITOR_v1      },
  { "audio channel",   FADER_TYPE_AUDIO_CHANNEL_v1},
  { "midi channel",    FADER_TYPE_MIDI_CHANNEL_v1 },
  { "generic",         FADER_TYPE_GENERIC_v1      },
};

typedef enum MidiFaderMode_v1
{
  MIDI_FADER_MODE_VEL_MULTIPLIER_v1,
  MIDI_FADER_MODE_CC_VOLUME_v1,
} MidiFaderMode_v1;

static const cyaml_strval_t midi_fader_mode_strings_v1[] = {
  {"vel_multiplier", MIDI_FADER_MODE_VEL_MULTIPLIER_v1},
  { "cc_volume",     MIDI_FADER_MODE_CC_VOLUME_v1     },
};

typedef struct Fader_v1
{
  int              schema_version;
  float            volume;
  float            phase;
  float            fader_val;
  float            last_cc_volume;
  Port_v1 *        amp;
  Port_v1 *        balance;
  Port_v1 *        mute;
  int              solo;
  StereoPorts_v1 * stereo_in;
  StereoPorts_v1 * stereo_out;
  Port_v1 *        midi_in;
  Port_v1 *        midi_out;
  float            l_port_db;
  float            r_port_db;
  FaderType_v1     type;
  MidiFaderMode_v1 midi_mode;
  bool             mono_compat_enabled;
  bool             passthrough;
  int              track_pos;
  int              magic;
  bool             is_project;
} Fader_v1;

static const cyaml_schema_field_t fader_fields_schema_v1[] = {
  YAML_FIELD_INT (Fader_v1, schema_version),
  YAML_FIELD_ENUM (Fader_v1, type, fader_type_strings_v1),
  YAML_FIELD_FLOAT (Fader_v1, volume),
  YAML_FIELD_MAPPING_PTR (Fader_v1, amp, port_fields_schema_v1),
  YAML_FIELD_FLOAT (Fader_v1, phase),
  YAML_FIELD_MAPPING_PTR (
    Fader_v1,
    balance,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v1, mute, port_fields_schema_v1),
  YAML_FIELD_INT (Fader_v1, solo),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v1,
    midi_in,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v1,
    midi_out,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v1,
    stereo_in,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v1,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_ENUM (
    Fader_v1,
    midi_mode,
    midi_fader_mode_strings_v1),
  YAML_FIELD_INT (Fader_v1, track_pos),
  YAML_FIELD_INT (Fader_v1, passthrough),
  YAML_FIELD_INT (Fader_v1, mono_compat_enabled),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t fader_schema_v1 = {
  YAML_VALUE_PTR (Fader_v1, fader_fields_schema_v1),
};

#endif
