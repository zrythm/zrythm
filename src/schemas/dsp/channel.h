// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Channel schema.
 */

#ifndef __SCHEMAS_AUDIO_CHANNEL_H__
#define __SCHEMAS_AUDIO_CHANNEL_H__

#include "schemas/dsp/channel_send.h"
#include "schemas/dsp/ext_port.h"
#include "schemas/dsp/fader.h"
#include "schemas/plugins/plugin.h"
#include "utils/yaml.h"

typedef struct Channel_v1
{
  int              schema_version;
  Plugin_v1 *      midi_fx[9];
  Plugin_v1 *      inserts[9];
  Plugin_v1 *      instrument;
  ChannelSend_v1 * sends[9];
  ExtPort_v1 *     ext_midi_ins[1024];
  int              num_ext_midi_ins;
  int              all_midi_ins;
  ExtPort_v1 *     ext_stereo_l_ins[1024];
  int              num_ext_stereo_l_ins;
  int              all_stereo_l_ins;
  ExtPort_v1 *     ext_stereo_r_ins[1024];
  int              num_ext_stereo_r_ins;
  int              all_stereo_r_ins;
  int              midi_channels[16];
  int              all_midi_channels;
  Fader_v1 *       fader;
  Fader_v1 *       prefader;
  Port_v1 *        midi_out;
  StereoPorts_v1 * stereo_out;
  int              has_output;
  unsigned int     output_name_hash;
  int              track_pos;
  int              width;
} Channel_v1;

static const cyaml_schema_field_t channel_fields_schema_v1[] = {
  YAML_FIELD_INT (Channel_v1, schema_version),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v1, midi_fx, plugin_schema_v1, 9),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v1, inserts, plugin_schema_v1, 9),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v1, sends, channel_send_schema_v1, 9),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Channel_v1, instrument, plugin_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Channel_v1, prefader, fader_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Channel_v1, fader, fader_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Channel_v1, midi_out, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel_v1,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_UINT (Channel_v1, has_output),
  YAML_FIELD_UINT (Channel_v1, output_name_hash),
  YAML_FIELD_INT (Channel_v1, track_pos),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel_v1,
    ext_midi_ins,
    ext_port_schema_v1),
  YAML_FIELD_INT (Channel_v1, all_midi_ins),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v1, midi_channels, int_schema, 16),
  YAML_FIELD_INT (Channel_v1, all_midi_channels),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel_v1,
    ext_stereo_l_ins,
    ext_port_schema_v1),
  YAML_FIELD_INT (Channel_v1, all_stereo_l_ins),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel_v1,
    ext_stereo_r_ins,
    ext_port_schema_v1),
  YAML_FIELD_INT (Channel_v1, all_stereo_r_ins),
  YAML_FIELD_INT (Channel_v1, width),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t channel_schema_v1 = {
  YAML_VALUE_PTR (Channel_v1, channel_fields_schema_v1),
};

typedef struct Channel_v2
{
  int              schema_version;
  Plugin_v1 *      midi_fx[9];
  Plugin_v1 *      inserts[9];
  Plugin_v1 *      instrument;
  ChannelSend_v1 * sends[9];
  ExtPort_v1 *     ext_midi_ins[1024];
  int              num_ext_midi_ins;
  int              all_midi_ins;
  ExtPort_v1 *     ext_stereo_l_ins[1024];
  int              num_ext_stereo_l_ins;
  int              all_stereo_l_ins;
  ExtPort_v1 *     ext_stereo_r_ins[1024];
  int              num_ext_stereo_r_ins;
  int              all_stereo_r_ins;
  int              midi_channels[16];
  int              all_midi_channels;
  Fader_v2 *       fader;
  Fader_v2 *       prefader;
  Port_v1 *        midi_out;
  StereoPorts_v1 * stereo_out;
  int              has_output;
  unsigned int     output_name_hash;
  int              track_pos;
  int              width;
} Channel_v2;

static const cyaml_schema_field_t channel_fields_schema_v2[] = {
  YAML_FIELD_INT (Channel_v2, schema_version),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v2, midi_fx, plugin_schema_v1, 9),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v2, inserts, plugin_schema_v1, 9),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v2, sends, channel_send_schema_v1, 9),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Channel_v2, instrument, plugin_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (Channel_v2, prefader, fader_fields_schema_v2),
  YAML_FIELD_MAPPING_PTR (Channel_v2, fader, fader_fields_schema_v2),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (Channel_v2, midi_out, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel_v2,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_UINT (Channel_v2, has_output),
  YAML_FIELD_UINT (Channel_v2, output_name_hash),
  YAML_FIELD_INT (Channel_v2, track_pos),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel_v2,
    ext_midi_ins,
    ext_port_schema_v1),
  YAML_FIELD_INT (Channel_v2, all_midi_ins),
  YAML_FIELD_SEQUENCE_FIXED (Channel_v2, midi_channels, int_schema, 16),
  YAML_FIELD_INT (Channel_v2, all_midi_channels),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel_v2,
    ext_stereo_l_ins,
    ext_port_schema_v1),
  YAML_FIELD_INT (Channel_v2, all_stereo_l_ins),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel_v2,
    ext_stereo_r_ins,
    ext_port_schema_v1),
  YAML_FIELD_INT (Channel_v2, all_stereo_r_ins),
  YAML_FIELD_INT (Channel_v2, width),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t channel_schema_v2 = {
  YAML_VALUE_PTR (Channel_v2, channel_fields_schema_v2),
};

Channel_v2 *
channel_upgrade_from_v1 (Channel_v1 * old);

#endif
