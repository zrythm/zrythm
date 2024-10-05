// SPDX-FileCopyrightText: © 2018-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_PORT_IDENTIFIER_H__
#define __AUDIO_PORT_IDENTIFIER_H__

#include "zrythm-config.h"

#include "common/plugins/plugin_identifier.h"
#include "common/utils/types.h"

#include "juce/juce.h"

/**
 * @addtogroup dsp
 *
 * @{
 */

constexpr int PORT_IDENTIFIER_MAGIC = 3411841;

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
  None,
  Hz,
  MHz,
  Db,
  Degrees,
  Seconds,

  /** Milliseconds. */
  Ms,

  /** Microseconds. */
  Us,
};

const char *
port_unit_to_str (const PortUnit unit);

/**
 * Struct used to identify Ports in the project.
 */
class PortIdentifier final : public ISerializable<PortIdentifier>
{
public:
  /**
   * Type of owner.
   */
  enum class OwnerType
  {
    /* NONE, */
    AudioEngine,

    /** Plugin owner. */
    Plugin,

    /** Track owner. */
    Track,

    /** Channel owner. */
    Channel,

    /** Fader. */
    Fader,

    /**
     * Channel send.
     *
     * PortIdentifier.port_index will contain the
     * send index on the port's track's channel.
     */
    ChannelSend,

    /* TrackProcessor. */
    TrackProcessor,

    /** Port is part of a HardwareProcessor. */
    HardwareProcessor,

    /** Port is owned by engine transport. */
    Transport,

    /** Modulator macro processor owner. */
    ModulatorMacroProcessor,
  };

  /**
   * Port flags.
   */
  enum class Flags
  {
    StereoL = 1 << 0,
    StereoR = 1 << 1,
    PianoRoll = 1 << 2,
    /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#sideChainOf. */
    Sidechain = 1 << 3,
    /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainInput
     * and http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainOutput. */
    MainPort = 1 << 4,
    ManualPress = 1 << 5,

    /** Amplitude port. */
    Amplitude = 1 << 6,

    /**
     * Port controls the stereo balance.
     *
     * This is used in channels for the balance
     * control.
     */
    StereoBalance = 1 << 7,

    /**
     * Whether the port wants to receive position
     * events.
     *
     * This is only applicable for LV2 Atom ports.
     */
    WantPosition = 1 << 8,

    /**
     * Trigger ports will be set to 0 at the end of
     * each cycle.
     *
     * This mostly applies to LV2 Control Input
     * ports.
     */
    Trigger = 1 << 9,

    /** Whether the port is a toggle (on/off). */
    Toggle = 1 << 10,

    /** Whether the port is an integer. */
    Integer = 1 << 11,

    /** Whether port is for letting the plugin know
     * that we are in freewheeling (export) mode. */
    Freewheel = 1 << 12,

    /** Used for plugin ports. */
    ReportsLatency = 1 << 13,

    /** Port should not be visible to users. */
    NotOnGui = 1 << 14,

    /** Port is a switch for plugin enabled. */
    PluginEnabled = 1 << 15,

    /** Port is a plugin control. */
    PluginControl = 1 << 16,

    /** Port is for fader mute. */
    FaderMute = 1 << 17,

    /** Port is for channel fader. */
    ChannelFader = 1 << 18,

    /**
     * Port has an automation track.
     *
     * If this is set, it is assumed that the automation track at
     * @ref PortIdentifier.port_index is for this port.
     */
    Automatable = 1 << 19,

    /** MIDI automatable control, such as modwheel or pitch bend. */
    MidiAutomatable = 1 << 20,

    /** Channels can send to this port (ie, this port
     * is a track processor midi/stereo in or a plugin
     * sidechain in). */
    SendReceivable = 1 << 21,

    /** This is a BPM port. */
    Bpm = 1 << 22,

    /**
     * Generic plugin port not belonging to the
     * underlying plugin.
     *
     * This is for ports that are added by Zrythm
     * such as Enabled and Gain.
     */
    GenericPluginPort = 1 << 23,

    /** This is the plugin gain. */
    PluginGain = 1 << 24,

    /** Track processor input mono switch. */
    TpMono = 1 << 25,

    /** Track processor input gain. */
    TpInputGain = 1 << 26,

    /** Port is a hardware port. */
    Hw = 1 << 27,

    /**
     * Port is part of a modulator macro processor.
     *
     * Which of the ports it is can be determined
     * by checking flow/type.
     */
    ModulatorMacro = 1 << 28,

    /** Logarithmic. */
    Logarithmic = 1 << 29,

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
    IsProperty = 1 << 30,
  };

  enum class Flags2
  {
    /** Transport ports. */
    TransportRoll = 1 << 0,
    TransportStop = 1 << 1,
    TransportBackward = 1 << 2,
    TransportForward = 1 << 3,
    TransportLoopToggle = 1 << 4,
    TransportRecToggle = 1 << 5,

    /** LV2 control atom port supports patch
     * messages. */
    SupportsPatchMessage = 1 << 6,

    /** Port's only reasonable values are its scale
     * points. */
    Enumeration = 1 << 7,

    /** Parameter port's value type is URI. */
    UriParam = 1 << 8,

    /** Atom port buffer type is sequence. */
    Sequence = 1 << 9,

    /** Atom or event port supports MIDI. */
    SupportsMidi = 1 << 10,

    /** Track processor output gain. */
    TpOutputGain = 1 << 11,

    /** MIDI pitch bend. */
    MidiPitchBend = 1 << 12,

    /** MIDI poly key pressure. */
    MidiPolyKeyPressure = 1 << 13,

    /** MIDI channel pressure. */
    MidiChannelPressure = 1 << 14,

    /** Channel send enabled. */
    ChannelSendEnabled = 1 << 15,

    /** Channel send amount. */
    ChannelSendAmount = 1 << 16,

    /** Beats per bar. */
    BeatsPerBar = 1 << 17,

    /** Beat unit. */
    BeatUnit = 1 << 18,

    /** Fader solo. */
    FaderSolo = 1 << 19,

    /** Fader listen. */
    FaderListen = 1 << 20,

    /** Fader mono compat. */
    FaderMonoCompat = 1 << 21,

    /** Track recording. */
    TrackRecording = 1 << 22,

    /** Track processor monitor audio. */
    TpMonitorAudio = 1 << 23,

    /** Port is owned by prefader. */
    Prefader = 1 << 24,

    /** Port is owned by postfader. */
    Postfader = 1 << 25,

    /** Port is owned by monitor fader. */
    MonitorFader = 1 << 26,

    /** Port is owned by the sample processor fader. */
    SampleProcessorFader = 1 << 27,

    /** Port is owned by sample processor
     * track/channel (including faders owned by those
     * tracks/channels). */
    SampleProcessorTrack = 1 << 28,

    /** Fader swap phase. */
    FaderSwapPhase = 1 << 29,

    /** MIDI clock. */
    MidiClock = 1 << 30,
  };

public:
  // Rule of 0

  void init ();

  std::string  get_label () const { return label_; }
  const char * get_label_as_c_str () const { return label_.c_str (); }

  bool is_control () const { return type_ == PortType::Control; }
  bool is_midi () const { return type_ == PortType::Event; }
  bool is_cv () const { return type_ == PortType::CV; }
  bool is_audio () const { return type_ == PortType::Audio; }

  bool is_input () const { return flow_ == PortFlow::Input; }
  bool is_output () const { return flow_ == PortFlow::Output; }

  /**
   * Returns the MIDI channel for a MIDI CC port, or -1 if not a MIDI CC port.
   *
   * @note MIDI channels start from 1 (not 0).
   */
  inline int get_midi_channel () const
  {
    if (
      static_cast<int> (flags2_ & PortIdentifier::Flags2::MidiPitchBend) != 0
      || static_cast<int> (flags2_ & PortIdentifier::Flags2::MidiPolyKeyPressure)
           != 0
      || static_cast<int> (flags2_ & PortIdentifier::Flags2::MidiChannelPressure)
           != 0)
      {
        return port_index_ + 1;
      }
    else if (
      static_cast<int> (flags_ & PortIdentifier::Flags::MidiAutomatable) != 0)
      {
        return port_index_ / 128 + 1;
      }
    return -1;
  }

  std::string print_to_str () const;
  void        print () const;
  bool        validate () const;
  uint32_t    get_hash () const;

  /**
   * Port group comparator function where @ref p1 and
   * @ref p2 are pointers to Port.
   */
  static int port_group_cmp (const void * p1, const void * p2);

  static void destroy_notify (void * data)
  {
    delete static_cast<PortIdentifier *> (data);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
  /** Index (e.g. in plugin's output ports). */
  int port_index_ = 0;

  /** Track name hash (0 for non-track ports). */
  unsigned int track_name_hash_ = 0;

  /** Owner type. */
  PortIdentifier::OwnerType owner_type_ = (OwnerType) 0;
  /** Data type (e.g. AUDIO). */
  PortType type_ = (PortType) 0;
  /** Flow (IN/OUT). */
  PortFlow flow_ = PortFlow::Unknown;

  /** Port unit. */
  PortUnit unit_ = (PortUnit) 0;

  /** Flags (e.g. is side chain). */
  PortIdentifier::Flags  flags_ = (Flags) 0;
  PortIdentifier::Flags2 flags2_ = (Flags2) 0;

  /** Identifier of plugin. */
  PluginIdentifier plugin_id_ = {};

  /** Human readable label. */
  std::string label_;

  /** Unique symbol. */
  std::string sym_;

  /** URI, if LV2 property. */
  std::string uri_;

  /** Comment, if any. */
  std::string comment_;

  /** Port group this port is part of (only applicable for LV2 plugin ports). */
  std::string port_group_;

  /** ExtPort ID (type + full name), if hw port. */
  std::string ext_port_id_;
};

ENUM_ENABLE_BITSET (PortIdentifier::Flags);
ENUM_ENABLE_BITSET (PortIdentifier::Flags2);

bool
operator== (const PortIdentifier &lhs, const PortIdentifier &rhs);

namespace std
{
template <> struct hash<PortIdentifier>
{
  size_t operator() (const PortIdentifier &id) const { return id.get_hash (); }
};
}

/**
 * @}
 */

#endif
