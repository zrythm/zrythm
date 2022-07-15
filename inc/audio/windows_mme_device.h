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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright (C) 2015 Tim Mayberry <mojofunk@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "zrythm-config.h"

#ifdef _WOE32

#  ifndef __AUDIO_WINDOWS_MME_DEVICE_H__
#    define __AUDIO_WINDOWS_MME_DEVICE_H__

#    include <windows.h>

#    include <stdint.h>

#    include "zix/ring.h"

typedef struct AudioEngine AudioEngine;
typedef struct MidiEvent   MidiEvent;

/**
 * @addtogroup audio
 *
 * @{
 */

#    define MIDI_BUFFER_SIZE 32768
#    define SYSEX_BUFFER_SIZE 32768

/**
 * For readability when passing 0/1 for input
 * and output.
 */
enum WindowsMmeDeviceFlow
{
  WINDOWS_MME_DEVICE_FLOW_OUTPUT,
  WINDOWS_MME_DEVICE_FLOW_INPUT,
};

typedef struct WindowsMmeDevice
{
  /** 1 for input, 0 for output. */
  int is_input;

  /** Index. */
  int id;

  unsigned int manufacturer_id;
  unsigned int product_id;
  unsigned int driver_ver_major;
  unsigned int driver_ver_minor;

  char * name;

  /** Whether opened or not. */
  int opened;

  /** Whether started (running) or not. */
  int started;

  /* ---- INPUT ---- */
  HMIDIIN in_handle;
  MIDIHDR sysex_header;

  /* ---- OUTPUT ---- */
  HMIDIOUT out_handle;

  /** MIDI event ring buffer. */
  ZixRing * midi_ring;

  uint8_t sysex_buffer[SYSEX_BUFFER_SIZE];

} WindowsMmeDevice;

WindowsMmeDevice *
windows_mme_device_new (int input, int index);

/**
 * Opens a device allocated with
 * engine_windows_mme_device_new().
 *
 * @param start Also start the device.
 *
 * @return Non-zero if error.
 */
int
windows_mme_device_open (WindowsMmeDevice * dev, int start);

/**
 * Close the WindowsMmeDevice.
 *
 * @param free Also free the memory.
 */
int
windows_mme_device_close (WindowsMmeDevice * self, int free);

int
windows_mme_device_start (WindowsMmeDevice * self);

int
windows_mme_device_stop (WindowsMmeDevice * self);

/**
 * MIDI in signal handler.
 *
 * The meaning of the dwParam1 and dwParam2
 * parameters
 * is specific to the message type. For more
 * information, see the topics for messages, such as
 * MIM_DATA.
 *
 * Applications should not call any multimedia
 * functions from inside the callback function, as
 * doing so can cause a deadlock. Other system
 * functions can safely be called from the callback.
 *
 * @param hMidiIn Handle to the MIDI input device.
 * @param wMsg MIDI input message.
 * @param dwInstance Instance data supplied with the midiInOpen function.
 * @param dwParam1 Message parameter.
 * @param dwParam2 Message parameter.
 */
void
windows_mme_device_input_cb (
  HMIDIIN   hMidiIn,
  UINT      wMsg,
  DWORD_PTR dwInstance,
  DWORD_PTR dwParam1,
  DWORD_PTR dwParam2);

/**
 * Dequeues a MIDI event from the queue into a
 * MidiEvent struct.
 *
 * @param timestamp_start The timestamp at the start
 *   of the processing cycle.
 * @param timestamp_end The expected timestamp at
 *   the end of the processing cycle.
 *
 * @return Whether a MIDI event was dequeued or
 * not.
 */
int
windows_mme_device_dequeue_midi_event_struct (
  WindowsMmeDevice * self,
  uint64_t           timestamp_start,
  uint64_t           timestamp_end,
  MidiEvent *        ev);

/**
 * @return Whether a MIDI event was dequeued or
 * not.
 */
int
windows_mme_device_dequeue_midi_event (
  WindowsMmeDevice * self,
  uint64_t           timestamp_start,
  uint64_t           timestamp_end,
  uint64_t *         timestamp,
  uint8_t *          midi_data,
  size_t *           data_size);

/**
 * Prints info about the device at the given ID
 * (index).
 */
void
windows_mme_device_print_info (WindowsMmeDevice * dev);

void
windows_mme_device_free (WindowsMmeDevice * dev);

/**
 * @}
 */

#  endif
#endif // _WOE32
