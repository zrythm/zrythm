// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __SCHEMAS_AUDIO_CHANNEL_SEND_H__
#define __SCHEMAS_AUDIO_CHANNEL_SEND_H__

#include "common/utils/yaml.h"
#include "gui/cpp/backend/cyaml_schemas/dsp/port.h"

typedef struct ChannelSend_v1
{
  int              schema_version;
  int              slot;
  StereoPorts_v1 * stereo_in;
  Port_v1 *        midi_in;
  StereoPorts_v1 * stereo_out;
  Port_v1 *        midi_out;
  Port_v1 *        amount;
  Port_v1 *        enabled;
  bool             is_sidechain;
  unsigned int     track_name_hash;
} ChannelSend_v1;

static const cyaml_schema_field_t channel_send_fields_schema_v1[] = {
  YAML_FIELD_INT (ChannelSend_v1, schema_version),
  YAML_FIELD_INT (ChannelSend_v1, slot),
  YAML_FIELD_MAPPING_PTR (ChannelSend_v1, amount, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (ChannelSend_v1, enabled, port_fields_schema_v1),
  YAML_FIELD_INT (ChannelSend_v1, is_sidechain),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (ChannelSend_v1, midi_in, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    ChannelSend_v1,
    stereo_in,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (ChannelSend_v1, midi_out, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR_OPTIONAL (
    ChannelSend_v1,
    stereo_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_UINT (ChannelSend_v1, track_name_hash),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t channel_send_schema_v1 = {
  YAML_VALUE_PTR_NULLABLE (ChannelSend_v1, channel_send_fields_schema_v1),
};

#endif
