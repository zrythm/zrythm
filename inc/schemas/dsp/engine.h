// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-FileCopyrightText: © 2020 Ryan Gonzalez <rymg19 at gmail dot com>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio engine schema.
 */

#ifndef __SCHEMAS_AUDIO_ENGINE_H__
#define __SCHEMAS_AUDIO_ENGINE_H__

#include "schemas/dsp/control_room.h"
#include "schemas/dsp/ext_port.h"
#include "schemas/dsp/hardware_processor.h"
#include "schemas/dsp/pool.h"
#include "schemas/dsp/sample_processor.h"
#include "schemas/dsp/transport.h"
#include "utils/types.h"

#include "zix/sem.h"

typedef enum AudioEngineJackTransportType_v1
{
  AUDIO_ENGINE_JACK_TIMEBASE_MASTER_v1,
  AUDIO_ENGINE_JACK_TRANSPORT_CLIENT_v1,
  AUDIO_ENGINE_NO_JACK_TRANSPORT_v1,
} AudioEngineJackTransportType_v1;

static const cyaml_strval_t jack_transport_type_strings_v1[] = {
  { "Timebase master",   AUDIO_ENGINE_JACK_TIMEBASE_MASTER_v1  },
  { "Transport client",  AUDIO_ENGINE_JACK_TRANSPORT_CLIENT_v1 },
  { "No JACK transport", AUDIO_ENGINE_NO_JACK_TRANSPORT_v1     },
};

typedef struct AudioEngine_v1
{
  int                             schema_version;
  AudioEngineJackTransportType_v1 transport_type;
  sample_rate_t                   sample_rate;
  double                          frames_per_tick;
  HardwareProcessor_v1 *          hw_in_processor;
  HardwareProcessor_v1 *          hw_out_processor;
  ControlRoom_v1 *                control_room;
  AudioPool_v1 *                  pool;
  StereoPorts_v1 *                monitor_out;
  Port_v1 *                       midi_editor_manual_press;
  Port_v1 *                       midi_in;
  Transport_v1 *                  transport;
  SampleProcessor_v1 *            sample_processor;
} AudioEngine_v1;

static const cyaml_schema_field_t engine_fields_schema_v1[] = {
  YAML_FIELD_INT (AudioEngine_v1, schema_version),
  YAML_FIELD_ENUM (AudioEngine_v1, transport_type, jack_transport_type_strings_v1),
  YAML_FIELD_INT (AudioEngine_v1, sample_rate),
  YAML_FIELD_FLOAT (AudioEngine_v1, frames_per_tick),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    monitor_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    midi_editor_manual_press,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (AudioEngine_v1, midi_in, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (AudioEngine_v1, transport, transport_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (AudioEngine_v1, pool, audio_pool_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    control_room,
    control_room_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    sample_processor,
    sample_processor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    hw_in_processor,
    hardware_processor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    hw_out_processor,
    hardware_processor_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t engine_schema_v1 = {
  YAML_VALUE_PTR (AudioEngine_v1, engine_fields_schema_v1),
};

typedef struct AudioEngine_v2
{
  int                             schema_version;
  AudioEngineJackTransportType_v1 transport_type;
  sample_rate_t                   sample_rate;
  double                          frames_per_tick;
  HardwareProcessor_v1 *          hw_in_processor;
  HardwareProcessor_v1 *          hw_out_processor;
  ControlRoom_v2 *                control_room;
  AudioPool_v1 *                  pool;
  StereoPorts_v1 *                monitor_out;
  Port_v1 *                       midi_editor_manual_press;
  Port_v1 *                       midi_in;
  Transport_v1 *                  transport;
  SampleProcessor_v2 *            sample_processor;
} AudioEngine_v2;

static const cyaml_schema_field_t engine_fields_schema_v2[] = {
  YAML_FIELD_INT (AudioEngine_v2, schema_version),
  YAML_FIELD_ENUM (AudioEngine_v2, transport_type, jack_transport_type_strings_v1),
  YAML_FIELD_INT (AudioEngine_v2, sample_rate),
  YAML_FIELD_FLOAT (AudioEngine_v2, frames_per_tick),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v2,
    monitor_out,
    stereo_ports_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v2,
    midi_editor_manual_press,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (AudioEngine_v2, midi_in, port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (AudioEngine_v2, transport, transport_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (AudioEngine_v2, pool, audio_pool_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v2,
    control_room,
    control_room_fields_schema_v2),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v2,
    sample_processor,
    sample_processor_fields_schema_v2),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v2,
    hw_in_processor,
    hardware_processor_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v2,
    hw_out_processor,
    hardware_processor_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t engine_schema_v2 = {
  YAML_VALUE_PTR (AudioEngine_v2, engine_fields_schema_v2),
};

AudioEngine_v2 *
engine_upgrade_from_v1 (AudioEngine_v1 * old);

#endif
