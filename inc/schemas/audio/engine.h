/*
 * Copyright (C) 2018-2021 Alexandros Theodotou <alex at zrythm dot org>
 * Copyright (C) 2020 Ryan Gonzalez <rymg19 at gmail dot com>
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
 * Audio engine schema.
 */

#ifndef __SCHEMAS_AUDIO_ENGINE_H__
#define __SCHEMAS_AUDIO_ENGINE_H__

#include "audio/control_room.h"
#include "audio/ext_port.h"
#include "audio/hardware_processor.h"
#include "audio/pan.h"
#include "audio/pool.h"
#include "audio/sample_processor.h"
#include "audio/transport.h"
#include "utils/types.h"

#include "zix/sem.h"

typedef enum AudioEngineJackTransportType_v1
{
  AUDIO_ENGINE_JACK_TIMEBASE_MASTER_v1,
  AUDIO_ENGINE_JACK_TRANSPORT_CLIENT_v1,
  AUDIO_ENGINE_NO_JACK_TRANSPORT_v1,
} AudioEngineJackTransportType_v1;

static const cyaml_strval_t jack_transport_type_strings_v1[] = {
  {"Timebase master",    AUDIO_ENGINE_JACK_TIMEBASE_MASTER_v1 },
  { "Transport client",  AUDIO_ENGINE_JACK_TRANSPORT_CLIENT_v1},
  { "No JACK transport", AUDIO_ENGINE_NO_JACK_TRANSPORT_v1    },
};

typedef struct AudioEngine_v1
{
  int           schema_version;
  volatile long cycle;
  void *        client;
  YamlDummyEnum transport_type;
  YamlDummyEnum audio_backend;
  YamlDummyEnum midi_backend;
  ;
  nframes_t              block_length;
  size_t                 midi_buf_size;
  sample_rate_t          sample_rate;
  double                 frames_per_tick;
  double                 ticks_per_frame;
  int                    buf_size_set;
  void *                 router;
  HardwareProcessor_v1 * hw_in_processor;
  void *                 midi_clock_in;
  ControlRoom_v1 *       control_room;
  AudioPool_v1 *         pool;
  StereoPorts_v1 *       dummy_input;
  StereoPorts_v1 *       monitor_out;
  int                    trigger_midi_activity;
  Port_v1 *              midi_editor_manual_press;
  Port_v1 *              midi_in;
  nframes_t              nframes;
  ZixSem                 port_operation_lock;
  volatile gint          run;
  gint                   exporting;
  gint                   skip_cycle;
  volatile gint          panic;
  void *                 playback_handle;
  void *                 seq_handle;
  void *                 hw_params;
  void *                 sw_params;
  float *                alsa_out_buf;
  volatile gint          filled_stereo_out_bufs;
  gint                   preparing_for_process;
  void *                 pa_stream;
  float *                pa_out_buf;
  void *                 mme_in_devs[1024];
  int                    num_mme_in_devs;
  void *                 mme_out_devs[1024];
  int                    num_mme_out_devs;
  uint32_t               sdl_dev;
  void *                 rtaudio;
  void *                 pulse_mainloop;
  void *                 pulse_context;
  void *                 pulse_stream;
  gboolean               pulse_notified_underflow;
  void *                 dummy_audio_thread;
  int                    stop_dummy_audio_thread;
  Transport_v1 *         transport;
  YamlDummyEnum          pan_law;
  YamlDummyEnum          pan_algo;
  gint64                 last_time_taken;
  gint64                 max_time_taken;
  gint64                 timestamp_start;
  gint64                 timestamp_end;
  nframes_t              remaining_latency_preroll;
  SampleProcessor_v1 *   sample_processor;
  int                    capture_cc;
  midi_byte_t            last_cc[3];
  gint64                 last_xrun_notification;
  bool                   denormal_prevention_val_positive;
  float                  denormal_prevention_val;
  gint64                 zrythm_start_time;
  int                    limit_reached;
  YamlDummyEnum          bounce_mode;
  void *                 metronome;
  void *                 ev_queue;
  void *                 ev_pool;
  guint                  process_source_id;
  int                    processing_events;
  gint64                 last_events_process_started;
  gint64                 last_events_processed;
  volatile gint          cycle_running;
  bool                   pre_setup;
  bool                   setup;
  bool                   activated;
} AudioEngine_v1;

static const cyaml_schema_field_t engine_fields_schema_v1[] = {
  YAML_FIELD_INT (AudioEngine_v1, schema_version),
  YAML_FIELD_ENUM (
    AudioEngine_v1,
    transport_type,
    jack_transport_type_strings_v1),
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
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    midi_in,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    transport,
    transport_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    AudioEngine_v1,
    pool,
    audio_pool_fields_schema_v1),
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

  CYAML_FIELD_END
};

static const cyaml_schema_value_t engine_schema_v1 = {
  YAML_VALUE_PTR (AudioEngine_v1, engine_fields_schema_v1),
};

#endif
