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
#include "utils/types.h"

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
enum class ZPortFlow
{
  Z_PORT_FLOW_UNKNOWN,
  Z_PORT_FLOW_INPUT,
  Z_PORT_FLOW_OUTPUT
};

/**
 * Type of signals the Port handles.
 */
enum class ZPortType
{
  Z_PORT_TYPE_UNKNOWN,
  Z_PORT_TYPE_CONTROL,
  Z_PORT_TYPE_AUDIO,
  Z_PORT_TYPE_EVENT,
  Z_PORT_TYPE_CV
};

/**
 * Port unit to be displayed in the UI.
 */
enum class ZPortUnit
{
  Z_PORT_UNIT_NONE,
  Z_PORT_UNIT_HZ,
  Z_PORT_UNIT_MHZ,
  Z_PORT_UNIT_DB,
  Z_PORT_UNIT_DEGREES,
  Z_PORT_UNIT_SECONDS,

  /** Milliseconds. */
  Z_PORT_UNIT_MS,

  /** Microseconds. */
  Z_PORT_UNIT_US,
};

const char *
port_unit_to_str (const ZPortUnit unit);

/**
 * Type of owner.
 */
enum class ZPortOwnerType
{
  /* Z_PORT_OWNER_TYPE_NONE, */
  Z_PORT_OWNER_TYPE_AUDIO_ENGINE,

  /** Plugin owner. */
  Z_PORT_OWNER_TYPE_PLUGIN,

  /** Track owner. */
  Z_PORT_OWNER_TYPE_TRACK,

  /** Channel owner. */
  Z_PORT_OWNER_TYPE_CHANNEL,

  /** Fader. */
  Z_PORT_OWNER_TYPE_FADER,

  /**
   * Channel send.
   *
   * PortIdentifier.port_index will contain the
   * send index on the port's track's channel.
   */
  Z_PORT_OWNER_TYPE_CHANNEL_SEND,

  /* TrackProcessor. */
  Z_PORT_OWNER_TYPE_TRACK_PROCESSOR,

  /** Port is part of a HardwareProcessor. */
  Z_PORT_OWNER_TYPE_HW,

  /** Port is owned by engine transport. */
  Z_PORT_OWNER_TYPE_TRANSPORT,

  /** Modulator macro processor owner. */
  Z_PORT_OWNER_TYPE_MODULATOR_MACRO_PROCESSOR,
};

/**
 * Port flags.
 */
enum class ZPortFlags
{
  Z_PORT_FLAG_STEREO_L = 1 << 0,
  Z_PORT_FLAG_STEREO_R = 1 << 1,
  Z_PORT_FLAG_PIANO_ROLL = 1 << 2,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#sideChainOf. */
  Z_PORT_FLAG_SIDECHAIN = 1 << 3,
  /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainInput
   * and http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainOutput. */
  Z_PORT_FLAG_MAIN_PORT = 1 << 4,
  Z_PORT_FLAG_MANUAL_PRESS = 1 << 5,

  /** Amplitude port. */
  Z_PORT_FLAG_AMPLITUDE = 1 << 6,

  /**
   * Port controls the stereo balance.
   *
   * This is used in channels for the balance
   * control.
   */
  Z_PORT_FLAG_STEREO_BALANCE = 1 << 7,

  /**
   * Whether the port wants to receive position
   * events.
   *
   * This is only applicable for LV2 Atom ports.
   */
  Z_PORT_FLAG_WANT_POSITION = 1 << 8,

  /**
   * Trigger ports will be set to 0 at the end of
   * each cycle.
   *
   * This mostly applies to LV2 Control Input
   * ports.
   */
  Z_PORT_FLAG_TRIGGER = 1 << 9,

  /** Whether the port is a toggle (on/off). */
  Z_PORT_FLAG_TOGGLE = 1 << 10,

  /** Whether the port is an integer. */
  Z_PORT_FLAG_INTEGER = 1 << 11,

  /** Whether port is for letting the plugin know
   * that we are in freewheeling (export) mode. */
  Z_PORT_FLAG_FREEWHEEL = 1 << 12,

  /** Used for plugin ports. */
  Z_PORT_FLAG_REPORTS_LATENCY = 1 << 13,

  /** Port should not be visible to users. */
  Z_PORT_FLAG_NOT_ON_GUI = 1 << 14,

  /** Port is a switch for plugin enabled. */
  Z_PORT_FLAG_PLUGIN_ENABLED = 1 << 15,

  /** Port is a plugin control. */
  Z_PORT_FLAG_PLUGIN_CONTROL = 1 << 16,

  /** Port is for fader mute. */
  Z_PORT_FLAG_FADER_MUTE = 1 << 17,

  /** Port is for channel fader. */
  Z_PORT_FLAG_CHANNEL_FADER = 1 << 18,

  /**
   * Port has an automation track.
   *
   * If this is set, it is assumed that the automation track at
   * \ref PortIdentifier.port_index is for this port.
   */
  Z_PORT_FLAG_AUTOMATABLE = 1 << 19,

  /** MIDI automatable control, such as modwheel or
   * pitch bend. */
  Z_PORT_FLAG_MIDI_AUTOMATABLE = 1 << 20,

  /** Channels can send to this port (ie, this port
   * is a track processor midi/stereo in or a plugin
   * sidechain in). */
  Z_PORT_FLAG_SEND_RECEIVABLE = 1 << 21,

  /** This is a BPM port. */
  Z_PORT_FLAG_BPM = 1 << 22,

  /**
   * Generic plugin port not belonging to the
   * underlying plugin.
   *
   * This is for ports that are added by Zrythm
   * such as Enabled and Gain.
   */
  Z_PORT_FLAG_GENERIC_PLUGIN_PORT = 1 << 23,

  /** This is the plugin gain. */
  Z_PORT_FLAG_PLUGIN_GAIN = 1 << 24,

  /** Track processor input mono switch. */
  Z_PORT_FLAG_TP_MONO = 1 << 25,

  /** Track processor input gain. */
  Z_PORT_FLAG_TP_INPUT_GAIN = 1 << 26,

  /** Port is a hardware port. */
  Z_PORT_FLAG_HW = 1 << 27,

  /**
   * Port is part of a modulator macro processor.
   *
   * Which of the ports it is can be determined
   * by checking flow/type.
   */
  Z_PORT_FLAG_MODULATOR_MACRO = 1 << 28,

  /** Logarithmic. */
  Z_PORT_FLAG_LOGARITHMIC = 1 << 29,

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
  Z_PORT_FLAG_IS_PROPERTY = 1 << 30,
};
ENUM_ENABLE_BITSET (ZPortFlags);

enum class ZPortFlags2
{
  /** Transport ports. */
  Z_PORT_FLAG2_TRANSPORT_ROLL = 1 << 0,
  Z_PORT_FLAG2_TRANSPORT_STOP = 1 << 1,
  Z_PORT_FLAG2_TRANSPORT_BACKWARD = 1 << 2,
  Z_PORT_FLAG2_TRANSPORT_FORWARD = 1 << 3,
  Z_PORT_FLAG2_TRANSPORT_LOOP_TOGGLE = 1 << 4,
  Z_PORT_FLAG2_TRANSPORT_REC_TOGGLE = 1 << 5,

  /** LV2 control atom port supports patch
   * messages. */
  Z_PORT_FLAG2_SUPPORTS_PATCH_MESSAGE = 1 << 6,

  /** Port's only reasonable values are its scale
   * points. */
  Z_PORT_FLAG2_ENUMERATION = 1 << 7,

  /** Parameter port's value type is URI. */
  Z_PORT_FLAG2_URI_PARAM = 1 << 8,

  /** Atom port buffer type is sequence. */
  Z_PORT_FLAG2_SEQUENCE = 1 << 9,

  /** Atom or event port supports MIDI. */
  Z_PORT_FLAG2_SUPPORTS_MIDI = 1 << 10,

  /** Track processor output gain. */
  Z_PORT_FLAG2_TP_OUTPUT_GAIN = 1 << 11,

  /** MIDI pitch bend. */
  Z_PORT_FLAG2_MIDI_PITCH_BEND = 1 << 12,

  /** MIDI poly key pressure. */
  Z_PORT_FLAG2_MIDI_POLY_KEY_PRESSURE = 1 << 13,

  /** MIDI channel pressure. */
  Z_PORT_FLAG2_MIDI_CHANNEL_PRESSURE = 1 << 14,

  /** Channel send enabled. */
  Z_PORT_FLAG2_CHANNEL_SEND_ENABLED = 1 << 15,

  /** Channel send amount. */
  Z_PORT_FLAG2_CHANNEL_SEND_AMOUNT = 1 << 16,

  /** Beats per bar. */
  Z_PORT_FLAG2_BEATS_PER_BAR = 1 << 17,

  /** Beat unit. */
  Z_PORT_FLAG2_BEAT_UNIT = 1 << 18,

  /** Fader solo. */
  Z_PORT_FLAG2_FADER_SOLO = 1 << 19,

  /** Fader listen. */
  Z_PORT_FLAG2_FADER_LISTEN = 1 << 20,

  /** Fader mono compat. */
  Z_PORT_FLAG2_FADER_MONO_COMPAT = 1 << 21,

  /** Track recording. */
  Z_PORT_FLAG2_TRACK_RECORDING = 1 << 22,

  /** Track processor monitor audio. */
  Z_PORT_FLAG2_TP_MONITOR_AUDIO = 1 << 23,

  /** Port is owned by prefader. */
  Z_PORT_FLAG2_PREFADER = 1 << 24,

  /** Port is owned by postfader. */
  Z_PORT_FLAG2_POSTFADER = 1 << 25,

  /** Port is owned by monitor fader. */
  Z_PORT_FLAG2_MONITOR_FADER = 1 << 26,

  /** Port is owned by the sample processor fader. */
  Z_PORT_FLAG2_SAMPLE_PROCESSOR_FADER = 1 << 27,

  /** Port is owned by sample processor
   * track/channel (including faders owned by those
   * tracks/channels). */
  Z_PORT_FLAG2_SAMPLE_PROCESSOR_TRACK = 1 << 28,

  /** Fader swap phase. */
  Z_PORT_FLAG2_FADER_SWAP_PHASE = 1 << 29,

  /** MIDI clock. */
  Z_PORT_FLAG2_MIDI_CLOCK = 1 << 30,
};
ENUM_ENABLE_BITSET (ZPortFlags2);

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
  ZPortOwnerType owner_type;
  /** Data type (e.g. AUDIO). */
  ZPortType type;
  /** Flow (IN/OUT). */
  ZPortFlow flow;
  /** Flags (e.g. is side chain). */
  ZPortFlags  flags;
  ZPortFlags2 flags2;

  /** Port unit. */
  ZPortUnit unit;

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
    static_cast<int> (self->flags2 & ZPortFlags2::Z_PORT_FLAG2_MIDI_PITCH_BEND)
      != 0
    || static_cast<int> (
         self->flags2 & ZPortFlags2::Z_PORT_FLAG2_MIDI_POLY_KEY_PRESSURE)
         != 0
    || static_cast<int> (
         self->flags2 & ZPortFlags2::Z_PORT_FLAG2_MIDI_CHANNEL_PRESSURE)
         != 0)
    {
      return self->port_index + 1;
    }
  else if (
    static_cast<int> (self->flags & ZPortFlags::Z_PORT_FLAG_MIDI_AUTOMATABLE)
    != 0)
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
