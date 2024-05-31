// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Port identifier.
 */

#ifndef __SCHEMAS_AUDIO_PORT_IDENTIFIER_H__
#define __SCHEMAS_AUDIO_PORT_IDENTIFIER_H__

#include "schemas/plugins/plugin_identifier.h"
#include "utils/yaml.h"

typedef struct PortIdentifier PortIdentifier;

typedef enum PortFlow_v1
{
  Z_PORT_FLOW_UNKNOWN_v1,
  Z_PORT_FLOW_INPUT_v1,
  Z_PORT_FLOW_OUTPUT_v1,
} PortFlow_v1;

static const cyaml_strval_t port_flow_strings_v1[] = {
  {"unknown", Z_PORT_FLOW_UNKNOWN_v1},
  { "input",  Z_PORT_FLOW_INPUT_v1  },
  { "output", Z_PORT_FLOW_OUTPUT_v1 },
};

typedef enum PortType_v1
{
  TYPE_UNKNOWN_v1,
  Z_PORT_TYPE_CONTROL_v1,
  Z_PORT_TYPE_AUDIO_v1,
  Z_PORT_TYPE_EVENT_v1,
  Z_PORT_TYPE_CV_v1,
} PortType_v1;

static const cyaml_strval_t port_type_strings_v1[] = {
  {"unknown",  TYPE_UNKNOWN_v1       },
  { "control", Z_PORT_TYPE_CONTROL_v1},
  { "audio",   Z_PORT_TYPE_AUDIO_v1  },
  { "event",   Z_PORT_TYPE_EVENT_v1  },
  { "cv",      Z_PORT_TYPE_CV_v1     },
};

typedef enum PortUnit_v1
{
  PORT_UNIT_NONE_v1,
  PORT_UNIT_HZ_v1,
  PORT_UNIT_MHZ_v1,
  PORT_UNIT_DB_v1,
  PORT_UNIT_DEGREES_v1,
  PORT_UNIT_SECONDS_v1,
  PORT_UNIT_MS_v1,
  PORT_UNIT_US_v1,
} PortUnit_v1;

static const cyaml_strval_t port_unit_strings_v1[] = {
  {"none", PORT_UNIT_NONE_v1   },
  { "Hz",  PORT_UNIT_HZ_v1     },
  { "MHz", PORT_UNIT_MHZ_v1    },
  { "dB",  PORT_UNIT_DB_v1     },
  { "°",  PORT_UNIT_DEGREES_v1},
  { "s",   PORT_UNIT_SECONDS_v1},
  { "ms",  PORT_UNIT_MS_v1     },
  { "μs", PORT_UNIT_US_v1     },
};

typedef enum ZPortOwnerType_v1
{
  Z_PORT_OWNER_TYPE_AUDIO_ENGINE_v1,
  Z_PORT_OWNER_TYPE_PLUGIN_v1,
  Z_PORT_OWNER_TYPE_TRACK_v1,
  Z_PORT_OWNER_TYPE_CHANNEL_v1,
  Z_PORT_OWNER_TYPE_FADER_v1,
  Z_PORT_OWNER_TYPE_CHANNEL_SEND_v1,
  Z_PORT_OWNER_TYPE_TRACK_PROCESSOR_v1,
  Z_PORT_OWNER_TYPE_HW_v1,
  Z_PORT_OWNER_TYPE_TRANSPORT_v1,
  Z_PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR_v1,
} ZPortOwnerType_v1;

static const cyaml_strval_t port_owner_type_strings_v1[] = {
  {"audio engine",               Z_PORT_OWNER_TYPE_AUDIO_ENGINE_v1             },
  { "plugin",                    Z_PORT_OWNER_TYPE_PLUGIN_v1                   },
  { "track",                     Z_PORT_OWNER_TYPE_TRACK_v1                    },
  { "channel",                   Z_PORT_OWNER_TYPE_CHANNEL_v1                  },
  { "fader",                     Z_PORT_OWNER_TYPE_FADER_v1                    },
  { "channel send",              Z_PORT_OWNER_TYPE_CHANNEL_SEND_v1             },
  { "track processor",           Z_PORT_OWNER_TYPE_TRACK_PROCESSOR_v1          },
  { "hw",                        Z_PORT_OWNER_TYPE_HW_v1                       },
  { "transport",                 Z_PORT_OWNER_TYPE_TRANSPORT_v1                },
  { "modulator macro processor", Z_PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR_v1},
};

/**
 * Port flags.
 */
typedef enum ZPortFlags_v1
{
  Z_PORT_FLAG_STEREO_L_v1 = 1 << 0,
  Z_PORT_FLAG_STEREO_R_v1 = 1 << 1,
  Z_PORT_FLAG_PIANO_ROLL_v1 = 1 << 2,
  Z_PORT_FLAG_SIDECHAIN_v1 = 1 << 3,
  Z_PORT_FLAG_MAIN_PORT_v1 = 1 << 4,
  Z_PORT_FLAG_MANUAL_PRESS_v1 = 1 << 5,
  Z_PORT_FLAG_AMPLITUDE_v1 = 1 << 6,
  Z_PORT_FLAG_STEREO_BALANCE_v1 = 1 << 7,
  Z_PORT_FLAG_WANT_POSITION_v1 = 1 << 8,
  Z_PORT_FLAG_TRIGGER_v1 = 1 << 9,
  Z_PORT_FLAG_TOGGLE_v1 = 1 << 10,
  Z_PORT_FLAG_INTEGER_v1 = 1 << 11,
  Z_PORT_FLAG_FREEWHEEL_v1 = 1 << 12,
  Z_PORT_FLAG_REPORTS_LATENCY_v1 = 1 << 13,
  Z_PORT_FLAG_NOT_ON_GUI_v1 = 1 << 14,
  Z_PORT_FLAG_PLUGIN_ENABLED_v1 = 1 << 15,
  Z_PORT_FLAG_PLUGIN_CONTROL_v1 = 1 << 16,
  Z_PORT_FLAG_CHANNEL_MUTE_v1 = 1 << 17,
  Z_PORT_FLAG_CHANNEL_FADER_v1 = 1 << 18,
  Z_PORT_FLAG_AUTOMATABLE_v1 = 1 << 19,
  Z_PORT_FLAG_MIDI_AUTOMATABLE_v1 = 1 << 20,
  Z_PORT_FLAG_SEND_RECEIVABLE_v1 = 1 << 21,
  Z_PORT_FLAG_BPM_v1 = 1 << 22,
  Z_PORT_FLAG_GENERIC_PLUGIN_PORT_v1 = 1 << 23,
  Z_PORT_FLAG_PLUGIN_GAIN_v1 = 1 << 24,
  Z_PORT_FLAG_TP_MONO_v1 = 1 << 25,
  Z_PORT_FLAG_TP_INPUT_GAIN_v1 = 1 << 26,
  Z_PORT_FLAG_HW_v1 = 1 << 27,
  Z_PORT_FLAG_MODULATOR_MACRO_v1 = 1 << 28,
  Z_PORT_FLAG_LOGARITHMIC_v1 = 1 << 29,
  Z_PORT_FLAG_IS_PROPERTY_v1 = 1 << 30,
} ZPortFlags_v1;

typedef enum ZPortFlags2_v1
{
  /** Transport ports. */
  Z_PORT_FLAG2_TRANSPORT_ROLL_v1 = 1 << 0,
  Z_PORT_FLAG2_TRANSPORT_STOP_v1 = 1 << 1,
  Z_PORT_FLAG2_TRANSPORT_BACKWARD_v1 = 1 << 2,
  Z_PORT_FLAG2_TRANSPORT_FORWARD_v1 = 1 << 3,
  Z_PORT_FLAG2_TRANSPORT_LOOP_TOGGLE_v1 = 1 << 4,
  Z_PORT_FLAG2_TRANSPORT_REC_TOGGLE_v1 = 1 << 5,
  Z_PORT_FLAG2_SUPPORTS_PATCH_MESSAGE_v1 = 1 << 6,
  Z_PORT_FLAG2_ENUMERATION_v1 = 1 << 7,
  Z_PORT_FLAG2_URI_PARAM_v1 = 1 << 8,
  Z_PORT_FLAG2_SEQUENCE_v1 = 1 << 9,
  Z_PORT_FLAG2_SUPPORTS_MIDI_v1 = 1 << 10,
  Z_PORT_FLAG2_TP_OUTPUT_GAIN_v1 = 1 << 11,
  Z_PORT_FLAG2_MIDI_PITCH_BEND_v1 = 1 << 12,
  Z_PORT_FLAG2_MIDI_POLY_KEY_PRESSURE_v1 = 1 << 13,
  Z_PORT_FLAG2_MIDI_CHANNEL_PRESSURE_v1 = 1 << 14,
  Z_PORT_FLAG2_CHANNEL_SEND_ENABLED_v1 = 1 << 15,
  Z_PORT_FLAG2_CHANNEL_SEND_AMOUNT_v1 = 1 << 16,
  Z_PORT_FLAG2_BEATS_PER_BAR_v1 = 1 << 17,
  Z_PORT_FLAG2_BEAT_UNIT_v1 = 1 << 18,
  Z_PORT_FLAG2_FADER_SOLO_v1 = 1 << 19,
  Z_PORT_FLAG2_FADER_LISTEN_v1 = 1 << 20,
  Z_PORT_FLAG2_FADER_MONO_COMPAT_v1 = 1 << 21,
  Z_PORT_FLAG2_TRACK_RECORDING_v1 = 1 << 22,
  Z_PORT_FLAG2_TP_MONITOR_AUDIO_v1 = 1 << 23,
  Z_PORT_FLAG2_PREFADER_v1 = 1 << 24,
  Z_PORT_FLAG2_POSTFADER_v1 = 1 << 25,
  Z_PORT_FLAG2_MONITOR_FADER_v1 = 1 << 26,
  Z_PORT_FLAG2_SAMPLE_PROCESSOR_FADER_v1 = 1 << 27,
  Z_PORT_FLAG2_SAMPLE_PROCESSOR_TRACK_v1 = 1 << 28,
  Z_PORT_FLAG2_FADER_SWAP_PHASE_v1 = 1 << 29,
} ZPortFlags2_v1;

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
  { .name = "reports_latency",     .offset = 13, .bits = 1},
  { .name = "not_on_gui",          .offset = 14, .bits = 1},
  { .name = "plugin_enabled",      .offset = 15, .bits = 1},
  { .name = "plugin_control",      .offset = 16, .bits = 1},
  { .name = "fader_mute",          .offset = 17, .bits = 1},
  { .name = "channel_fader",       .offset = 18, .bits = 1},
  { .name = "automatable",         .offset = 19, .bits = 1},
  { .name = "midi_automatable",    .offset = 20, .bits = 1},
  { .name = "send_receivable",     .offset = 21, .bits = 1},
  { .name = "bpm",                 .offset = 22, .bits = 1},
  { .name = "generic_plugin_port", .offset = 23, .bits = 1},
  { .name = "plugin_gain",         .offset = 24, .bits = 1},
  { .name = "tp_mono",             .offset = 25, .bits = 1},
  { .name = "tp_input_gain",       .offset = 26, .bits = 1},
  { .name = "hw",                  .offset = 27, .bits = 1},
  { .name = "modulator_macro",     .offset = 28, .bits = 1},
  { .name = "logarithmic",         .offset = 29, .bits = 1},
  { .name = "is_property",         .offset = 30, .bits = 1},
};

static const cyaml_bitdef_t port_flags2_bitvals_v1[] = {
  YAML_BITVAL ("transport_roll", 0),
  YAML_BITVAL ("transport_stop", 1),
  YAML_BITVAL ("transport_backward", 2),
  YAML_BITVAL ("transport_forward", 3),
  YAML_BITVAL ("transport_loop_toggle", 4),
  YAML_BITVAL ("transport_rec_toggle", 5),
  YAML_BITVAL ("patch_message", 6),
  YAML_BITVAL ("enumeration", 7),
  YAML_BITVAL ("uri_param", 8),
  YAML_BITVAL ("sequence", 9),
  YAML_BITVAL ("supports_midi", 10),
  YAML_BITVAL ("output_gain", 11),
  YAML_BITVAL ("pitch_bend", 12),
  YAML_BITVAL ("poly_key_pressure", 13),
  YAML_BITVAL ("channel_pressure", 14),
  YAML_BITVAL ("ch_send_enabled", 15),
  YAML_BITVAL ("ch_send_amount", 16),
  YAML_BITVAL ("beats_per_bar", 17),
  YAML_BITVAL ("beat_unit", 18),
  YAML_BITVAL ("fader_solo", 19),
  YAML_BITVAL ("fader_listen", 20),
  YAML_BITVAL ("fader_mono_compat", 21),
  YAML_BITVAL ("track_recording", 22),
  YAML_BITVAL ("tp_monitor_audio", 23),
  YAML_BITVAL ("prefader", 24),
  YAML_BITVAL ("postfader", 25),
  YAML_BITVAL ("monitor_fader", 26),
  YAML_BITVAL ("sample_processor_fader", 27),
  YAML_BITVAL ("sample_processor_track", 28),
  YAML_BITVAL ("fader_swap_phase", 29),
  YAML_BITVAL ("midi_clock", 30),
};

typedef struct PortIdentifier_v1
{
  int                 schema_version;
  char *              label;
  char *              sym;
  char *              uri;
  char *              comment;
  ZPortOwnerType_v1   owner_type;
  PortType_v1         type;
  PortFlow_v1         flow;
  ZPortFlags_v1       flags;
  ZPortFlags2_v1      flags2;
  PortUnit_v1         unit;
  PluginIdentifier_v1 plugin_id;
  char *              port_group;
  char *              ext_port_id;
  unsigned int        track_name_hash;
  int                 port_index;
} PortIdentifier_v1;

static const cyaml_schema_field_t port_identifier_fields_schema_v1[] = {
  YAML_FIELD_INT (PortIdentifier_v1, schema_version),
  YAML_FIELD_STRING_PTR_OPTIONAL (PortIdentifier_v1, label),
  YAML_FIELD_STRING_PTR_OPTIONAL (PortIdentifier_v1, sym),
  YAML_FIELD_STRING_PTR_OPTIONAL (PortIdentifier_v1, uri),
  YAML_FIELD_STRING_PTR_OPTIONAL (PortIdentifier_v1, comment),
  YAML_FIELD_ENUM (PortIdentifier_v1, owner_type, port_owner_type_strings_v1),
  YAML_FIELD_ENUM (PortIdentifier_v1, type, port_type_strings_v1),
  YAML_FIELD_ENUM (PortIdentifier_v1, flow, port_flow_strings_v1),
  YAML_FIELD_ENUM (PortIdentifier_v1, unit, port_unit_strings_v1),
  YAML_FIELD_BITFIELD (PortIdentifier_v1, flags, port_flags_bitvals_v1),
  YAML_FIELD_BITFIELD (PortIdentifier_v1, flags2, port_flags2_bitvals_v1),
  YAML_FIELD_UINT (PortIdentifier_v1, track_name_hash),
  YAML_FIELD_MAPPING_EMBEDDED (
    PortIdentifier_v1,
    plugin_id,
    plugin_identifier_fields_schema_v1),
  YAML_FIELD_STRING_PTR_OPTIONAL (PortIdentifier_v1, port_group),
  YAML_FIELD_STRING_PTR_OPTIONAL (PortIdentifier_v1, ext_port_id),
  YAML_FIELD_INT (PortIdentifier_v1, port_index),

  CYAML_FIELD_END,
};

static const cyaml_schema_value_t port_identifier_schema_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    PortIdentifier_v1,
    port_identifier_fields_schema_v1),
};

static const cyaml_schema_value_t port_identifier_schema_default_v1 = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_DEFAULT,
    PortIdentifier_v1,
    port_identifier_fields_schema_v1),
};

#endif
