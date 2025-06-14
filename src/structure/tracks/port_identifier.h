// SPDX-FileCopyrightText: © 2018-2021, 2023-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "utils/icloneable.h"
#include "utils/types.h"
#include "utils/uuid_identifiable_object.h"

namespace zrythm::structure::tracks
{
class Track;
}
namespace zrythm::gui::old_dsp::plugins
{
class Plugin;
}

class Port;

namespace zrythm::structure::tracks
{

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

/**
 * Struct used to identify Ports in the project.
 */
class PortIdentifier
{
public:
  using TrackUuid =
    utils::UuidIdentifiableObject<zrythm::structure::tracks::Track>::Uuid;
  using PluginUuid =
    utils::UuidIdentifiableObject<gui::old_dsp::plugins::Plugin>::Uuid;
  using PortUuid = utils::UuidIdentifiableObject<Port>::Uuid;

  /**
   * Type of owner.
   */
  enum class OwnerType
  {
    /* NONE, */
    AudioEngine,

    /** zrythm::gui::old_dsp::plugins::Plugin owner. */
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

    /**
     * @brief Piano roll user (piano) key press.
     */
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
     * zrythm::gui::old_dsp::plugins::Plugin control is a property (changes are
     * set via atom message on the plugin's control port), as opposed to
     * conventional float control ports.
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

    /** LV2 control atom port supports patch messages. */
    SupportsPatchMessage = 1 << 6,

    /** Port's only reasonable values are its scale points. */
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

  friend bool operator== (const PortIdentifier &lhs, const PortIdentifier &rhs);

public:
  // void init ();

  utils::Utf8String get_label () const { return label_; }

  bool is_control () const { return type_ == PortType::Control; }
  bool is_midi () const { return type_ == PortType::Event; }
  bool is_cv () const { return type_ == PortType::CV; }
  bool is_audio () const { return type_ == PortType::Audio; }

  bool is_input () const { return flow_ == PortFlow::Input; }
  bool is_output () const { return flow_ == PortFlow::Output; }

  bool is_monitor_fader_stereo_in_or_out_port () const
  {
    return ENUM_BITSET_TEST (flags2_, Flags2::MonitorFader)
           && (ENUM_BITSET_TEST (flags_, Flags::StereoL) || ENUM_BITSET_TEST (flags_, Flags::StereoR));
  }

  auto get_track_id () const { return track_id_; }
  void set_track_id (TrackUuid track_id) { track_id_ = track_id; }
  auto get_plugin_id () const { return plugin_id_; }
  void set_plugin_id (PluginUuid plugin_id) { plugin_id_ = plugin_id; }
  auto get_symbol () const { return sym_; }

  std::string print_to_str () const;
  void        print () const;
  size_t      get_hash () const;

  static utils::Utf8String port_unit_to_string (PortUnit unit);

  friend void init_from (
    PortIdentifier        &obj,
    const PortIdentifier  &other,
    utils::ObjectCloneType clone_type)
  {
    obj = other;
  }

public:
  /** Index (e.g. in plugin's output ports, the modulator macro processor slot,
   * etc.). */
  size_t port_index_{};

  /** Track identifier. */
  std::optional<TrackUuid> track_id_;

  /** Owner type. */
  PortIdentifier::OwnerType owner_type_{};
  /** Data type (e.g. AUDIO). */
  PortType type_{ PortType::Unknown };
  /** Flow (IN/OUT). */
  PortFlow flow_{ PortFlow::Unknown };

  /** Port unit. */
  PortUnit unit_{ PortUnit::None };

  /** Flags (e.g. is side chain). */
  PortIdentifier::Flags  flags_{};
  PortIdentifier::Flags2 flags2_{};

  /** Identifier of plugin. */
  std::optional<PluginUuid> plugin_id_;

  /** Human readable label. */
  utils::Utf8String label_;

  /** Unique symbol. */
  utils::Utf8String sym_;

  /** URI, if LV2 property. */
  std::optional<utils::Utf8String> uri_;

  /** Comment, if any. */
  std::optional<utils::Utf8String> comment_;

  /** Port group this port is part of (only applicable for LV2 plugin ports). */
  std::optional<utils::Utf8String> port_group_;

  /** ExtPort ID (type + full name), if hw port. */
  std::optional<utils::Utf8String> ext_port_id_;

  /** MIDI channel if MIDI CC port, starting from 1 (so [1, 16]). */
  std::optional<midi_byte_t> midi_channel_;

  friend void to_json (nlohmann::json &j, const PortIdentifier &port)
  {
    j = nlohmann::json{
      { "label",          port.label_        },
      { "symbol",         port.sym_          },
      { "uri",            port.uri_          },
      { "comment",        port.comment_      },
      { "ownerType",      port.owner_type_   },
      { "type",           port.type_         },
      { "flow",           port.flow_         },
      { "unit",           port.unit_         },
      { "flags",          port.flags_        },
      { "flags2",         port.flags2_       },
      { "trackId",        port.track_id_     },
      { "pluginId",       port.plugin_id_    },
      { "portGroup",      port.port_group_   },
      { "externalPortId", port.ext_port_id_  },
      { "portIndex",      port.port_index_   },
      { "midiChannel",    port.midi_channel_ },
    };
  }
  friend void from_json (const nlohmann::json &j, PortIdentifier &port)
  {
    j.at ("label").get_to (port.label_);
    j.at ("symbol").get_to (port.sym_);
    j.at ("uri").get_to (port.uri_);
    j.at ("comment").get_to (port.comment_);
    j.at ("ownerType").get_to (port.owner_type_);
    j.at ("type").get_to (port.type_);
    j.at ("flow").get_to (port.flow_);
    j.at ("unit").get_to (port.unit_);
    j.at ("flags").get_to (port.flags_);
    j.at ("flags2").get_to (port.flags2_);
    j.at ("trackId").get_to (port.track_id_);
    j.at ("pluginId").get_to (port.plugin_id_);
    j.at ("portGroup").get_to (port.port_group_);
    j.at ("externalPortId").get_to (port.ext_port_id_);
    j.at ("portIndex").get_to (port.port_index_);
    j.at ("midiChannel").get_to (port.midi_channel_);
  }
};

}; // namespace zrythm::dsp

ENUM_ENABLE_BITSET (zrythm::structure::tracks::PortIdentifier::Flags);
ENUM_ENABLE_BITSET (zrythm::structure::tracks::PortIdentifier::Flags2);

namespace std
{
template <> struct hash<zrythm::utils::UuidIdentifiableObject<Port>::Uuid>
{
  std::size_t
  operator() (const zrythm::utils::UuidIdentifiableObject<Port>::Uuid &uuid) const
  {
    return uuid.hash ();
  }
};
}
