/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * @file
 *
 * Hardware processor schema.
 */

#ifndef __SCHEMAS_AUDIO_HARDWARE_PROCESSOR_H__
#define __SCHEMAS_AUDIO_HARDWARE_PROCESSOR_H__

#include <stdbool.h>

#include "schemas/audio/ext_port.h"
#include "schemas/audio/port.h"

typedef struct HardwareProcessor_v1
{
  int           schema_version;
  bool          is_input;
  char **       selected_midi_ports;
  int           num_selected_midi_ports;
  char **       selected_audio_ports;
  int           num_selected_audio_ports;
  ExtPort_v1 ** ext_audio_ports;
  int           num_ext_audio_ports;
  size_t        ext_audio_ports_size;
  ExtPort_v1 ** ext_midi_ports;
  int           num_ext_midi_ports;
  size_t        ext_midi_ports_size;
  Port_v1 **    audio_ports;
  int           num_audio_ports;
  Port_v1 **    midi_ports;
  int           num_midi_ports;
  bool          setup;
  bool          activated;
} HardwareProcessor_v1;

static const cyaml_schema_field_t
  hardware_processor_fields_schema_v1[] = {
    YAML_FIELD_INT (HardwareProcessor_v1, schema_version),
    YAML_FIELD_INT (HardwareProcessor_v1, is_input),
    YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
      HardwareProcessor_v1,
      ext_audio_ports,
      ext_port_schema_v1),
    YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
      HardwareProcessor_v1,
      ext_midi_ports,
      ext_port_schema_v1),
    YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
      HardwareProcessor_v1,
      audio_ports,
      port_schema_v1),
    YAML_FIELD_DYN_PTR_ARRAY_VAR_COUNT_OPT (
      HardwareProcessor_v1,
      midi_ports,
      port_schema_v1),

    CYAML_FIELD_END
  };

static const cyaml_schema_value_t hardware_processor_schema_v1 = {
  YAML_VALUE_PTR (
    HardwareProcessor_v1,
    hardware_processor_fields_schema_v1),
};

#endif
