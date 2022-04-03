/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __AUDIO_RTMIDI_DEVICE_H__
#define __AUDIO_RTMIDI_DEVICE_H__

#include "zrythm-config.h"

#ifdef HAVE_RTMIDI

#  include <stdint.h>

#  include "zix/ring.h"
#  include <rtmidi_c.h>

typedef struct Port Port;

/**
 * @addtogroup audio
 *
 * @{
 */

#  define MIDI_BUFFER_SIZE 32768

/**
 * For readability when passing 0/1 for input
 * and output.
 */
enum RtMidiDeviceFlow
{
  RTMIDI_DEVICE_FLOW_OUTPUT,
  RTMIDI_DEVICE_FLOW_INPUT,
};

typedef struct RtMidiDevice
{
  /** 1 for input, 0 for output. */
  int is_input;

  /** Index (device index from RtMidi). */
  unsigned int id;

  char * name;

  /** Whether opened or not. */
  int opened;

  /** Whether started (running) or not. */
  int started;

  /* ---- INPUT ---- */
  RtMidiInPtr in_handle;

  /* ---- OUTPUT ---- */
  RtMidiOutPtr out_handle;

  /** Associated port. */
  Port * port;

  /** MIDI event ring buffer. */
  ZixRing * midi_ring;

  /** Events enqueued at the beginning of each
   * processing cycle from the ring. */
  MidiEvents * events;

  /** Semaphore for blocking writing events while
   * events are being read. */
  ZixSem midi_ring_sem;

} RtMidiDevice;

/**
 * @param name If non-NUL, search by name instead of
 *   by @ref device_id.
 */
RtMidiDevice *
rtmidi_device_new (
  bool         is_input,
  const char * name,
  unsigned int device_id,
  Port *       port);

/**
 * Opens a device allocated with
 * rtmidi_device_new().
 *
 * @param start Also start the device.
 *
 * @return Non-zero if error.
 */
int
rtmidi_device_open (RtMidiDevice * dev, int start);

/**
 * Close the RtMidiDevice.
 *
 * @param free Also free the memory.
 */
int
rtmidi_device_close (RtMidiDevice * self, int free);

int
rtmidi_device_start (RtMidiDevice * self);

int
rtmidi_device_stop (RtMidiDevice * self);

void
rtmidi_device_free (RtMidiDevice * self);

/**
 * @}
 */

#endif // HAVE_RTMIDI
#endif
