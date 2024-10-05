// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Fader schema.
 */

#ifndef __SCHEMAS_AUDIO_FADER_H__
#define __SCHEMAS_AUDIO_FADER_H__

#include "common/utils/types.h"
#include "common/utils/yaml.h"
#include "gui/cpp/backend/cyaml_schemas/dsp/port.h"

typedef enum FaderType_v1
{
  FADER_TYPE_NONE_v1,
  FADER_TYPE_MONITOR_v1,
  FADER_TYPE_SAMPLE_PROCESSOR_v1,
  FADER_TYPE_AUDIO_CHANNEL_v1,
  FADER_TYPE_MIDI_CHANNEL_v1,
  FADER_TYPE_GENERIC_v1,
} FaderType_v1;

static const cyaml_strval_t fader_type_strings_v1[] = {
  { "none",             FADER_TYPE_NONE_v1             },
  { "monitor",          FADER_TYPE_MONITOR_v1          },
  { "sample processor", FADER_TYPE_SAMPLE_PROCESSOR_v1 },
  { "audio channel",    FADER_TYPE_AUDIO_CHANNEL_v1    },
  { "midi channel",     FADER_TYPE_MIDI_CHANNEL_v1     },
  { "generic",          FADER_TYPE_GENERIC_v1          },
};

typedef enum MidiFaderMode_v1
{
  MIDI_FADER_MODE_VEL_MULTIPLIER_v1,
  MIDI_FADER_MODE_CC_VOLUME_v1,
} MidiFaderMode_v1;

static const cyaml_strval_t midi_fader_mode_strings_v1[] = {
  { "vel_multiplier", MIDI_FADER_MODE_VEL_MULTIPLIER_v1 },
  { "cc_volume",      MIDI_FADER_MODE_CC_VOLUME_v1      },
};

typedef struct Fader_v1
{
  int              schema_version;
  float            volume;
  float            phase;
  Port_v1 *        amp;
  Port_v1 *        balance;
  Port_v1 *        mute;
  Port_v1 *        solo;
  Port_v1 *        listen;
  Port_v1 *        mono_compat_enabled;
  StereoPorts_v1 * stereo_in;
  StereoPorts_v1 * stereo_out;
  Port_v1 *        midi_in;
  Port_v1 *        midi_out;
  FaderType_v1     type;
  MidiFaderMode_v1 midi_mode;
  bool             passthrough;
} Fader_v1;

static const cyaml_schema_field_t fader_fields_schema_v1[] = {
  YAML_FIELD_INT (Fader_v1, schema_version),
  YAML_FIELD_ENUM (Fader_v1, type, fader_type_strings_v1),
  YAML_FIELD_FLOAT (Fader_v1, volume),
  YAML_FIELD_MAPPING_PTR (Fader_v1, amp, port_fields_schema_v1),
  YAML_FIELD_FLOAT (Fader_v1, phase),
  YAML_FIELD_MAPPING_PTR (Fader_v1, balance, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v1, mute, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v1, solo, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v1, listen, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v1, mono_compat_enabled, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Fader_v1, midi_in, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Fader_v1, midi_out, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v1,
    stereo_in,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v1,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_ENUM (Fader_v1, midi_mode, midi_fader_mode_strings_v1),
  YAML_FIELD_INT (Fader_v1, passthrough),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t fader_schema_v1 = {
  YAML_VALUE_PTR (Fader_v1, fader_fields_schema_v1),
};

typedef struct Fader_v2
{
  int              schema_version;
  float            volume;
  float            phase;
  Port_v1 *        amp;
  Port_v1 *        balance;
  Port_v1 *        mute;
  Port_v1 *        solo;
  Port_v1 *        listen;
  Port_v1 *        mono_compat_enabled;
  Port_v1 *        swap_phase;
  StereoPorts_v1 * stereo_in;
  StereoPorts_v1 * stereo_out;
  Port_v1 *        midi_in;
  Port_v1 *        midi_out;
  FaderType_v1     type;
  MidiFaderMode_v1 midi_mode;
  bool             passthrough;
} Fader_v2;

static const cyaml_schema_field_t fader_fields_schema_v2[] = {
  YAML_FIELD_INT (Fader_v2, schema_version),
  YAML_FIELD_ENUM (Fader_v2, type, fader_type_strings_v1),
  YAML_FIELD_FLOAT (Fader_v2, volume),
  YAML_FIELD_MAPPING_PTR (Fader_v2, amp, port_fields_schema_v1),
  YAML_FIELD_FLOAT (Fader_v2, phase),
  YAML_FIELD_MAPPING_PTR (Fader_v2, balance, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v2, mute, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v2, solo, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v2, listen, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v2, mono_compat_enabled, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Fader_v2, swap_phase, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Fader_v2, midi_in, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Fader_v2, midi_out, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v2,
    stereo_in,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Fader_v2,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_ENUM (Fader_v2, midi_mode, midi_fader_mode_strings_v1),
  YAML_FIELD_INT (Fader_v2, passthrough),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t fader_schema_v2 = {
  YAML_VALUE_PTR (Fader_v2, fader_fields_schema_v2),
};

Fader_v2 *
fader_upgrade_from_v1 (Fader_v1 * old);

#endif
