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
 * the Free Software Foundation, either version 3 of the License, or
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

#include "config.h"

#ifdef _WIN32

#ifndef __AUDIO_WINDOWS_MME_DEVICE_H__
#define __AUDIO_WINDOWS_MME_DEVICE_H__

#include <Windows.h>

typedef struct AudioEngine AudioEngine;

/**
 * @addtogroup audio
 *
 * @{
 */

static const uint32_t MIDI_BUFFER_SIZE = 32768;
static const uint32_t SYSEX_BUFFER_SIZE = 32768;

typedef struct WindowsMmeDevice
{
  /** 1 for input, 0 for output. */
  int           is_input;

  /** Index. */
  unsigned int  id;

  unsigned int  manufacturer_id;
  unsigned int  product_id;
  unsigned int  driver_ver_major;
  unsigned int  driver_ver_minor;

  char *        name;

  /* ---- INPUT ---- */
  HMIDIIN       in_handle;
  MIDIHDR       in_midi_header;

  /* ---- OUTPUT ---- */
  HMIDIOUT      out_handle;

  /** MIDI event ring buffer. */
  ZixRing *     midi_ring;

  uint8_t       sysex_buffer[SYSEX_BUFFER_SIZE];

} WindowsMmeDevice;

void

WindowsMmeDevice *
windows_mme_device_new (
  int           input,
  int           index);

/**
 * Opens a device allocated with
 * engine_windows_mme_device_new().
 *
 * @return Non-zero if error.
 */
int
windows_mme_device_open (
  WindowsMmeDevice * dev);

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
  DWORD_PTR dwParam2)

/**
 * Prints info about the device at the given ID
 * (index).
 */
void
windows_mme_print_device_info (
  WindowsMmeDevice * dev);

/**
 * @}
 */

#endif
#endif // _WIN32
