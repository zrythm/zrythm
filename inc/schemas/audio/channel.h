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
 * Channel schema.
 */

#ifndef __SCHEMAS_AUDIO_CHANNEL_H__
#define __SCHEMAS_AUDIO_CHANNEL_H__

#include "utils/yaml.h"

#include "schemas/audio/channel_send.h"
#include "schemas/audio/ext_port.h"
#include "schemas/audio/fader.h"
#include "schemas/plugins/plugin.h"

typedef struct Channel_v1
{
  int              schema_version;
  Plugin_v1 *      midi_fx[STRIP_SIZE];
  Plugin_v1 *      inserts[STRIP_SIZE];
  Plugin_v1 *      instrument;
  ChannelSend_v1   sends[STRIP_SIZE];
  ExtPort_v1 *     ext_midi_ins[EXT_PORTS_MAX];
  int              num_ext_midi_ins;
  int              all_midi_ins;
  ExtPort_v1 *     ext_stereo_l_ins[EXT_PORTS_MAX];
  int              num_ext_stereo_l_ins;
  int              all_stereo_l_ins;
  ExtPort_v1 *     ext_stereo_r_ins[EXT_PORTS_MAX];
  int              num_ext_stereo_r_ins;
  int              all_stereo_r_ins;
  int              midi_channels[16];
  int              all_midi_channels;
  Fader_v1 *       fader;
  Fader_v1 *       prefader;
  Port_v1 *        midi_out;
  StereoPorts_v1 * stereo_out;
  int              has_output;
  int              output_pos;
  int              track_pos;
  int              magic;
  void *           widget;
  int              record_set_automatically;
  void *           track;
} Channel_v1;

static const cyaml_schema_field_t channel_fields_schema_v1[] = {
  YAML_FIELD_INT (Channel_v1, schema_version),
  YAML_FIELD_SEQUENCE_FIXED (
    Channel_v1,
    midi_fx,
    plugin_schema_v1,
    STRIP_SIZE),
  YAML_FIELD_SEQUENCE_FIXED (
    Channel_v1,
    inserts,
    plugin_schema_v1,
    STRIP_SIZE),
  YAML_FIELD_SEQUENCE_FIXED (
    Channel_v1,
    sends,
    channel_send_schema_v1,
    STRIP_SIZE),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel_v1,
    instrument,
    plugin_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Channel_v1,
    prefader,
    fader_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    Channel_v1,
    fader,
    fader_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel_v1,
    midi_out,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    Channel_v1,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_INT (Channel_v1, has_output),
  YAML_FIELD_INT (Channel_v1, output_pos),
  YAML_FIELD_INT (Channel_v1, track_pos),
  YAML_FIELD_FIXED_SIZE_PTR_ARRAY_VAR_COUNT (
    Channel_v1,
    ext_midi_ins,
    ext_port_schema_v1),
  YAML_FIELD_INT (Channel_v1, all_midi_ins),
  YAML_FIELD_SEQUENCE_FIXED (
    Channel_v1,
    midi_channels,
    int_schema,
    16),
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
  YAML_FIELD_INT (Channel_v1, all_midi_channels),
  YAML_FIELD_INT (
    Channel_v1,
    record_set_automatically),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t channel_schema_v1 = {
  YAML_VALUE_PTR (
    Channel_v1,
    channel_fields_schema_v1),
};

#endif
