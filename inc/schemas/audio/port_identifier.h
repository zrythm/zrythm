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
 * Port identifier.
 */

#ifndef __SCHEMAS_AUDIO_PORT_IDENTIFIER_H__
#define __SCHEMAS_AUDIO_PORT_IDENTIFIER_H__

typedef enum PortFlow_v1
{
  FLOW_UNKNOWN_v1,
  FLOW_INPUT_v1,
  FLOW_OUTPUT_v1,
} PortFlow_v1;

static const cyaml_strval_t port_flow_strings_v1[] = {
  {"unknown", FLOW_UNKNOWN_v1},
  { "input",  FLOW_INPUT_v1  },
  { "output", FLOW_OUTPUT_v1 },
};

typedef enum PortType_v1
{
  TYPE_UNKNOWN,
  TYPE_CONTROL,
  TYPE_AUDIO,
  TYPE_EVENT,
  TYPE_CV_v1,
} PortType_v1;

static const cyaml_strval_t port_type_strings_v1[] = {
  {"unknown",  TYPE_UNKNOWN_v1},
  { "control", TYPE_CONTROL_v1},
  { "audio",   TYPE_AUDIO_v1  },
  { "event",   TYPE_EVENT_v1  },
  { "cv",      TYPE_CV_v1     },
};

typedef enum PortUnit_v1
{
  PORT_UNIT_NONE_v1,
  PORT_UNIT_HZ_v1,
  PORT_UNIT_MHZ_v1,
  PORT_UNIT_DB_v1,
  PORT_UNIT_DEGREES_v1,
  PORT_UNIT_SECONDS_v1,
  PORT_UNIT_MS,
} PortUnit_v1;

static const cyaml_strval_t port_unit_strings_v1[] = {
  {"none", PORT_UNIT_NONE_v1   },
  { "Hz",  PORT_UNIT_HZ_v1     },
  { "MHz", PORT_UNIT_MHZ_v1    },
  { "dB",  PORT_UNIT_DB_v1     },
  { "°",  PORT_UNIT_DEGREES_v1},
  { "s",   PORT_UNIT_SECONDS_v1},
  { "ms",  PORT_UNIT_MS_v1     },
};

typedef enum PortOwnerType_v1
{
  PORT_OWNER_TYPE_BACKEND_v1,
  PORT_OWNER_TYPE_PLUGIN_v1,
  PORT_OWNER_TYPE_TRACK_v1,
  PORT_OWNER_TYPE_FADER_v1,
  PORT_OWNER_TYPE_PREFADER_v1,
  PORT_OWNER_TYPE_MONITOR_FADER_v1,
  PORT_OWNER_TYPE_TRACK_PROCESSOR_v1,
  PORT_OWNER_TYPE_SAMPLE_PROCESSOR_v1,
  PORT_OWNER_TYPE_HW_v1,
  PORT_OWNER_TYPE_TRANSPORT_v1,
} PortOwnerType_v1;

static const cyaml_strval_t port_owner_type_strings_v1[] = {
  {"backend",           PORT_OWNER_TYPE_BACKEND_v1  },
  { "plugin",           PORT_OWNER_TYPE_PLUGIN_v1   },
  { "track",            PORT_OWNER_TYPE_TRACK_v1    },
  { "pre-fader",        PORT_OWNER_TYPE_PREFADER_v1 },
  { "fader",            PORT_OWNER_TYPE_FADER_v1    },
  { "track processor",
   PORT_OWNER_TYPE_TRACK_PROCESSOR_v1               },
  { "monitor fader",
   PORT_OWNER_TYPE_MONITOR_FADER_v1                 },
  { "sample processor",
   PORT_OWNER_TYPE_SAMPLE_PROCESSOR_v1              },
  { "hw",               PORT_OWNER_TYPE_HW_v1       },
  { "transport",        PORT_OWNER_TYPE_TRANSPORT_v1},
};

/**
 * Port flags.
 */
typedef enum PortFlags_v1
{
  PORT_FLAG_STEREO_L_v1 = 1 << 0,
  PORT_FLAG_STEREO_R_v1 = 1 << 1,
  PORT_FLAG_PIANO_ROLL_v1 = 1 << 2,
  PORT_FLAG_SIDECHAIN_v1 = 1 << 3,
  PORT_FLAG_MAIN_PORT_v1 = 1 << 4,
  PORT_FLAG_MANUAL_PRESS_v1 = 1 << 5,
  PORT_FLAG_AMPLITUDE_v1 = 1 << 6,
  PORT_FLAG_STEREO_BALANCE_v1 = 1 << 7,
  PORT_FLAG_WANT_POSITION_v1 = 1 << 8,
  PORT_FLAG_TRIGGER_v1 = 1 << 9,
  PORT_FLAG_TOGGLE_v1 = 1 << 10,
  PORT_FLAG_INTEGER_v1 = 1 << 11,
  PORT_FLAG_FREEWHEEL_v1 = 1 << 12,
  PORT_FLAG_REPORTS_LATENCY_v1 = 1 << 13,
  PORT_FLAG_NOT_ON_GUI_v1 = 1 << 14,
  PORT_FLAG_PLUGIN_ENABLED_v1 = 1 << 15,
  PORT_FLAG_PLUGIN_CONTROL_v1 = 1 << 16,
  PORT_FLAG_CHANNEL_MUTE_v1 = 1 << 17,
  PORT_FLAG_CHANNEL_FADER_v1 = 1 << 18,
  PORT_FLAG_AUTOMATABLE_v1 = 1 << 19,
  PORT_FLAG_MIDI_AUTOMATABLE_v1 = 1 << 20,
  PORT_FLAG_SEND_RECEIVABLE_v1 = 1 << 21,
  PORT_FLAG_BPM_v1 = 1 << 22,
  PORT_FLAG_TIME_SIG_v1 = 1 << 23,
  PORT_FLAG_GENERIC_PLUGIN_PORT_v1 = 1 << 24,
  PORT_FLAG_PLUGIN_GAIN_v1 = 1 << 25,
  PORT_FLAG_TP_MONO_v1 = 1 << 26,
  PORT_FLAG_TP_INPUT_GAIN_v1 = 1 << 27,
  PORT_FLAG_HW_v1 = 1 << 28,
  PORT_FLAG_MODULATOR_MACRO_v1 = 1 << 29,
  PORT_FLAG_LOGARITHMIC_v1 = 1 << 30,
  PORT_FLAG_IS_PROPERTY_v1 = 1 << 31,
} PortFlags_v1;

typedef enum PortFlags2_v1
{
  /** Transport ports. */
  PORT_FLAG2_TRANSPORT_ROLL_v1 = 1 << 0,
  PORT_FLAG2_TRANSPORT_STOP_v1 = 1 << 1,
  PORT_FLAG2_TRANSPORT_BACKWARD_v1 = 1 << 2,
  PORT_FLAG2_TRANSPORT_FORWARD_v1 = 1 << 3,
  PORT_FLAG2_TRANSPORT_LOOP_TOGGLE_v1 = 1 << 4,
  PORT_FLAG2_TRANSPORT_REC_TOGGLE_v1 = 1 << 5,
  PORT_FLAG2_SUPPORTS_PATCH_MESSAGE_v1 = 1 << 6,
  PORT_FLAG2_ENUMERATION_v1 = 1 << 7,
  PORT_FLAG2_URI_PARAM_v1 = 1 << 8,
  PORT_FLAG2_SEQUENCE_v1 = 1 << 9,
  PORT_FLAG2_SUPPORTS_MIDI_v1 = 1 << 10,
} PortFlags2_v1;

static const cyaml_bitdef_t port_flags_bitvals_v1[] = {
  {.name = "stereo_l",             .offset = 0,  .bits = 1},
  { .name = "stereo_r",            .offset = 1,  .bits = 1},
  { .name = "piano_roll",          .offset = 2,  .bits = 1},
  { .name = "sidechain",           .offset = 3,  .bits = 1},
  { .name = "main_port",           .offset = 4,  .bits = 1},
  { .name = "manual_press",        .offset = 5,  .bits = 1},
  { .name = "amplitude",           .offset = 6,  .bits = 1},
  { .name = "stereo_balance",      .offset = 7,  .bits = 1},
  { .name = "want_position",       .offset = 8,  .bits = 1},
  { .name = "trigger",             .offset = 9,  .bits = 1},
  { .name = "toggle",              .offset = 10, .bits = 1},
  { .name = "integer",             .offset = 11, .bits = 1},
  { .name = "freewheel",           .offset = 12, .bits = 1},
  { .name = "reports_latency",
   .offset = 13,
   .bits = 1                                              },
  { .name = "not_on_gui",          .offset = 14, .bits = 1},
  { .name = "plugin_enabled",
   .offset = 15,
   .bits = 1                                              },
  { .name = "plugin_control",
   .offset = 16,
   .bits = 1                                              },
  { .name = "channel_mute",        .offset = 17, .bits = 1},
  { .name = "channel_fader",       .offset = 18, .bits = 1},
  { .name = "automatable",         .offset = 19, .bits = 1},
  { .name = "midi_automatable",
   .offset = 20,
   .bits = 1                                              },
  { .name = "send_receivable",
   .offset = 21,
   .bits = 1                                              },
  { .name = "bpm",                 .offset = 22, .bits = 1},
  { .name = "time_sig",            .offset = 23, .bits = 1},
  { .name = "generic_plugin_port",
   .offset = 24,
   .bits = 1                                              },
  { .name = "plugin_gain",         .offset = 25, .bits = 1},
  { .name = "tp_mono",             .offset = 26, .bits = 1},
  { .name = "tp_input_gain",       .offset = 27, .bits = 1},
  { .name = "hw",                  .offset = 28, .bits = 1},
  { .name = "modulator_macro",
   .offset = 29,
   .bits = 1                                              },
  { .name = "logarithmic",         .offset = 30, .bits = 1},
  { .name = "is_property",         .offset = 31, .bits = 1},
};

static const cyaml_bitdef_t port_flags2_bitvals_v1[] = {
  {.name = "transport_roll",         .offset = 0,  .bits = 1},
  { .name = "transport_stop",        .offset = 1,  .bits = 1},
  { .name = "transport_backward",
   .offset = 2,
   .bits = 1                                                },
  { .name = "transport_forward",
   .offset = 3,
   .bits = 1                                                },
  { .name = "transport_loop_toggle",
   .offset = 4,
   .bits = 1                                                },
  { .name = "transport_rec_toggle",
   .offset = 5,
   .bits = 1                                                },
  { .name = "patch_message",         .offset = 6,  .bits = 1},
  { .name = "enumeration",           .offset = 7,  .bits = 1},
  { .name = "uri_param",             .offset = 8,  .bits = 1},
  { .name = "sequence",              .offset = 9,  .bits = 1},
  { .name = "supports_midi",         .offset = 10, .bits = 1},
};

typedef struct PortIdentifier_v1
{
  int                 schema_version;
  char *              label;
  char *              sym;
  char *              uri;
  char *              comment;
  PortOwnerType_v1    owner_type;
  PortType_v1         type;
  PortFlow_v1         flow;
  PortFlags_v1        flags;
  PortFlags2_v1       flags2;
  PortUnit_v1         unit;
  PluginIdentifier_v1 plugin_id;
  char *              port_group;
  char *              ext_port_id;
  int                 track_pos;
  int                 port_index;
} PortIdentifier_v1;

static const cyaml_schema_field_t port_identifier_fields_schema[] = {
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier_v1,
    label),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier_v1,
    sym),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier_v1,
    uri),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier_v1,
    comment),
  YAML_FIELD_ENUM (
    PortIdentifier_v1,
    owner_type,
    port_owner_type_strings_v1),
  YAML_FIELD_ENUM (
    PortIdentifier_v1,
    type,
    port_type_strings_v1),
  YAML_FIELD_ENUM (
    PortIdentifier_v1,
    flow,
    port_flow_strings_v1),
  YAML_FIELD_ENUM (
    PortIdentifier_v1,
    unit,
    port_unit_strings_v1),
  YAML_FIELD_BITFIELD (
    PortIdentifier_v1,
    flags,
    port_flags_bitvals_v1),
  YAML_FIELD_BITFIELD (
    PortIdentifier_v1,
    flags2,
    port_flags2_bitvals_v1),
  YAML_FIELD_INT (PortIdentifier_v1, track_pos),
  YAML_FIELD_MAPPING_EMBEDDED (
    PortIdentifier_v1,
    plugin_id,
    plugin_identifier_fields_schema_v1),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier_v1,
    port_group),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier_v1,
    ext_port_id),
  YAML_FIELD_INT (PortIdentifier_v1, port_index),

  CYAML_FIELD_END,
};

static const cyaml_schema_value_t
  port_identifier_schema_v1 = {
    CYAML_VALUE_MAPPING (
      CYAML_FLAG_POINTER,
      PortIdentifier_v1,
      port_identifier_fields_schema_v1),
  };

static const cyaml_schema_value_t
  port_identifier_schema_default_v1 = {
    CYAML_VALUE_MAPPING (
      CYAML_FLAG_DEFAULT,
      PortIdentifier_v1,
      port_identifier_fields_schema_v1),
  };

#endif
