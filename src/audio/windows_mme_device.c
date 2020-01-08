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

#include "audio/engine_windows_mme.h"
#include "audio/windows_mmcss.h"
#include "audio/windows_mme_device.h"

WindowsMmeDevice *
windows_mme_device_new (
  int           input,
  int           index)
{
  MIDIINCAPS in_caps;
  MIDIOUTCAPS out_caps;
  MMRESULT ret;
  if (input)
    ret =
      midiInGetDevCaps (
        i, &in_caps, sizeof (MIDIINCAPS));
  else
    ret =
      midiOutGetDevCaps (
        i, &out_caps, sizeof (MIDIOUTCAPS));
  if (ret != MMSYSERR_NOERROR)
    {
      engine_windows_mme_print_error (
        ret, input);
      return -1;
    }

  WindowsMmeDevice * self =
    calloc (1, sizeof (WindowsMmeDevice));
  self->is_input = input;
  self->id = index;
  if (input)
    {
      self->manufacturer_id = in_caps.wMid;
      self->product_id = in_caps.wPid;
      self->driver_ver_major =
        in_caps.vDriverVersion >> 8;
      self->driver_ver_minor =
        in_caps.vDriverVersion & 0xff;
      self->name =
        g_strdup (in_caps.szPname);
    }
  else
    {
      self->manufacturer_id = out_caps.wMid;
      self->product_id = out_caps.wPid;
      self->driver_ver_major =
        out_caps.vDriverVersion >> 8;
      self->driver_ver_minor =
        out_caps.vDriverVersion & 0xff;
      self->name =
        g_strdup (out_caps.szPname);
    }

  self->midi_ring =
    zix_ring_new (
      sizeof (uint8_t) * (size_t) MIDI_BUFFER_SIZE);

  return self;
}

static int
enqueue_midi_msg (
  WindowsMmeDevice * self,
  const uint8_t * midi_data,
  size_t          data_size,
  uint32_t        timestamp)
{
  const uint32_t total_size =
    sizeof (MidiEventHeader) + data_size;

  if (data_size == 0)
    {
      g_critical ("zero length midi data");
      return -1;
    }

  if (zix_ring_write_space (
        self->midi_ring) < total_size)
    {
      g_critical (
        "WinMMEMidiInput: ring buffer overflow");
      return -1;
    }

  // don't use winmme timestamps for now
  uint64_t ts = g_get_monotonic_time ();

  g_message (
    "Enqueing MIDI data device: %s "
    "with timestamp: %ul and size %d",
      self->name, ts, data_size);

  struct MidiEventHeader h = {
    .h = ts, .size = data_size,
  };
  zix_ring_write (
    self->midi_ring,
    (uint8_t *) &h, sizeof (MidiEventHeader));
  zix_ring_write (
    self->midi_ring, midi_data, data_size);

  return 0;
}

static void
handle_short_msg (
  WindowsMmeDevice * self,
  const uint8_t *    midi_data,
  uint32_t           timestamp)
{
  int length = midi_get_msg_length (midi_data[0]);

  if (length == 0 || length == -1)
    {
      g_critical (
        "midi input driver sent an invalid midi "
        "message");
      return;
    }

  enqueue_midi_msg (
    self, midi_data, length, timestamp);
}

static void
handle_sysex_msg (
  WindowsMmeDevice * self,
  MIDIHDR * const    midi_header,
  uint32_t           timestamp)
{
  size_t byte_count = midi_header->dwBytesRecorded;

  if (byte_count == 0)
    {
      if ((midi_header->dwFlags & WHDR_DONE) != 0)
        {
          g_message ("WinMME: In midi reset");
          // unprepare handled by close
        }
      else
        {
          g_critical (
            "ERROR: WinMME driver has returned sysex "
            "header to us with no bytes");
        }
      return;
    }

  uint8_t * data = (uint8_t *) midi_header->lpData;

  g_message (
    "WinMME sysex flags: %u", midi_header->dwFlags);

  if ((data[0] != 0xf0) ||
      (data[byte_count - 1] != 0xf7))
    {
      g_message (
        "Discarding %u byte sysex chunk", byte_count);
    }
  else
    {
      enqueue_midi_msg (
        self, data, byte_count, timestamp);
    }

  g_message (
    "Adding sysex buffer back to WinMME buffer pool");

  midi_header->dwFlags = 0;
  midi_header->dwBytesRecorded = 0;

  MMRESULT result =
    midiInPrepareHeader (
      self->in_handle, midi_header, sizeof (MIDIHDR));

  if (result != MMSYSERR_NOERROR)
    {
      g_critical ("Unable to prepare header:");
      engine_windows_mme_print_error (
        result, self->is_input);
      return;
    }

  result =
    midiInAddBuffer (
      self->in_handle, midi_header, sizeof(MIDIHDR));
  if (result != MMSYSERR_NOERROR)
    {
      g_critical (
        "Unable to add sysex buffer to buffer pool:");
      engine_windows_mme_print_error (
        result, self->is_input);
    }
}

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
{
  EngineWindowsMmeDevice * self =
    (EngineWindowsMmeDevice *) dwInstance;

#ifdef USE_MMCSS_THREAD_PRIORITIES

  static HANDLE input_thread = GetCurrentThread ();
  static int priority_boosted = 0;

#if 0 // GetThreadId() is Vista or later only.
  if (input_thread != GetCurrentThread ()) {
    DWORD otid = GetThreadId (input_thread);
    DWORD ntid = GetThreadId (GetCurrentThread ());
    // There was a reference on the internet
    // somewhere that it is possible
    // for the callback to come from different
    // threads(thread pool) this
    // could be problematic but I haven't seen this
    // behaviour yet
    DEBUG_THREADS (string_compose (
        "WinMME input Thread ID Changed: was %1,
        now %2\n", otid, ntid));
  }
#endif

  HANDLE task_handle;

  if (!priority_boosted)
    {
      windows_mmcss_set_thread_characteristics (
        "Pro Audio", &task_handle);
      windows_mmcss_set_thread_priority (
        task_handle, AVRT_PRIORITY_HIGH);
    priority_boosted = 1;
  }
#endif

  switch (wMsg)
    {
    case MIM_OPEN:
      g_message (
        "WinMME: [%s] opened", self->name);
      break;
    case MIM_CLOSE:
      g_message (
        "WinMME: [%s] closed", self->name);
      break;
    case MIM_MOREDATA:
      g_message (
        "WinMME: [%s] more data ..", self->name);
      // passing MIDI_IO_STATUS to midiInOpen means
      // that MIM_MOREDATA
      // will be sent when the callback isn't
      // processing MIM_DATA messages
      // fast enough to keep up with messages
      // arriving at input device
      // driver. I'm not sure what could be done
      // differently if that occurs
      // so just handle MIM_DATA as per normal
    case MIM_DATA:
      g_message (
        "WinMME: [%s] short msg at %u",
        self->name,
        (uint32_t) timestamp);
      handle_short_msg (
        self, (const uint8_t *) &midi_msg,
        (uint32_t) timestamp);
      break;
    case MIM_LONGDATA:
      g_message (
        "WinMME: [%s] long msg at %u",
        self->name,
        (uint32_t) timestamp);
      handle_sysex_msg (
        self, (MIDIHDR *) midi_msg,
        (uint32_t) timestamp);
      break;
    case MIM_ERROR:
      g_warning (
        "WinMME: Driver %s sent an invalid MIDI "
        "message", self->name);
      break;
    case MIM_LONGERROR:
      g_warning (
        "WinMME: Driver %s sent an invalid or "
        "incomplete SYSEX message", self->name);
      break;
    default:
      g_warning (
        "WinMME: Driver %s sent an unknown message",
        self->name);
      break;
    }
}

/**
 * Opens a device allocated with
 * engine_windows_mme_device_new().
 *
 * @return Non-zero if error.
 */
int
windows_mme_device_open (
  WindowsMmeDevice * dev)
{
  if (dev->is_input)
    {
      MMRESULT result =
        midiInOpen (
          &dev->in_handle, dev->id,
          (DWORD_PTR) input_cb,
          (DWORD_PTR) dev,
          CALLBACK_FUNCTION | MIDI_IO_STATUS);
      if (result != MMSYSERR_NOERROR)
        {
          engine_windows_mme_print_error (
            result, dev->is_input);
          return -1;
        }
      g_message (
        "Opened MIDI in device at index %u",
        dev->id);
    }
  return 0;
}

/**
 * Prints info about the device at the given ID
 * (index).
 */
void
windows_mme_device_print_info (
  WindowsMmeDevice * dev);
{
  g_return_if_fail (dev);

  g_message (
    "Windows MME Device:\n"
    "Manufacturer ID: %u \n"
    "Product identifier: %u \n"
    "Driver version: %u.%u \n"
    "Product name: %s",
    dev->manufacturer_id, dev->product_id,
    dev->driver_ver_major, dev->driver_ver_minor,
    dev->name);
    }
}

#endif // _WIN32
