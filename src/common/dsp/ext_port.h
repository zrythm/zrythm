// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_EXT_PORT_H__
#define __AUDIO_EXT_PORT_H__

/**
 * @file
 *
 * External ports.
 */

#include "zrythm-config.h"

#include "common/dsp/port.h"
#include "common/io/serialization/iserializable.h"
#include "common/utils/types.h"

#if HAVE_JACK
#  include "weakjack/weak_libjack.h"
#endif

#if HAVE_RTMIDI
#  include <rtmidi_c.h>
#endif

class HardwareProcessor;
enum class AudioBackend;
enum class MidiBackend;

/**
 * @addtogroup dsp
 *
 * @{
 */

/**
 * Maximum external ports.
 *
 * Used for fixed-size arrays.
 */
constexpr int EXT_PORTS_MAX = 1024;

/**
 * External port.
 */
class ExtPort final : public ISerializable<ExtPort>
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

public:
  ExtPort () = default;
#if HAVE_JACK
  ExtPort (jack_port_t * jport);
#endif /* HAVE_JACK */

  bool is_in_active_project () const;

  /**
   * Inits the ExtPort after loading a project.
   */
  ATTR_COLD void init_loaded (HardwareProcessor * hw_processor)
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
  std::string get_id () const;

  friend bool operator== (const ExtPort &a, const ExtPort &b);

  /**
   * Returns a user-friendly display name (eg, to be used in dropdowns).
   */
  std::string get_friendly_name () const;

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

  DECLARE_DEFINE_FIELDS_METHOD ();

public:
/** JACK port. */
#if HAVE_JACK
  jack_port_t * jport_ = nullptr;
#else
  void * jport_ = nullptr;
#endif

  /** Full port name, used also as ID. */
  std::string full_name_;

  /** Short port name. */
  std::string short_name_;

  /** Alias #1 if any. */
  std::string alias1_;

  /** Alias #2 if any. */
  std::string alias2_;

  int num_aliases_ = 0;

  /** RtAudio channel index. */
  unsigned int rtaudio_channel_idx_ = 0;

  /** RtAudio device name. */
  std::string rtaudio_dev_name_;

  /** RtAudio device ID (NOT index!!!). */
  unsigned int rtaudio_id_ = 0;

  /** Whether the channel is input. */
  bool rtaudio_is_input_ = false;
  bool rtaudio_is_duplex_ = false;

  std::shared_ptr<
#if HAVE_RTAUDIO
    RtAudioDevice
#else
    int
#endif
    >
    rtaudio_dev_;

  /** RtMidi port index. */
  unsigned int rtmidi_id_ = 0;

  std::shared_ptr<
#if HAVE_RTMIDI
    RtMidiDevice
#else
    int
#endif
    >
    rtmidi_dev_;

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

inline bool
operator== (const ExtPort &a, const ExtPort &b)
{
  return a.type_ == b.type_ && a.full_name_ == b.full_name_;
}

/**
 * @}
 */

#endif
