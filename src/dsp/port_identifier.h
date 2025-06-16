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

namespace zrythm::dsp
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
  enum class Flags : std::uint64_t
  {
    StereoL = UINT64_C (1) << 0,
    StereoR = UINT64_C (1) << 1,
    PianoRoll = UINT64_C (1) << 2,
    /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#sideChainOf. */
    Sidechain = UINT64_C (1) << 3,
    /** See http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainInput
     * and http://lv2plug.in/ns/ext/port-groups/port-groups.html#mainOutput. */
    MainPort = UINT64_C (1) << 4,

    /**
     * @brief Piano roll user (piano) key press.
     */
    ManualPress = UINT64_C (1) << 5,

    /** Amplitude port. */
    Amplitude = UINT64_C (1) << 6,

    /**
     * Port controls the stereo balance.
     *
     * This is used in channels for the balance
     * control.
     */
    StereoBalance = UINT64_C (1) << 7,

    /**
     * Whether the port wants to receive position
     * events.
     *
     * This is only applicable for LV2 Atom ports.
     */
    WantPosition = UINT64_C (1) << 8,

    /**
     * Trigger ports will be set to 0 at the end of
     * each cycle.
     *
     * This mostly applies to LV2 Control Input
     * ports.
     */
    Trigger = UINT64_C (1) << 9,

    /** Whether the port is a toggle (on/off). */
    Toggle = UINT64_C (1) << 10,

    /** Whether the port is an integer. */
    Integer = UINT64_C (1) << 11,

    /** Whether port is for letting the plugin know
     * that we are in freewheeling (export) mode. */
    Freewheel = UINT64_C (1) << 12,

    /** Used for plugin ports. */
    ReportsLatency = UINT64_C (1) << 13,

    /** Port should not be visible to users. */
    NotOnGui = UINT64_C (1) << 14,

    /** Port is a switch for plugin enabled. */
    PluginEnabled = UINT64_C (1) << 15,

    /** Port is a plugin control. */
    PluginControl = UINT64_C (1) << 16,

    /** Port is for fader mute. */
    FaderMute = UINT64_C (1) << 17,

    /** Port is for channel fader. */
    ChannelFader = UINT64_C (1) << 18,

    /**
     * Port has an automation track.
     *
     * If this is set, it is assumed that the automation track at
     * @ref PortIdentifier.port_index is for this port.
     */
    Automatable = UINT64_C (1) << 19,

    /** MIDI automatable control, such as modwheel or pitch bend. */
    MidiAutomatable = UINT64_C (1) << 20,

    /** Channels can send to this port (ie, this port
     * is a track processor midi/stereo in or a plugin
     * sidechain in). */
    SendReceivable = UINT64_C (1) << 21,

    /**
     * Generic plugin port not belonging to the
     * underlying plugin.
     *
     * This is for ports that are added by Zrythm
     * such as Enabled and Gain.
     */
    GenericPluginPort = UINT64_C (1) << 22,

    /** This is the plugin gain. */
    PluginGain = UINT64_C (1) << 23,

    /** Track processor input mono switch. */
    TpMono = UINT64_C (1) << 24,

    /** Track processor input gain. */
    TpInputGain = UINT64_C (1) << 25,

    /** Port is a hardware port. */
    Hw = UINT64_C (1) << 26,

    /**
     * Port is part of a modulator macro processor.
     *
     * Which of the ports it is can be determined
     * by checking flow/type.
     */
    ModulatorMacro = UINT64_C (1) << 27,

    /** Logarithmic. */
    Logarithmic = UINT64_C (1) << 28,

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
    IsProperty = UINT64_C (1) << 29,

    /** Transport ports. */
    TransportRoll = UINT64_C (1) << 30,
    TransportStop = UINT64_C (1) << 31,
    TransportBackward = UINT64_C (1) << 32,
    TransportForward = UINT64_C (1) << 33,
    TransportLoopToggle = UINT64_C (1) << 34,
    TransportRecToggle = UINT64_C (1) << 35,

    /** LV2 control atom port supports patch messages. */
    SupportsPatchMessage = UINT64_C (1) << 36,

    /** Port's only reasonable values are its scale points. */
    Enumeration = UINT64_C (1) << 37,

    /** Parameter port's value type is URI. */
    UriParam = UINT64_C (1) << 38,

    /** Atom port buffer type is sequence. */
    Sequence = UINT64_C (1) << 39,

    /** Atom or event port supports MIDI. */
    SupportsMidi = UINT64_C (1) << 40,

    /** Track processor output gain. */
    TpOutputGain = UINT64_C (1) << 41,

    /** MIDI pitch bend. */
    MidiPitchBend = UINT64_C (1) << 42,

    /** MIDI poly key pressure. */
    MidiPolyKeyPressure = UINT64_C (1) << 43,

    /** MIDI channel pressure. */
    MidiChannelPressure = UINT64_C (1) << 44,

    /** Channel send enabled. */
    ChannelSendEnabled = UINT64_C (1) << 45,

    /** Channel send amount. */
    ChannelSendAmount = UINT64_C (1) << 46,

    /** Fader solo. */
    FaderSolo = UINT64_C (1) << 47,

    /** Fader listen. */
    FaderListen = UINT64_C (1) << 48,

    /** Fader mono compat. */
    FaderMonoCompat = UINT64_C (1) << 49,

    /** Track recording. */
    TrackRecording = UINT64_C (1) << 50,

    /** Track processor monitor audio. */
    TpMonitorAudio = UINT64_C (1) << 51,

    /** Port is owned by prefader. */
    Prefader = UINT64_C (1) << 52,

    /** Port is owned by postfader. */
    Postfader = UINT64_C (1) << 53,

    /** Port is owned by monitor fader. */
    MonitorFader = UINT64_C (1) << 54,

    /** Port is owned by the sample processor fader. */
    SampleProcessorFader = UINT64_C (1) << 55,

    /** Port is owned by sample processor track/channel (including faders owned
     * by those tracks/channels). */
    SampleProcessorTrack = UINT64_C (1) << 56,

    /** Fader swap phase. */
    FaderSwapPhase = UINT64_C (1) << 57,

    /** MIDI clock. */
    MidiClock = UINT64_C (1) << 58,
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
    return ENUM_BITSET_TEST (flags_, Flags::MonitorFader)
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
  PortIdentifier::Flags flags_{};

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
    j.at ("trackId").get_to (port.track_id_);
    j.at ("pluginId").get_to (port.plugin_id_);
    j.at ("portGroup").get_to (port.port_group_);
    j.at ("externalPortId").get_to (port.ext_port_id_);
    j.at ("portIndex").get_to (port.port_index_);
    j.at ("midiChannel").get_to (port.midi_channel_);
  }
};

}; // namespace zrythm::dsp

ENUM_ENABLE_BITSET (zrythm::dsp::PortIdentifier::Flags);

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
