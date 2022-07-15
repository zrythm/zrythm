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
 * Port schema.
 */

#ifndef __SCHEMAS_AUDIO_PORTS_H__
#define __SCHEMAS_AUDIO_PORTS_H__

#include "zrythm-config.h"

typedef enum PortInternalType_v1
{
  INTERNAL_NONE_v1,
  INTERNAL_LV2_PORT_v1,
  INTERNAL_JACK_PORT_v1,
  INTERNAL_PA_PORT_v1,
  INTERNAL_ALSA_SEQ_PORT_v1,
} PortInternalType_v1;

static const cyaml_strval_t port_internal_type_strings_v1[] = {
  {"LV2 port",   INTERNAL_LV2_PORT_v1 },
  { "JACK port", INTERNAL_JACK_PORT_v1},
};

typedef struct Port_v1
{
  int                 schema_version;
  PortIdentifier_v1   id;
  int                 exposed_to_backend;
  float *             buf;
  void *              midi_events;
  struct Port_v1 **   srcs;
  struct Port_v1 **   dests;
  PortIdentifier_v1 * src_ids;
  PortIdentifier_v1 * dest_ids;
  float *             multipliers;
  float *             src_multipliers;
  int *               dest_locked;
  int *               src_locked;
  int *               dest_enabled;
  int *               src_enabled;
  int                 num_srcs;
  int                 num_dests;
  size_t              srcs_size;
  size_t              dests_size;
  PortInternalType_v1 internal_type;
  float               minf;
  float               maxf;
  float               zerof;
  float               deff;
  int                 vst_param_id;
  int                 carla_param_id;
  void *              data;
  void *              mme_connections[40];
  int                 num_mme_connections;
  ZixSem              mme_connections_sem;
  gint64              last_midi_dequeue;
  void *              rtmidi_ins[128];
  int                 num_rtmidi_ins;
  void *              rtmidi_outs[128];
  int                 num_rtmidi_outs;
  void *              rtaudio_ins[128];
  int                 num_rtaudio_ins;
  void *              scale_points;
  int                 num_scale_points;
  float               control;
  float               unsnapped_control;
  bool                value_changed_from_reading;
  gint64              last_change;
  void *              tmp_plugin;
  void *              sample_processor;
  int                 initialized;
  float               base_value;
  long                capture_latency;
  long                playback_latency;
  int                 deleting;
  bool                write_ring_buffers;
  volatile int        has_midi_events;
  gint64              last_midi_event_time;
  ZixRing *           audio_ring;
  ZixRing *           midi_ring;
  float               peak;
  gint64              peak_timestamp;
  midi_byte_t         last_midi_status;
  const void *        lilv_port;
  LV2_URID            value_type;
  void *              evbuf;
  void *              widget;
  size_t              min_buf_size;
  int                 lilv_port_index;
  bool                received_ui_event;
  float               last_sent_control;
  bool                automating;
  bool                old_api;
  void *              at;
  void *              ext_port;
  bool                is_project;
  int                 magic;
} Port_v1;

static const cyaml_schema_field_t port_fields_schema__v1[] = {
  YAML_FIELD_INT (Port_v1, schema_version),
  YAML_FIELD_MAPPING_EMBEDDED (
    Port_v1,
    id,
    port_identifier_fields_schema_v1),
  YAML_FIELD_INT (Port_v1, exposed_to_backend),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_ids",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    dest_ids,
    num_dests,
    &port_identifier_schema_default_v1,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_ids",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    src_ids,
    num_srcs,
    &port_identifier_schema_default_v1,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "multipliers",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    multipliers,
    num_dests,
    &float_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_multipliers",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    src_multipliers,
    num_srcs,
    &float_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_locked",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    dest_locked,
    num_dests,
    &int_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_locked",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    src_locked,
    num_srcs,
    &int_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "dest_enabled",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    dest_enabled,
    num_dests,
    &int_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_SEQUENCE_COUNT (
    "src_enabled",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    Port_v1,
    src_enabled,
    num_srcs,
    &int_schema,
    0,
    CYAML_UNLIMITED),
  CYAML_FIELD_ENUM (
    "internal_type",
    CYAML_FLAG_DEFAULT,
    Port_v1,
    internal_type,
    port_internal_type_strings_v1,
    CYAML_ARRAY_LEN (port_internal_type_strings_v1)),
  YAML_FIELD_FLOAT (Port_v1, control),
  YAML_FIELD_FLOAT (Port_v1, minf),
  YAML_FIELD_FLOAT (Port_v1, maxf),
  YAML_FIELD_FLOAT (Port_v1, zerof),
  YAML_FIELD_FLOAT (Port_v1, deff),
  YAML_FIELD_INT (Port_v1, vst_param_id),
  YAML_FIELD_INT (Port_v1, carla_param_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t port_schema_v1 = {
  YAML_VALUE_PTR_NULLABLE (Port_v1, port_fields_schema_v1),
};

/**
 * L & R port, for convenience.
 *
 * Must ONLY be created via stereo_ports_new()
 */
typedef struct StereoPorts_v1
{
  int       schema_version;
  Port_v1 * l;
  Port_v1 * r;
} StereoPorts_v1;

static const cyaml_schema_field_t stereo_ports_fields_schema_v1[] = {
  YAML_FIELD_INT (StereoPorts_v1, schema_version),
  YAML_FIELD_MAPPING_PTR (
    StereoPorts_v1,
    l,
    port_fields_schema_v1),
  YAML_FIELD_MAPPING_PTR (
    StereoPorts_v1,
    r,
    port_fields_schema_v1),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t stereo_ports_schema_v1 = {
  YAML_VALUE_PTR (StereoPorts_v1, stereo_ports_fields_schema_v1),
};

#endif
