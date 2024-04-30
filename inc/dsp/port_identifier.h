// clang-format off
// SPDX-FileCopyrightText: © 2018-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Port identifier.
 */

#ifndef __AUDIO_PORT_IDENTIFIER_H__
#define __AUDIO_PORT_IDENTIFIER_H__

#include "zrythm-config.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "plugins/plugin_identifier.h"
#include "utils/string.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

#define PORT_IDENTIFIER_SCHEMA_VERSION 1

#define PORT_IDENTIFIER_MAGIC 3411841
#define IS_PORT_IDENTIFIER(tr) \
  (tr && ((PortIdentifier *) tr)->magic == PORT_IDENTIFIER_MAGIC)

/**
 * Direction of the signal.
 */
typedef enum PortFlow
{
  FLOW_UNKNOWN,
  FLOW_INPUT,
  FLOW_OUTPUT
} PortFlow;

static const char * port_flow_strings[] = {
  "unknown",
  "input",
  "output",
};

/**
 * Type of signals the Port handles.
 */
typedef enum PortType
{
  TYPE_UNKNOWN,
  TYPE_CONTROL,
  TYPE_AUDIO,
  TYPE_EVENT,
  TYPE_CV
} PortType;

static const char * port_type_strings[] = {
  "unknown", "control", "audio", "event", "cv",
};

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

  /** Microseconds. */
  PORT_UNIT_US,
} PortUnit;

static const char * port_unit_strings[] = {
  "none", "Hz", "MHz", "dB", "°", "s", "ms", "μs",
};

/**
 * Type of owner.
 */
typedef enum PortOwnerType
{
  /* PORT_OWNER_TYPE_NONE, */
  PORT_OWNER_TYPE_AUDIO_ENGINE,

  /** Plugin owner. */
  PORT_OWNER_TYPE_PLUGIN,

  /** Track owner. */
  PORT_OWNER_TYPE_TRACK,

  /** Channel owner. */
  PORT_OWNER_TYPE_CHANNEL,

  /** Fader. */
  PORT_OWNER_TYPE_FADER,

  /**
   * Channel send.
   *
   * PortIdentifier.port_index will contain the
   * send index on the port's track's channel.
   */
  PORT_OWNER_TYPE_CHANNEL_SEND,

  /* TrackProcessor. */
  PORT_OWNER_TYPE_TRACK_PROCESSOR,

  /** Port is part of a HardwareProcessor. */
  PORT_OWNER_TYPE_HW,

  /** Port is owned by engine transport. */
  PORT_OWNER_TYPE_TRANSPORT,

  /** Modulator macro processor owner. */
  PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR,
} PortOwnerType;

static const char * port_owner_type_strings[] = {
  "audio engine",    "plugin",
  "track",           "channel",
  "fader",           "channel send",
  "track processor", "hw",
  "transport",       "modulator macro processor",
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
   * If this is set, it is assumed that the automation track at
   * \ref PortIdentifier.port_index is for this port.
   */
  PORT_FLAG_AUTOMATABLE = 1 << 19,

  /** MIDI automatable control, such as modwheel or
   * pitch bend. */
  PORT_FLAG_MIDI_AUTOMATABLE = 1 << 20,

  /** Channels can send to this port (ie, this port
   * is a track processor midi/stereo in or a plugin
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
   * @see http://lv2plug.in/ns/lv2core#Parameter. */
  PORT_FLAG_IS_PROPERTY = 1 << 30,
} PortFlags;

static const char * port_flags_bitvals[] = {
  "stereo_l",         "stereo_r",        "piano_roll",    "sidechain",
  "main_port",        "manual_press",    "amplitude",     "stereo_balance",
  "want_position",    "trigger",         "toggle",        "integer",
  "freewheel",        "reports_latency", "not_on_gui",    "plugin_enabled",
  "plugin_control",   "fader_mute",      "channel_fader", "automatable",
  "midi_automatable", "send_receivable", "bpm",           "generic_plugin_port",
  "plugin_gain",      "tp_mono",         "tp_input_gain", "hw",
  "modulator_macro",  "logarithmic",     "is_property",
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

  /** Port is owned by prefader. */
  PORT_FLAG2_PREFADER = 1 << 24,

  /** Port is owned by postfader. */
  PORT_FLAG2_POSTFADER = 1 << 25,

  /** Port is owned by monitor fader. */
  PORT_FLAG2_MONITOR_FADER = 1 << 26,

  /** Port is owned by the sample processor fader. */
  PORT_FLAG2_SAMPLE_PROCESSOR_FADER = 1 << 27,

  /** Port is owned by sample processor
   * track/channel (including faders owned by those
   * tracks/channels). */
  PORT_FLAG2_SAMPLE_PROCESSOR_TRACK = 1 << 28,

  /** Fader swap phase. */
  PORT_FLAG2_FADER_SWAP_PHASE = 1 << 29,

  /** MIDI clock. */
  PORT_FLAG2_MIDI_CLOCK = 1 << 30,
} PortFlags2;

static const char * port_flags2_bitvals[] = {
  "transport_roll",
  "transport_stop",
  "transport_backward",
  "transport_forward",
  "transport_loop_toggle",
  "transport_rec_toggle",
  "patch_message",
  "enumeration",
  "uri_param",
  "sequence",
  "supports_midi",
  "output_gain",
  "pitch_bend",
  "poly_key_pressure",
  "channel_pressure",
  "ch_send_enabled",
  "ch_send_amount",
  "beats_per_bar",
  "beat_unit",
  "fader_solo",
  "fader_listen",
  "fader_mono_compat",
  "track_recording",
  "tp_monitor_audio",
  "prefader",
  "postfader",
  "monitor_fader",
  "sample_processor_fader",
  "sample_processor_track",
  "fader_swap_phase",
  "midi_clock",
};

/**
 * Struct used to identify Ports in the project.
 *
 * This should include some members of the original struct
 * enough to identify the port. To be used for sources and
 * dests.
 *
 * This must be filled in before saving and read from while
 * loading to fill in the srcs/dests.
 */
typedef struct PortIdentifier
{
  int schema_version;

  /** Human readable label. */
  char * label;

  /** Unique symbol. */
  char * sym;

  /** URI, if LV2 property. */
  char * uri;

  /** Comment, if any. */
  char * comment;

  /** Owner type. */
  PortOwnerType owner_type;
  /** Data type (e.g. AUDIO). */
  PortType type;
  /** Flow (IN/OUT). */
  PortFlow flow;
  /** Flags (e.g. is side chain). */
  PortFlags  flags;
  PortFlags2 flags2;

  /** Port unit. */
  PortUnit unit;

  /** Identifier of plugin. */
  PluginIdentifier plugin_id;

  /** Port group this port is part of (only
   * applicable for LV2 plugin ports). */
  char * port_group;

  /** ExtPort ID (type + full name), if hw port. */
  char * ext_port_id;

  /** Track name hash (0 for non-track ports). */
  unsigned int track_name_hash;

  /** Index (e.g. in plugin's output ports). */
  int port_index;
} PortIdentifier;

void
port_identifier_init (PortIdentifier * self);

static inline const char *
port_identifier_get_label (PortIdentifier * self)
{
  return self->label;
}

/**
 * Port group comparator function where @ref p1 and
 * @ref p2 are pointers to Port.
 */
int
port_identifier_port_group_cmp (const void * p1, const void * p2);

/**
 * Returns the MIDI channel for a MIDI CC port, or -1 if not a MIDI CC port.
 *
 * @note MIDI channels start from 1 (not 0).
 */
static inline int
port_identifier_get_midi_channel (const PortIdentifier * self)
{
  if (
    self->flags2 & PORT_FLAG2_MIDI_PITCH_BEND
    || self->flags2 & PORT_FLAG2_MIDI_POLY_KEY_PRESSURE
    || self->flags2 & PORT_FLAG2_MIDI_CHANNEL_PRESSURE)
    {
      return self->port_index + 1;
    }
  else if (self->flags & PORT_FLAG_MIDI_AUTOMATABLE)
    {
      return self->port_index / 128 + 1;
    }
  return -1;
}

/**
 * Copy the identifier content from \ref src to
 * \ref dest.
 *
 * @note This frees/allocates memory on \ref dest.
 */
NONNULL void
port_identifier_copy (PortIdentifier * dest, const PortIdentifier * src);

/**
 * Returns if the 2 PortIdentifier's are equal.
 *
 * @note Does not check insignificant data like
 *   comment.
 */
WARN_UNUSED_RESULT NONNULL bool
port_identifier_is_equal (
  const PortIdentifier * src,
  const PortIdentifier * dest);

/**
 * To be used as GEqualFunc.
 */
int
port_identifier_is_equal_func (const void * a, const void * b);

NONNULL void
port_identifier_print_to_str (
  const PortIdentifier * self,
  char *                 buf,
  size_t                 buf_sz);

NONNULL void
port_identifier_print (const PortIdentifier * self);

NONNULL bool
port_identifier_validate (PortIdentifier * self);

NONNULL uint32_t
port_identifier_get_hash (const void * self);

NONNULL PortIdentifier *
port_identifier_clone (const PortIdentifier * src);

NONNULL void
port_identifier_free_members (PortIdentifier * self);

NONNULL void
port_identifier_free (PortIdentifier * self);

/**
 * Compatible with GDestroyNotify.
 */
void
port_identifier_free_func (void * self);

/**
 * @}
 */

#endif
