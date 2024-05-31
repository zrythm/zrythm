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

#include <cstddef>
#include <cstdint>

#include "plugins/plugin_identifier.h"
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
enum class PortFlow
{
  Unknown,
  Input,
  Output
};

/**
 * Type of signals the Port handles.
 */
enum class PortType
{
  Unknown,
  Control,
  Audio,
  Event,
  CV
};

/**
 * Port unit to be displayed in the UI.
 */
enum class PortUnit
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
port_unit_to_str (const PortUnit unit);

/**
 * Struct used to identify Ports in the project.
 *
 * This should include some members of the original struct enough to identify
 * the port. To be used for sources and dests.
 *
 * This must be filled in before saving and read from while loading to fill in
 * the srcs/dests.
 */
struct PortIdentifier
{
  /**
   * Type of owner.
   */
  enum class OwnerType
  {
    /* NONE, */
    PORT_OWNER_TYPE_AUDIO_ENGINE,

    /** Plugin owner. */
    PLUGIN,

    /** Track owner. */
    TRACK,

    /** Channel owner. */
    CHANNEL,

    /** Fader. */
    FADER,

    /**
     * Channel send.
     *
     * PortIdentifier.port_index will contain the
     * send index on the port's track's channel.
     */
    CHANNEL_SEND,

    /* TrackProcessor. */
    TRACK_PROCESSOR,

    /** Port is part of a HardwareProcessor. */
    HW,

    /** Port is owned by engine transport. */
    PORT_OWNER_TYPE_TRANSPORT,

    /** Modulator macro processor owner. */
    MODULATOR_MACRO_PROCESSOR,
  };

  /**
   * Port flags.
   */
  enum class Flags
  {
    STEREO_L = 1 << 0,
    STEREO_R = 1 << 1,
    PianoRoll = 1 << 2,
    /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#sideChainOf. */
    SIDECHAIN = 1 << 3,
    /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainInput
     * and http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainOutput. */
    MAIN_PORT = 1 << 4,
    MANUAL_PRESS = 1 << 5,

    /** Amplitude port. */
    AMPLITUDE = 1 << 6,

    /**
     * Port controls the stereo balance.
     *
     * This is used in channels for the balance
     * control.
     */
    STEREO_BALANCE = 1 << 7,

    /**
     * Whether the port wants to receive position
     * events.
     *
     * This is only applicable for LV2 Atom ports.
     */
    WANT_POSITION = 1 << 8,

    /**
     * Trigger ports will be set to 0 at the end of
     * each cycle.
     *
     * This mostly applies to LV2 Control Input
     * ports.
     */
    TRIGGER = 1 << 9,

    /** Whether the port is a toggle (on/off). */
    TOGGLE = 1 << 10,

    /** Whether the port is an integer. */
    INTEGER = 1 << 11,

    /** Whether port is for letting the plugin know
     * that we are in freewheeling (export) mode. */
    FREEWHEEL = 1 << 12,

    /** Used for plugin ports. */
    REPORTS_LATENCY = 1 << 13,

    /** Port should not be visible to users. */
    NOT_ON_GUI = 1 << 14,

    /** Port is a switch for plugin enabled. */
    PLUGIN_ENABLED = 1 << 15,

    /** Port is a plugin control. */
    PLUGIN_CONTROL = 1 << 16,

    /** Port is for fader mute. */
    FADER_MUTE = 1 << 17,

    /** Port is for channel fader. */
    CHANNEL_FADER = 1 << 18,

    /**
     * Port has an automation track.
     *
     * If this is set, it is assumed that the automation track at
     * \ref PortIdentifier.port_index is for this port.
     */
    AUTOMATABLE = 1 << 19,

    /** MIDI automatable control, such as modwheel or
     * pitch bend. */
    MIDI_AUTOMATABLE = 1 << 20,

    /** Channels can send to this port (ie, this port
     * is a track processor midi/stereo in or a plugin
     * sidechain in). */
    SEND_RECEIVABLE = 1 << 21,

    /** This is a BPM port. */
    BPM = 1 << 22,

    /**
     * Generic plugin port not belonging to the
     * underlying plugin.
     *
     * This is for ports that are added by Zrythm
     * such as Enabled and Gain.
     */
    GENERIC_PLUGIN_PORT = 1 << 23,

    /** This is the plugin gain. */
    PLUGIN_GAIN = 1 << 24,

    /** Track processor input mono switch. */
    TP_MONO = 1 << 25,

    /** Track processor input gain. */
    TP_INPUT_GAIN = 1 << 26,

    /** Port is a hardware port. */
    HW = 1 << 27,

    /**
     * Port is part of a modulator macro processor.
     *
     * Which of the ports it is can be determined
     * by checking flow/type.
     */
    MODULATOR_MACRO = 1 << 28,

    /** Logarithmic. */
    LOGARITHMIC = 1 << 29,

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
    IS_PROPERTY = 1 << 30,
  };

  enum class Flags2
  {
    /** Transport ports. */
    TRANSPORT_ROLL = 1 << 0,
    TRANSPORT_STOP = 1 << 1,
    TRANSPORT_BACKWARD = 1 << 2,
    TRANSPORT_FORWARD = 1 << 3,
    TRANSPORT_LOOP_TOGGLE = 1 << 4,
    TRANSPORT_REC_TOGGLE = 1 << 5,

    /** LV2 control atom port supports patch
     * messages. */
    SUPPORTS_PATCH_MESSAGE = 1 << 6,

    /** Port's only reasonable values are its scale
     * points. */
    ENUMERATION = 1 << 7,

    /** Parameter port's value type is URI. */
    URI_PARAM = 1 << 8,

    /** Atom port buffer type is sequence. */
    SEQUENCE = 1 << 9,

    /** Atom or event port supports MIDI. */
    SUPPORTS_MIDI = 1 << 10,

    /** Track processor output gain. */
    TP_OUTPUT_GAIN = 1 << 11,

    /** MIDI pitch bend. */
    MIDI_PITCH_BEND = 1 << 12,

    /** MIDI poly key pressure. */
    MIDI_POLY_KEY_PRESSURE = 1 << 13,

    /** MIDI channel pressure. */
    MIDI_CHANNEL_PRESSURE = 1 << 14,

    /** Channel send enabled. */
    CHANNEL_SEND_ENABLED = 1 << 15,

    /** Channel send amount. */
    CHANNEL_SEND_AMOUNT = 1 << 16,

    /** Beats per bar. */
    BEATS_PER_BAR = 1 << 17,

    /** Beat unit. */
    BEAT_UNIT = 1 << 18,

    /** Fader solo. */
    FADER_SOLO = 1 << 19,

    /** Fader listen. */
    FADER_LISTEN = 1 << 20,

    /** Fader mono compat. */
    FADER_MONO_COMPAT = 1 << 21,

    /** Track recording. */
    TRACK_RECORDING = 1 << 22,

    /** Track processor monitor audio. */
    TP_MONITOR_AUDIO = 1 << 23,

    /** Port is owned by prefader. */
    PREFADER = 1 << 24,

    /** Port is owned by postfader. */
    POSTFADER = 1 << 25,

    /** Port is owned by monitor fader. */
    MonitorFader = 1 << 26,

    /** Port is owned by the sample processor fader. */
    SAMPLE_PROCESSOR_FADER = 1 << 27,

    /** Port is owned by sample processor
     * track/channel (including faders owned by those
     * tracks/channels). */
    SAMPLE_PROCESSOR_TRACK = 1 << 28,

    /** Fader swap phase. */
    FADER_SWAP_PHASE = 1 << 29,

    /** MIDI clock. */
    MIDI_CLOCK = 1 << 30,
  };

public:
  PortIdentifier () = default;
  PortIdentifier (const PortIdentifier &other);
  ~PortIdentifier ();

  PortIdentifier &operator= (const PortIdentifier &other);

  void init ();

  const std::string &get_label () const { return label_; }
  const char *       get_label_as_c_str () const { return label_.c_str (); }

  /**
   * Returns the MIDI channel for a MIDI CC port, or -1 if not a MIDI CC port.
   *
   * @note MIDI channels start from 1 (not 0).
   */
  inline int get_midi_channel () const
  {
    if (
      static_cast<int> (flags2_ & PortIdentifier::Flags2::MIDI_PITCH_BEND) != 0
      || static_cast<int> (
           flags2_ & PortIdentifier::Flags2::MIDI_POLY_KEY_PRESSURE)
           != 0
      || static_cast<int> (flags2_ & PortIdentifier::Flags2::MIDI_CHANNEL_PRESSURE)
           != 0)
      {
        return port_index_ + 1;
      }
    else if (
      static_cast<int> (flags_ & PortIdentifier::Flags::MIDI_AUTOMATABLE) != 0)
      {
        return port_index_ / 128 + 1;
      }
    return -1;
  }

  /**
   * Returns if the 2 PortIdentifier's are equal.
   *
   * @note Does not check insignificant data like
   *   comment.
   */
  WARN_UNUSED_RESULT bool is_equal (const PortIdentifier &other) const;

  NONNULL void print_to_str (char * buf, size_t buf_sz) const;
  void         print () const;
  bool         validate () const;
  uint32_t     get_hash () const;

  /**
   * Port group comparator function where @ref p1 and
   * @ref p2 are pointers to Port.
   */
  static int port_group_cmp (const void * p1, const void * p2);

  static const char * get_label (void * data);

  static uint32_t get_hash (const void * data);

  static void destroy_notify (void * data);

  /**
   * To be used as GEqualFunc.
   */
  static int is_equal_func (const void * a, const void * b);

public:
  /** Human readable label. */
  std::string label_;

  /** Unique symbol. */
  std::string sym_;

  /** URI, if LV2 property. */
  std::string uri_;

  /** Comment, if any. */
  std::string comment_;

  /** Owner type. */
  PortIdentifier::OwnerType owner_type_ = (OwnerType) 0;
  /** Data type (e.g. AUDIO). */
  PortType type_ = (PortType) 0;
  /** Flow (IN/OUT). */
  PortFlow flow_ = PortFlow::Unknown;
  /** Flags (e.g. is side chain). */
  PortIdentifier::Flags  flags_ = (Flags) 0;
  PortIdentifier::Flags2 flags2_ = (Flags2) 0;

  /** Port unit. */
  PortUnit unit_ = (PortUnit) 0;

  /** Identifier of plugin. */
  PluginIdentifier plugin_id_ = {};

  /** Port group this port is part of (only applicable for LV2 plugin ports). */
  std::string port_group_;

  /** ExtPort ID (type + full name), if hw port. */
  std::string ext_port_id_;

  /** Track name hash (0 for non-track ports). */
  unsigned int track_name_hash_ = 0;

  /** Index (e.g. in plugin's output ports). */
  int port_index_ = 0;
};

ENUM_ENABLE_BITSET (PortIdentifier::Flags);
ENUM_ENABLE_BITSET (PortIdentifier::Flags2);

/**
 * @}
 */

#endif
