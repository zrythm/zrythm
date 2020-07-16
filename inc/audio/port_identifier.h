/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#include "plugins/plugin_identifier.h"
#include "utils/string.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

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

  /* track prefader */
  PORT_OWNER_TYPE_PREFADER,

  /* monitor fader */
  PORT_OWNER_TYPE_MONITOR_FADER,

  /* TrackProcessor. */
  PORT_OWNER_TYPE_TRACK_PROCESSOR,

  PORT_OWNER_TYPE_SAMPLE_PROCESSOR,
} PortOwnerType;

static const cyaml_strval_t
port_owner_type_strings[] =
{
  { "backend",   PORT_OWNER_TYPE_BACKEND    },
  { "plugin",    PORT_OWNER_TYPE_PLUGIN   },
  { "track",     PORT_OWNER_TYPE_TRACK   },
  { "pre-fader", PORT_OWNER_TYPE_PREFADER   },
  { "fader",     PORT_OWNER_TYPE_FADER   },
  { "track processor",
    PORT_OWNER_TYPE_TRACK_PROCESSOR   },
  { "monitor fader",
    PORT_OWNER_TYPE_MONITOR_FADER },
  { "sample processor",
    PORT_OWNER_TYPE_SAMPLE_PROCESSOR },
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

  /** Port is for channel mute. */
  PORT_FLAG_CHANNEL_MUTE = 1 << 17,

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

  /** This is a time signature port. */
  PORT_FLAG_TIME_SIG = 1 << 23,
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
  { .name = "channel_mute", .offset = 17, .bits = 1 },
  { .name = "channel_fader", .offset = 18, .bits = 1 },
  { .name = "automatable", .offset = 19, .bits = 1 },
  { .name = "midi_automatable", .offset = 20, .bits = 1 },
  { .name = "send_receivable", .offset = 21, .bits = 1 },
  { .name = "bpm", .offset = 22, .bits = 1 },
  { .name = "time_sig", .offset = 23, .bits = 1 },
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
  /** Human readable label. */
  char *              label;
  /** Owner type. */
  PortOwnerType       owner_type;
  /** Data type (e.g. AUDIO). */
  PortType            type;
  /** Flow (IN/OUT). */
  PortFlow            flow;
  /** Flags (e.g. is side chain). */
  PortFlags           flags;

  /** Port unit. */
  PortUnit            unit;

  /** Identifier of plugin. */
  PluginIdentifier    plugin_id;

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
  YAML_FIELD_STRING_PTR_OPTIONAL (
    PortIdentifier, label),
  YAML_FIELD_ENUM (
    PortIdentifier, owner_type,
    port_owner_type_strings),
  YAML_FIELD_ENUM (
    PortIdentifier, type, port_type_strings),
  YAML_FIELD_ENUM (
    PortIdentifier, flow, port_flow_strings),
  YAML_FIELD_ENUM (
    PortIdentifier, unit, port_unit_strings),
  CYAML_FIELD_BITFIELD (
    "flags", CYAML_FLAG_DEFAULT,
    PortIdentifier, flags, port_flags_bitvals,
    CYAML_ARRAY_LEN (port_flags_bitvals)),
  YAML_FIELD_INT (
    PortIdentifier, track_pos),
  CYAML_FIELD_MAPPING (
    "plugin_id", CYAML_FLAG_DEFAULT,
    PortIdentifier, plugin_id,
    plugin_identifier_fields_schema),
  YAML_FIELD_INT (
    PortIdentifier, port_index),

  CYAML_FIELD_END,
};

static const cyaml_schema_value_t
port_identifier_schema = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    PortIdentifier, port_identifier_fields_schema),
};

static const cyaml_schema_value_t
port_identifier_schema_default = {
  CYAML_VALUE_MAPPING (
    CYAML_FLAG_DEFAULT,
    PortIdentifier, port_identifier_fields_schema),
};

static inline void
port_identifier_copy (
  PortIdentifier * dest,
  PortIdentifier * src)
{
  g_return_if_fail (dest && src);
  if (dest->label)
    g_free (dest->label);
  dest->label = g_strdup (src->label);
  dest->owner_type = src->owner_type;
  dest->type = src->type;
  dest->flow = src->flow;
  dest->flags = src->flags;
  plugin_identifier_copy (
    &dest->plugin_id, &src->plugin_id);
  dest->track_pos = src->track_pos;
  dest->port_index = src->port_index;
}

/**
 * Returns if the 2 PortIdentifier's are equal.
 */
static inline int
port_identifier_is_equal (
  PortIdentifier * src,
  PortIdentifier * dest)
{
  bool eq =
    string_is_equal (
      dest->label, src->label, 0) &&
    dest->owner_type == src->owner_type &&
    dest->type == src->type &&
    dest->flow == src->flow &&
    dest->flags == src->flags &&
    dest->track_pos == src->track_pos &&
    dest->port_index == src->port_index;
  if (dest->owner_type == PORT_OWNER_TYPE_PLUGIN)
    {
      eq =
        eq &&
        plugin_identifier_is_equal (
          &dest->plugin_id, &src->plugin_id);
    }

  return eq;
}

/**
 * @}
 */

#endif
