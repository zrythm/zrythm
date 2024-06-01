/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __AUDIO_RTMIDI_DEVICE_H__
#define __AUDIO_RTMIDI_DEVICE_H__

#include "zrythm-config.h"

#ifdef HAVE_RTMIDI

#  include "zix/ring.h"
#  include <rtmidi_c.h>
#  include <stdint.h>

class Port;

/**
 * @addtogroup dsp
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

  /** Events enqueued at the beginning of each processing cycle from the ring. */
  MidiEvents * events;

  /** Semaphore for blocking writing events while events are being read. */
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
