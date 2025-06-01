// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "zrythm-config.h"

#include "gui/dsp/port.h"
#include "utils/serialization.h"
#include "utils/types.h"

#ifdef HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

namespace zrythm::engine::device_io
{
class HardwareProcessor;
class AudioEngine;
enum class AudioBackend : basic_enum_base_type_t;
enum class MidiBackend : basic_enum_base_type_t;

/**
 * External port.
 */
class ExtPort final : public IPortOwner
{
public:
  /**
   * External port type.
   */
  enum class Type
  {
    JACK,
    ALSA,
    WindowsMME,
    RtMidi,
    RtAudio,
  };

  using PortType = zrythm::dsp::PortType;
  using PortFlow = zrythm::dsp::PortFlow;

public:
  ExtPort () = default;
#ifdef HAVE_JACK
  ExtPort (jack_port_t * jport);
#endif /* defined(HAVE_JACK) */

  bool is_in_active_project () const override;

  void set_port_metadata_from_owner (dsp::PortIdentifier &id, PortRange &range)
    const override;

  utils::Utf8String
  get_full_designation_for_port (const dsp::PortIdentifier &id) const override;

  /**
   * Inits the ExtPort after loading a project.
   */
  [[gnu::cold]] void init_loaded (HardwareProcessor * hw_processor)
  {
    hw_processor_ = hw_processor;
  }

  /**
   * Prints the port info.
   */
  void print () const;

  /**
   * Returns if the ext port matches the current backend.
   */
  bool matches_backend () const;

  /**
   * Returns a unique identifier (full name prefixed with backend type).
   */
  utils::Utf8String get_id () const;

  friend bool operator== (const ExtPort &a, const ExtPort &b);

  /**
   * Returns a user-friendly display name (eg, to be used in dropdowns).
   */
  utils::Utf8String get_friendly_name () const;

  /**
   * Returns the buffer of the external port.
   */
  float * get_buffer (nframes_t nframes) const;

  /**
   * Clears the buffer of the external port.
   */
  void clear_buffer (nframes_t nframes);

#if 0
/**
 * Exposes the given Port if not exposed and makes
 * the connection from the Port to the ExtPort (eg in
 * JACK) or backwards.
 *
 * @param src 1 if the ext_port is the source, 0 if
 *   it is the destination.
 */
void
connect (
  Port *    port,
  int       src);
#endif

  /**
   * Disconnects the Port from the ExtPort.
   *
   * @param src 1 if the ext_port is the source, 0 if it
   *   is the destination.
   */
  void disconnect (Port * port, int src);

  /**
   * Activates the port (starts receiving data) or deactivates it.
   *
   * @param port Port to send the output to.
   *
   * @return Whether successful.
   */
  bool activate (Port * port, bool activate);

  /**
   * Checks in the GSettings whether this port is marked as enabled by the user.
   *
   * @note Not realtime safe.
   *
   * @return Whether the port is enabled.
   */
  bool get_enabled () const;

  /**
   * Collects external ports of the given type.
   *
   * @param flow The signal flow. Note that this is inverse to what Zrythm sees.
   * E.g., to get MIDI inputs like MIDI keyboards, pass @ref Z_PORT_FLOW_OUTPUT
   * here.
   * @param hw Hardware or not.
   */
  static void ext_ports_get (
    PortType              type,
    PortFlow              flow,
    bool                  hw,
    std::vector<ExtPort> &ports,
    AudioEngine          &engine);

private:
  static constexpr std::string_view kTypeKey = "type";
  static constexpr std::string_view kFullNameKey = "fullName";
  static constexpr std::string_view kShortNameKey = "shortName";
  static constexpr std::string_view kAlias1Key = "alias1";
  static constexpr std::string_view kAlias2Key = "alias2";
  static constexpr std::string_view kNumAliasesKey = "numAliases";
  static constexpr std::string_view kIsMidiKey = "isMidi";
  friend void to_json (nlohmann::json &j, const ExtPort &port)
  {
    j = nlohmann::json{
      { kTypeKey,       port.type_        },
      { kFullNameKey,   port.full_name_   },
      { kShortNameKey,  port.short_name_  },
      { kAlias1Key,     port.alias1_      },
      { kAlias2Key,     port.alias2_      },
      { kNumAliasesKey, port.num_aliases_ },
      { kIsMidiKey,     port.is_midi_     },
    };
  }
  friend void from_json (const nlohmann::json &j, ExtPort &port)
  {
    j.at (kTypeKey).get_to (port.type_);
    j.at (kFullNameKey).get_to (port.full_name_);
    j.at (kShortNameKey).get_to (port.short_name_);
    j.at (kAlias1Key).get_to (port.alias1_);
    j.at (kAlias2Key).get_to (port.alias2_);
    j.at (kNumAliasesKey).get_to (port.num_aliases_);
    j.at (kIsMidiKey).get_to (port.is_midi_);
  }

  friend bool operator== (const ExtPort &a, const ExtPort &b)
  {
    return a.type_ == b.type_ && a.full_name_ == b.full_name_;
  }

public:
/** JACK port. */
#ifdef HAVE_JACK
  jack_port_t * jport_ = nullptr;
#else
  void * jport_ = nullptr;
#endif

  /** Full port name, used also as ID. */
  utils::Utf8String full_name_;

  /** Short port name. */
  utils::Utf8String short_name_;

  /** Alias #1 if any. */
  utils::Utf8String alias1_;

  /** Alias #2 if any. */
  utils::Utf8String alias2_;

  int num_aliases_ = 0;

  Type type_ = (Type) 0;

  /** True if MIDI, false if audio. */
  bool is_midi_ = false;

  /** Index in the HW processor (cache for real-time use) */
  int hw_processor_index_ = -1;

  /** Pointer to owner hardware processor, if any. */
  HardwareProcessor * hw_processor_ = nullptr;

  /** Whether the port is active and receiving events (for use by hw processor).
   */
  bool active_ = false;

  /**
   * Set to true when a hardware port is disconnected.
   *
   * Hardware processor will then attempt to reconnect next scan.
   */
  bool pending_reconnect_ = false;

  /**
   * Temporary port to receive data.
   */
  Port * port_ = nullptr;
};
}
