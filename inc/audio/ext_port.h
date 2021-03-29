/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version approved by
 * Alexandros Theodotou in a public statement of acceptance.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_EXT_PORT_H__
#define __AUDIO_EXT_PORT_H__

/**
 * \file
 *
 * External ports.
 */

#include "zrythm-config.h"

#include "audio/fader.h"
#include "plugins/plugin.h"
#include "utils/audio.h"
#include "utils/types.h"
#include "utils/yaml.h"

#include <gdk/gdk.h>

#ifdef HAVE_JACK
#include "weak_libjack.h"
#endif

#ifdef HAVE_RTMIDI
#include <rtmidi/rtmidi_c.h>
#endif

typedef struct WindowsMmeDevice WindowsMmeDevice;

/**
 * @addtogroup audio
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

/**
 * External port type.
 */
typedef enum ExtPortType
{
  EXT_PORT_TYPE_JACK,
  EXT_PORT_TYPE_ALSA,
  EXT_PORT_TYPE_WINDOWS_MME,
  EXT_PORT_TYPE_RTMIDI,
  EXT_PORT_TYPE_RTAUDIO,
} ExtPortType;

static const cyaml_strval_t
ext_port_type_strings[] =
{
  { "JACK",   EXT_PORT_TYPE_JACK    },
  { "ALSA",   EXT_PORT_TYPE_ALSA   },
  { "Windows MME",   EXT_PORT_TYPE_WINDOWS_MME   },
  { "RtMidi",   EXT_PORT_TYPE_RTMIDI   },
  { "RtAudio",   EXT_PORT_TYPE_RTAUDIO   },
};

/**
 * External port.
 */
typedef struct ExtPort
{
  int              schema_version;

  /** JACK port. */
#ifdef HAVE_JACK
  jack_port_t *    jport;
#else
  void *           jport;
#endif

  /** Full port name, used also as ID. */
  char *           full_name;

  /** Short port name. */
  char *           short_name;

  /** Alias #1 if any. */
  char *           alias1;

  /** Alias #2 if any. */
  char *           alias2;

  int              num_aliases;

#ifdef _WOE32
  /**
   * Pointer to a WindowsMmeDevice.
   *
   * This must be one of the devices in AudioEngine.
   * It must NOT be allocated or free'd.
   */
  WindowsMmeDevice * mme_dev;
#else
  void *             mme_dev;
#endif

  /** RtAudio channel index. */
  unsigned int     rtaudio_channel_idx;

  /** RtAudio device name. */
  char *           rtaudio_dev_name;

  /** RtAudio device index. */
  unsigned int     rtaudio_id;

  /** Whether the channel is input. */
  bool             rtaudio_is_input;
  bool             rtaudio_is_duplex;

#ifdef HAVE_RTAUDIO
  RtAudioDevice *  rtaudio_dev;
#else
  void *           rtaudio_dev;
#endif

  /** RtMidi port index. */
  unsigned int     rtmidi_id;

#ifdef HAVE_RTMIDI
  RtMidiDevice *   rtmidi_dev;
#else
  void *           rtmidi_dev;
#endif

  ExtPortType      type;

  /** True if MIDI, false if audio. */
  bool             is_midi;

  /** Index in the HW processor (cache for real-time
   * use) */
  int              hw_processor_index;

  /** Whether the port is active and receiving
   * events (for use by hw processor). */
  bool             active;

  /**
   * Temporary port to receive data.
   */
  Port *           port;
} ExtPort;

static const cyaml_schema_field_t
ext_port_fields_schema[] =
{
  YAML_FIELD_INT (
    ExtPort, schema_version),
  YAML_FIELD_STRING_PTR (
    ExtPort, full_name),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    ExtPort, short_name),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    ExtPort, alias1),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    ExtPort, alias2),
  YAML_FIELD_STRING_PTR_OPTIONAL (
    ExtPort, rtaudio_dev_name),
  YAML_FIELD_INT (
    ExtPort, num_aliases),
  YAML_FIELD_INT (
    ExtPort, is_midi),
  YAML_FIELD_ENUM (
    ExtPort, type, ext_port_type_strings),
  YAML_FIELD_UINT (
    ExtPort, rtaudio_channel_idx),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
ext_port_schema =
{
  YAML_VALUE_PTR (
    ExtPort, ext_port_fields_schema),
};

/**
 * Inits the ExtPort after loading a project.
 */
void
ext_port_init_loaded (
  ExtPort * ext_port);

/**
 * Prints the port info.
 */
void
ext_port_print (
  ExtPort * self);

/**
 * Returns if the ext port matches the current
 * backend.
 */
bool
ext_port_matches_backend (
  ExtPort * self);

/**
 * Returns a unique identifier (full name prefixed
 * with backend type).
 */
char *
ext_port_get_id (
  ExtPort * ext_port);

/**
 * Returns the buffer of the external port.
 */
float *
ext_port_get_buffer (
  ExtPort * ext_port,
  nframes_t nframes);

/**
 * Clears the buffer of the external port.
 */
void
ext_port_clear_buffer (
  ExtPort * ext_port,
  nframes_t nframes);

/**
 * Exposes the given Port if not exposed and makes
 * the connection from the Port to the ExtPort (eg in
 * JACK) or backwards.
 *
 * @param src 1 if the ext_port is the source, 0 if it
 *   is the destination.
 */
void
ext_port_connect (
  ExtPort * ext_port,
  Port *    port,
  int       src);

/**
 * Disconnects the Port from the ExtPort.
 *
 * @param src 1 if the ext_port is the source, 0 if it
 *   is the destination.
 */
void
ext_port_disconnect (
  ExtPort * ext_port,
  Port *    port,
  int       src);

/**
 * Activates the port (starts receiving data) or
 * deactivates it.
 *
 * @param port Port to send the output to.
 */
void
ext_port_activate (
  ExtPort * self,
  Port *    port,
  bool      activate);

/**
 * Checks in the GSettings whether this port is
 * marked as enabled by the user.
 *
 * @note Not realtime safe.
 *
 * @return Whether the port is enabled.
 */
bool
ext_port_get_enabled (
  ExtPort * self);

/**
 * Collects external ports of the given type.
 *
 * @param flow The signal flow. Note that this is
 *   inverse to what Zrythm sees. E.g., to get
 *   MIDI inputs like MIDI keyboards, pass
 *   \ref FLOW_OUTPUT here.
 * @param hw Hardware or not.
 * @param ports An array of ExtPort pointers to fill
 *   in. The array should be preallocated.
 * @param size Size of the array to fill in.
 */
void
ext_ports_get (
  PortType   type,
  PortFlow   flow,
  bool       hw,
  ExtPort ** ports,
  int *      size);

/**
 * Creates a shallow clone of the port.
 */
ExtPort *
ext_port_clone (
  ExtPort * ext_port);

/**
 * Frees an array of ExtPort pointers.
 */
void
ext_ports_free (
  ExtPort ** ext_port,
  int        size);

/**
 * Frees the ext_port.
 */
void
ext_port_free (
  ExtPort * ext_port);

/**
 * @}
 */

#endif
