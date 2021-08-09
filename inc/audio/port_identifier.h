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

#ifndef __AUDIO_PORT_IDENTIFIER_H__
#define __AUDIO_PORT_IDENTIFIER_H__

#include "zrythm-config.h"

#include <stdbool.h>

#include "plugins/plugin_identifier.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define PORT_IDENTIFIER_SCHEMA_VERSION 1

#define PORT_IDENTIFIER_MAGIC 3411841
#define IS_PORT_IDENTIFIER(tr) \
  (tr && \
   ((PortIdentifier *) tr)->magic == \
     PORT_IDENTIFIER_MAGIC)

/**
 * Direction of the signal.
 */
typedef enum PortFlow {
  FLOW_UNKNOWN,
  FLOW_INPUT,
  FLOW_OUTPUT
} PortFlow;

/**
 * Type of signals the Port handles.
 */
typedef enum PortType {
  TYPE_UNKNOWN,
  TYPE_CONTROL,
  TYPE_AUDIO,
  TYPE_EVENT,
  TYPE_CV
} PortType;

/**
 * Port unit to be displayed in the UI.
 */
typedef enum PortUnit
{
  PORT_UNIT_NONE,
  PORT_UNIT_HZ,
  PORT_UNIT_MHZ,
  PORT_UNIT_DB,
  PORT_UNIT_DEGREES,
  PORT_UNIT_SECONDS,

  /** Milliseconds. */
  PORT_UNIT_MS,
} PortUnit;

static const cyaml_strval_t
port_unit_strings[] =
{
  { "none", PORT_UNIT_NONE     },
  { "Hz",   PORT_UNIT_HZ       },
  { "MHz",  PORT_UNIT_MHZ      },
  { "dB",   PORT_UNIT_DB       },
  { "°",    PORT_UNIT_DEGREES  },
  { "s",    PORT_UNIT_SECONDS  },
  { "ms",   PORT_UNIT_MS       },
};

/**
 * Type of owner.
 */
typedef enum PortOwnerType
{
  //PORT_OWNER_TYPE_NONE,
  PORT_OWNER_TYPE_BACKEND,
  PORT_OWNER_TYPE_PLUGIN,
  PORT_OWNER_TYPE_TRACK,

  /* track fader */
  PORT_OWNER_TYPE_FADER,

  /**
   * Channel send.
   *
   * PortIdentifier.port_index will contain the
   * send index on the port's track's channel.
   */
  PORT_OWNER_TYPE_CHANNEL_SEND,

  /* track prefader */
  PORT_OWNER_TYPE_PREFADER,

  /* monitor fader */
  PORT_OWNER_TYPE_MONITOR_FADER,

  /* TrackProcessor. */
  PORT_OWNER_TYPE_TRACK_PROCESSOR,

  PORT_OWNER_TYPE_SAMPLE_PROCESSOR,

  /** Port is part of a HardwareProcessor. */
  PORT_OWNER_TYPE_HW,

  /** Port is owned by engine transport. */
  PORT_OWNER_TYPE_TRANSPORT,
} PortOwnerType;

static const cyaml_strval_t
port_owner_type_strings[] =
{
  { "backend",   PORT_OWNER_TYPE_BACKEND    },
  { "plugin",    PORT_OWNER_TYPE_PLUGIN   },
  { "track",     PORT_OWNER_TYPE_TRACK   },
  { "pre-fader", PORT_OWNER_TYPE_PREFADER   },
  { "fader",     PORT_OWNER_TYPE_FADER   },
  { "channel send", PORT_OWNER_TYPE_CHANNEL_SEND  },
  { "track processor",
    PORT_OWNER_TYPE_TRACK_PROCESSOR   },
  { "monitor fader",
    PORT_OWNER_TYPE_MONITOR_FADER },
  { "sample processor",
    PORT_OWNER_TYPE_SAMPLE_PROCESSOR },
  { "hw",        PORT_OWNER_TYPE_HW },
  { "transport", PORT_OWNER_TYPE_TRANSPORT },
};

/**
 * Port flags.
 */
typedef enum PortFlags
{
  PORT_FLAG_STEREO_L = 1 << 0,
  PORT_FLAG_STEREO_R = 1 << 1,
  PORT_FLAG_PIANO_ROLL = 1 << 2,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#sideChainOf. */
  PORT_FLAG_SIDECHAIN = 1 << 3,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainInput
   * and http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainOutput. */
  PORT_FLAG_MAIN_PORT = 1 << 4,
  PORT_FLAG_MANUAL_PRESS = 1 << 5,

  /** Amplitude port. */
  PORT_FLAG_AMPLITUDE = 1 << 6,

  /**
   * Port controls the stereo balance.
   *
   * This is used in channels for the balance
   * control.
   */
  PORT_FLAG_STEREO_BALANCE = 1 << 7,

  /**
   * Whether the port wants to receive position
   * events.
   *
   * This is only applicable for LV2 Atom ports.
   */
  PORT_FLAG_WANT_POSITION = 1 << 8,

  /**
   * Trigger ports will be set to 0 at the end of
   * each cycle.
   *
   * This mostly applies to LV2 Control Input
   * ports.
   */
  PORT_FLAG_TRIGGER = 1 << 9,

  /** Whether the port is a toggle (on/off). */
  PORT_FLAG_TOGGLE = 1 << 10,

  /** Whether the port is an integer. */
  PORT_FLAG_INTEGER = 1 << 11,

  /** Whether port is for letting the plugin know
   * that we are in freewheeling (export) mode. */
  PORT_FLAG_FREEWHEEL = 1 << 12,

  /** Used for plugin ports. */
  PORT_FLAG_REPORTS_LATENCY = 1 << 13,

  /** Port should not be visible to users. */
  PORT_FLAG_NOT_ON_GUI = 1 << 14,

  /** Port is a switch for plugin enabled. */
  PORT_FLAG_PLUGIN_ENABLED = 1 << 15,

  /** Port is a plugin control. */
  PORT_FLAG_PLUGIN_CONTROL = 1 << 16,

  /** Port is for fader mute. */
  PORT_FLAG_FADER_MUTE = 1 << 17,

  /** Port is for channel fader. */
  PORT_FLAG_CHANNEL_FADER = 1 << 18,

  /**
   * Port has an automation track.
   *
   * If this is set, it is assumed that the
   * automation track at
   * \ref PortIdentifier.port_index is for this
   * port.
   */
  PORT_FLAG_AUTOMATABLE = 1 << 19,

  /** MIDI automatable control, such as modwheel or
   * pitch bend. */
  PORT_FLAG_MIDI_AUTOMATABLE = 1 << 20,

  /** Channels can send to this port (ie, this port
   * is a track procesor midi/stereo in or a plugin
   * sidechain in). */
  PORT_FLAG_SEND_RECEIVABLE = 1 << 21,

  /** This is a BPM port. */
  PORT_FLAG_BPM = 1 << 22,

  /**
   * Generic plugin port not belonging to the
   * underlying plugin.
   *
   * This is for ports that are added by Zrythm
   * such as Enabled and Gain.
   */
  PORT_FLAG_GENERIC_PLUGIN_PORT = 1 << 23,

  /** This is the plugin gain. */
  PORT_FLAG_PLUGIN_GAIN = 1 << 24,

  /** Track processor input mono switch. */
  PORT_FLAG_TP_MONO = 1 << 25,

  /** Track processor input gain. */
  PORT_FLAG_TP_INPUT_GAIN = 1 << 26,

  /** Port is a hardware port. */
  PORT_FLAG_HW = 1 << 27,

  /**
   * Port is part of a modulator macro processor.
   *
   * Which of the ports it is can be determined
   * by checking flow/type.
   */
  PORT_FLAG_MODULATOR_MACRO = 1 << 28,

  /** Logarithmic. */
  PORT_FLAG_LOGARITHMIC = 1 << 29,

  /**
   * Plugin control is a property (changes are set
   * via atom message on the plugin's control port),
   * as opposed to conventional float control ports.
   *
   * An input Port is created for each parameter
   * declared as either writable or readable (or
   * both).
   *
   * @seealso http://lv2plug.in/ns/lv2core#Parameter. */
  PORT_FLAG_IS_PROPERTY = 1 << 30,
} PortFlags;

static const cyaml_bitdef_t
port_flags_bitvals[] =
{
  { .name = "stereo_l", .offset =  0, .bits =  1 },
  { .name = "stereo_r", .offset =  1, .bits =  1 },
  { .name = "piano_roll", .offset = 2, .bits = 1 },
  { .name = "sidechain", .offset = 3, .bits =  1 },
  { .name = "main_port", .offset = 4, .bits = 1 },
  { .name = "manual_press", .offset = 5, .bits = 1 },
  { .name = "amplitude", .offset = 6, .bits = 1 },
  { .name = "stereo_balance", .offset = 7, .bits = 1 },
  { .name = "want_position", .offset = 8, .bits = 1 },
  { .name = "trigger", .offset = 9, .bits = 1 },
  { .name = "toggle", .offset = 10, .bits = 1 },
  { .name = "integer", .offset = 11, .bits = 1 },
  { .name = "freewheel", .offset = 12, .bits = 1 },
  { .name = "reports_latency", .offset = 13, .bits = 1 },
  { .name = "not_on_gui", .offset = 14, .bits = 1 },
  { .name = "plugin_enabled", .offset = 15, .bits = 1 },
  { .name = "plugin_control", .offset = 16, .bits = 1 },
  { .name = "fader_mute", .offset = 17, .bits = 1 },
  { .name = "channel_fader", .offset = 18, .bits = 1 },
  { .name = "automatable", .offset = 19, .bits = 1 },
  { .name = "midi_automatable", .offset = 20, .bits = 1 },
  { .name = "send_receivable", .offset = 21, .bits = 1 },
  { .name = "bpm", .offset = 22, .bits = 1 },
  { .name = "generic_plugin_port", .offset = 23, .bits = 1 },
  { .name = "plugin_gain", .offset = 24, .bits = 1 },
  { .name = "tp_mono", .offset = 25, .bits = 1 },
  { .name = "tp_input_gain", .offset = 26, .bits = 1 },
  { .name = "hw", .offset = 27, .bits = 1 },
  { .name = "modulator_macro", .offset = 28, .bits = 1 },
  { .name = "logarithmic", .offset = 29, .bits = 1 },
  { .name = "is_property", .offset = 30, .bits = 1 },
};

typedef enum PortFlags2
{
  /** Transport ports. */
  PORT_FLAG2_TRANSPORT_ROLL = 1 << 0,
  PORT_FLAG2_TRANSPORT_STOP = 1 << 1,
  PORT_FLAG2_TRANSPORT_BACKWARD = 1 << 2,
  PORT_FLAG2_TRANSPORT_FORWARD = 1 << 3,
  PORT_FLAG2_TRANSPORT_LOOP_TOGGLE = 1 << 4,
  PORT_FLAG2_TRANSPORT_REC_TOGGLE = 1 << 5,

  /** LV2 control atom port supports patch
   * messages. */
  PORT_FLAG2_SUPPORTS_PATCH_MESSAGE = 1 << 6,

  /** Port's only reasonable values are its scale
   * points. */
  PORT_FLAG2_ENUMERATION = 1 << 7,

  /** Parameter port's value type is URI. */
  PORT_FLAG2_URI_PARAM = 1 << 8,

  /** Atom port buffer type is sequence. */
  PORT_FLAG2_SEQUENCE = 1 << 9,

  /** Atom or event port supports MIDI. */
  PORT_FLAG2_SUPPORTS_MIDI = 1 << 10,

  /** Track processor output gain. */
  PORT_FLAG2_TP_OUTPUT_GAIN = 1 << 11,

  /** MIDI pitch bend. */
  PORT_FLAG2_MIDI_PITCH_BEND = 1 << 12,

  /** MIDI poly key pressure. */
  PORT_FLAG2_MIDI_POLY_KEY_PRESSURE = 1 << 13,

  /** MIDI channel pressure. */
  PORT_FLAG2_MIDI_CHANNEL_PRESSURE = 1 << 14,

  /** Channel send enabled. */
  PORT_FLAG2_CHANNEL_SEND_ENABLED = 1 << 15,

  /** Channel send amount. */
  PORT_FLAG2_CHANNEL_SEND_AMOUNT = 1 << 16,

  /** Beats per bar. */
  PORT_FLAG2_BEATS_PER_BAR = 1 << 17,

  /** Beat unit. */
  PORT_FLAG2_BEAT_UNIT = 1 << 18,

  /** Fader solo. */
  PORT_FLAG2_FADER_SOLO = 1 << 19,

  /** Fader listen. */
  PORT_FLAG2_FADER_LISTEN = 1 << 20,

  /** Fader mono compat. */
  PORT_FLAG2_FADER_MONO_COMPAT = 1 << 21,

  /** Track recording. */
  PORT_FLAG2_TRACK_RECORDING = 1 << 22,

  /** Track processor monitor audio. */
  PORT_FLAG2_TP_MONITOR_AUDIO = 1 << 23,
} PortFlags2;

static const cyaml_bitdef_t
port_flags2_bitvals[] =
{
  { .name = "transport_roll", .offset = 0, .bits = 1 },
  { .name = "transport_stop", .offset = 1, .bits = 1 },
  { .name = "transport_backward", .offset = 2, .bits = 1 },
  { .name = "transport_forward", .offset = 3, .bits = 1 },
  { .name = "transport_loop_toggle", .offset = 4, .bits = 1 },
  { .name = "transport_rec_toggle", .offset = 5, .bits = 1 },
  { .name = "patch_message", .offset = 6, .bits = 1 },
  { .name = "enumeration", .offset = 7, .bits = 1 },
  { .name = "uri_param", .offset = 8, .bits = 1 },
  { .name = "sequence", .offset = 9, .bits = 1 },
  { .name = "supports_midi", .offset = 10, .bits = 1 },
  { .name = "output_gain", .offset = 11, .bits = 1 },
  { .name = "pitch_bend", .offset = 12, .bits = 1 },
  { .name = "poly_key_pressure", .offset = 13, .bits = 1 },
  { .name = "channel_pressure", .offset = 14, .bits = 1 },
  { .name = "ch_send_enabled", .offset = 15, .bits = 1 },
  { .name = "ch_send_amount", .offset = 16, .bits = 1 },
  { .name = "beats_per_bar", .offset = 17, .bits = 1 },
  { .name = "beat_unit", .offset = 18, .bits = 1 },
  { .name = "fader_solo", .offset = 19, .bits = 1 },
  { .name = "fader_listen", .offset = 20, .bits = 1 },
  { .name = "fader_mono_compat", .offset = 21, .bits = 1 },
  { .name = "track_recording", .offset = 22, .bits = 1 },
  { .name = "tp_monitor_audio", .offset = 23, .bits = 1 },
};

/**
 * Struct used to identify Ports in the project.
 *
 * This should include some members of the original
 * struct enough to identify the port. To be used
 * for sources and dests.
 *
 * This must be filled in before saving and read from
 * while loading to fill in the srcs/dests.
 */
typedef struct PortIdentifier
{
  int                 schema_version;

  /** Human readable label. */
  char *              label;

  /** Symbol, if LV2. */
  char *              sym;

  /** URI, if LV2 property. */
  char *              uri;

  /** Comment, if any. */
  char *              comment;

  /** Owner type. */
  PortOwnerType       owner_type;
  /** Data type (e.g. AUDIO). */
  PortType            type;
  /** Flow (IN/OUT). */
  PortFlow            flow;
  /** Flags (e.g. is side chain). */
  PortFlags           flags;
  PortFlags2          flags2;

  /** Port unit. */
  PortUnit            unit;

  /** Identifier of plugin. */
  PluginIdentifier    plugin_id;

  /** Port group this port is part of (only
   * applicable for LV2 plugin ports). */
  char *              port_group;

  /** ExtPort ID (type + full name), if hw port. */
  char *              ext_port_id;

  /** Index of Track in the Tracklist. */
  int                 track_pos;
  /** Index (e.g. in plugin's output ports). */
  int                 port_index;
} PortIdentifier;

static const cyaml_strval_t
port_flow_strings[] =
{
  { "unknown",       FLOW_UNKNOWN    },
  { "input",         FLOW_INPUT   },
  { "output",        FLOW_OUTPUT   },
};

static const cyaml_strval_t
port_type_strings[] =
{
  { "unknown",       TYPE_UNKNOWN    },
  { "control",       TYPE_CONTROL   },
  { "audio",         TYPE_AUDIO   },
  { "event",         TYPE_EVENT   },
  { "cv",            TYPE_CV   },
};

static const cyaml_schema_field_t
port_identifier_fields_schema[] =
{
  YAML_FIELD_INT (
    PortIdentifier, schema_version),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier, label),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier, sym),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier, uri),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier, comment),
  YAML_FIELD_ENUM (
    PortIdentifier, owner_type,
    port_owner_type_strings),
  YAML_FIELD_ENUM (
    PortIdentifier, type, port_type_strings),
  YAML_FIELD_ENUM (
    PortIdentifier, flow, port_flow_strings),
  YAML_FIELD_ENUM (
    PortIdentifier, unit, port_unit_strings),
  YAML_FIELD_BITFIELD (
    PortIdentifier, flags, port_flags_bitvals),
  YAML_FIELD_BITFIELD (
    PortIdentifier, flags2, port_flags2_bitvals),
  YAML_FIELD_INT (
    PortIdentifier, track_pos),
  YAML_FIELD_MAPPING_EMBEDDED (
    PortIdentifier, plugin_id,
    plugin_identifier_fields_schema),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier, port_group),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier, ext_port_id),
  YAML_FIELD_INT (
    PortIdentifier, port_index),

  CYAML_FIELD_END,
};

static const cyaml_schema_value_t
port_identifier_schema = {
  YAML_VALUE_PTR (
    PortIdentifier, port_identifier_fields_schema),
};

static const cyaml_schema_value_t
port_identifier_schema_default = {
  YAML_VALUE_DEFAULT (
    PortIdentifier, port_identifier_fields_schema),
};

void
port_identifier_init (
  PortIdentifier * self);

static inline const char *
port_identifier_get_label (
  PortIdentifier * self)
{
  return self->label;
}

/**
 * Port group comparator function where @ref p1 and
 * @ref p2 are pointers to Port.
 */
int
port_identifier_port_group_cmp (
  const void* p1, const void* p2);

/**
 * Copy the identifier content from \ref src to
 * \ref dest.
 *
 * @note This frees/allocates memory on \ref dest.
 */
NONNULL
void
port_identifier_copy (
  PortIdentifier * dest,
  PortIdentifier * src);

/**
 * Returns if the 2 PortIdentifier's are equal.
 */
WARN_UNUSED_RESULT
HOT
NONNULL
bool
port_identifier_is_equal (
  PortIdentifier * src,
  PortIdentifier * dest);

NONNULL
void
port_identifier_print (
  PortIdentifier * self);

NONNULL
bool
port_identifier_validate (
  PortIdentifier * self);

NONNULL
void
port_identifier_free_members (
  PortIdentifier * self);

/**
 * @}
 */

#endif
