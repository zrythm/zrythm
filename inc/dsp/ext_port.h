// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __AUDIO_EXT_PORT_H__
#define __AUDIO_EXT_PORT_H__

/**
 * \file
 *
 * External ports.
 */

#include "zrythm-config.h"

#include "dsp/fader.h"
#include "plugins/plugin.h"
#include "utils/audio.h"
#include "utils/types.h"
#include "utils/yaml.h"

#include <gdk/gdk.h>

#ifdef HAVE_JACK
#  include "weak_libjack.h"
#endif

#ifdef HAVE_RTMIDI
#  include <rtmidi_c.h>
#endif

typedef struct WindowsMmeDevice  WindowsMmeDevice;
typedef struct HardwareProcessor HardwareProcessor;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define EXT_PORT_SCHEMA_VERSION 1

/**
 * Maximum external ports.
 *
 * Used for fixed-size arrays.
 */
#define EXT_PORTS_MAX 1024

#define ext_port_is_in_active_project(self) \
  (self->hw_processor \
   && hw_processor_is_in_active_project ((self)->hw_processor))

/**
 * External port type.
 */
enum class ExtPortType
{
  EXT_PORT_TYPE_JACK,
  EXT_PORT_TYPE_ALSA,
  EXT_PORT_TYPE_WINDOWS_MME,
  EXT_PORT_TYPE_RTMIDI,
  EXT_PORT_TYPE_RTAUDIO,
};

/**
 * External port.
 */
typedef struct ExtPort
{
  int schema_version;

  /** JACK port. */
#ifdef HAVE_JACK
  jack_port_t * jport;
#else
  void * jport;
#endif

  /** Full port name, used also as ID. */
  char * full_name;

  /** Short port name. */
  char * short_name;

  /** Alias #1 if any. */
  char * alias1;

  /** Alias #2 if any. */
  char * alias2;

  int num_aliases;

#ifdef _WIN32
  /**
   * Pointer to a WindowsMmeDevice.
   *
   * This must be one of the devices in AudioEngine.
   * It must NOT be allocated or free'd.
   */
  WindowsMmeDevice * mme_dev;
#else
  void * mme_dev;
#endif

  /** RtAudio channel index. */
  unsigned int rtaudio_channel_idx;

  /** RtAudio device name. */
  char * rtaudio_dev_name;

  /** RtAudio device ID (NOT index!!!). */
  unsigned int rtaudio_id;

  /** Whether the channel is input. */
  bool rtaudio_is_input;
  bool rtaudio_is_duplex;

#ifdef HAVE_RTAUDIO
  RtAudioDevice * rtaudio_dev;
#else
  void * rtaudio_dev;
#endif

  /** RtMidi port index. */
  unsigned int rtmidi_id;

#ifdef HAVE_RTMIDI
  RtMidiDevice * rtmidi_dev;
#else
  void * rtmidi_dev;
#endif

  ExtPortType type;

  /** True if MIDI, false if audio. */
  bool is_midi;

  /** Index in the HW processor (cache for real-time
   * use) */
  int hw_processor_index;

  /** Pointer to owner hardware processor, if any. */
  HardwareProcessor * hw_processor;

  /** Whether the port is active and receiving events (for use by hw processor).
   */
  bool active;

  /**
   * Set to true when a hardware port is
   * disconnected.
   *
   * Hardware processor will then attempt to
   * reconnect next scan.
   */
  bool pending_reconnect;

  /**
   * Temporary port to receive data.
   */
  Port * port;
} ExtPort;

/**
 * Inits the ExtPort after loading a project.
 */
COLD NONNULL_ARGS (1) void ext_port_init_loaded (
  ExtPort *           self,
  HardwareProcessor * hw_processor);

/**
 * Prints the port info.
 */
void
ext_port_print (ExtPort * self);

/**
 * Returns if the ext port matches the current
 * backend.
 */
bool
ext_port_matches_backend (ExtPort * self);

/**
 * Returns a unique identifier (full name prefixed with backend type).
 *
 * @memberof ExtPort
 */
MALLOC
NONNULL char *
ext_port_get_id (ExtPort * ext_port);

NONNULL bool
ext_ports_equal (const ExtPort * a, const ExtPort * b);

/**
 * Returns a user-friendly display name (eg, to be used in dropdowns).
 *
 * @memberof ExtPort
 */
NONNULL char *
ext_port_get_friendly_name (ExtPort * self);

/**
 * Returns the buffer of the external port.
 */
float *
ext_port_get_buffer (ExtPort * ext_port, nframes_t nframes);

/**
 * Clears the buffer of the external port.
 */
void
ext_port_clear_buffer (ExtPort * ext_port, nframes_t nframes);

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
ext_port_connect (
  ExtPort * ext_port,
  Port *    port,
  int       src);
#endif

#ifdef HAVE_JACK
/**
 * Creates an ExtPort from a JACK port.
 */
ExtPort *
ext_port_new_from_jack_port (jack_port_t * jport);
#endif /* HAVE_JACK */

/**
 * Disconnects the Port from the ExtPort.
 *
 * @param src 1 if the ext_port is the source, 0 if it
 *   is the destination.
 */
void
ext_port_disconnect (ExtPort * ext_port, Port * port, int src);

/**
 * Activates the port (starts receiving data) or
 * deactivates it.
 *
 * @param port Port to send the output to.
 *
 * @return Non-zero if fail.
 */
int
ext_port_activate (ExtPort * self, Port * port, bool activate);

/**
 * Checks in the GSettings whether this port is
 * marked as enabled by the user.
 *
 * @note Not realtime safe.
 *
 * @return Whether the port is enabled.
 */
bool
ext_port_get_enabled (ExtPort * self);

/**
 * Collects external ports of the given type.
 *
 * @param flow The signal flow. Note that this is inverse to what Zrythm sees.
 * E.g., to get MIDI inputs like MIDI keyboards, pass \ref Z_PORT_FLOW_OUTPUT
 * here.
 * @param hw Hardware or not.
 */
void
ext_ports_get (PortType type, PortFlow flow, bool hw, GPtrArray * ports);

/**
 * Creates a shallow clone of the port.
 */
ExtPort *
ext_port_clone (ExtPort * ext_port);

/**
 * Frees an array of ExtPort pointers.
 */
void
ext_ports_free (ExtPort ** ext_port, int size);

/**
 * Frees the ext_port.
 */
void
ext_port_free (ExtPort * ext_port);

/**
 * @}
 */

#endif
