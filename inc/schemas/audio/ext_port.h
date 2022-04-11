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

#ifndef __SCHEMAS_AUDIO_EXT_PORT_H__
#define __SCHEMAS_AUDIO_EXT_PORT_H__

/**
 * \file
 *
 * External port schema.
 */

#include "utils/types.h"
#include "utils/yaml.h"

typedef enum ExtPortType_v1
{
  EXT_PORT_TYPE_JACK_v1,
  EXT_PORT_TYPE_ALSA_v1,
  EXT_PORT_TYPE_WINDOWS_MME_v1,
  EXT_PORT_TYPE_RTMIDI_v1,
  EXT_PORT_TYPE_RTAUDIO_v1,
} ExtPortType_v1;

static const cyaml_strval_t
  ext_port_type_strings_v1[] = {
    {"JACK",         EXT_PORT_TYPE_JACK_v1       },
    { "ALSA",        EXT_PORT_TYPE_ALSA_v1       },
    { "Windows MME", EXT_PORT_TYPE_WINDOWS_MME_v1},
    { "RtMidi",      EXT_PORT_TYPE_RTMIDI_v1     },
    { "RtAudio",     EXT_PORT_TYPE_RTAUDIO_v1    },
};

typedef struct ExtPort_v1
{
  int            schema_version;
  void *         jport;
  char *         full_name;
  char *         short_name;
  char *         alias1;
  char *         alias2;
  int            num_aliases;
  void *         mme_dev;
  unsigned int   rtaudio_channel_idx;
  char *         rtaudio_dev_name;
  unsigned int   rtaudio_id;
  bool           rtaudio_is_input;
  bool           rtaudio_is_duplex;
  void *         rtaudio_dev;
  unsigned int   rtmidi_id;
  void *         rtmidi_dev;
  ExtPortType_v1 type;
  bool           is_midi;
  int            hw_processor_index;
  bool           active;
  void *         port;
} ExtPort_v1;

static const cyaml_schema_field_t
  ext_port_fields_schema_v1[] = {
    YAML_FIELD_INT (ExtPort_v1, schema_version),
    YAML_FIELD_STRING_PTR (ExtPort_v1, full_name),
    YAML_FIELD_STRING_PTR_OPTIONAL (
      ExtPort_v1,
      short_name),
    YAML_FIELD_STRING_PTR_OPTIONAL (
      ExtPort_v1,
      alias1),
    YAML_FIELD_STRING_PTR_OPTIONAL (
      ExtPort_v1,
      alias2),
    YAML_FIELD_STRING_PTR_OPTIONAL (
      ExtPort_v1,
      rtaudio_dev_name),
    YAML_FIELD_INT (ExtPort_v1, num_aliases),
    YAML_FIELD_INT (ExtPort_v1, is_midi),
    YAML_FIELD_ENUM (
      ExtPort_v1,
      type,
      ext_port_type_strings_v1),
    YAML_FIELD_UINT (ExtPort_v1, rtaudio_channel_idx),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t ext_port_schema_v1 = {
  YAML_VALUE_PTR (
    ExtPort_v1,
    ext_port_fields_schema_v1),
};

#endif
