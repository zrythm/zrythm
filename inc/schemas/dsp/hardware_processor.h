// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Hardware processor schema.
 */

#ifndef __SCHEMAS_AUDIO_HARDWARE_PROCESSOR_H__
#define __SCHEMAS_AUDIO_HARDWARE_PROCESSOR_H__

#include <stdbool.h>

#include "schemas/dsp/ext_port.h"
#include "schemas/dsp/port.h"

typedef struct HardwareProcessor_v1
{
  int           schema_version;
  bool          is_input;
  ExtPort_v1 ** ext_audio_ports;
  int           num_ext_audio_ports;
  ExtPort_v1 ** ext_midi_ports;
  int           num_ext_midi_ports;
  Port_v1 **    audio_ports;
  int           num_audio_ports;
  Port_v1 **    midi_ports;
  int           num_midi_ports;
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

HardwareProcessor *
hardware_processor_upgrade_from_v1 (
  HardwareProcessor_v1 * old);

#endif
