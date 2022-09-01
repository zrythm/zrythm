// SPDX-FileCopyrightText: © 2020, 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
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
 *
 * ---
 */

#include "zrythm-config.h"

#ifdef _WOE32

#  include "audio/engine.h"
#  include "audio/engine_windows_mme.h"
#  include "audio/midi_event.h"
#  include "audio/windows_mmcss.h"
#  include "audio/windows_mme_device.h"
#  include "project.h"

WindowsMmeDevice *
windows_mme_device_new (int input, int index)
{
  MIDIINCAPS  in_caps;
  MIDIOUTCAPS out_caps;
  MMRESULT    ret;
  if (input)
    {
      ret = midiInGetDevCaps (
        (UINT_PTR) index, &in_caps, sizeof (MIDIINCAPS));
    }
  else
    {
      ret = midiOutGetDevCaps (
        (UINT_PTR) index, &out_caps, sizeof (MIDIOUTCAPS));
    }
  if (ret != MMSYSERR_NOERROR)
    {
      engine_windows_mme_print_error (ret, input);
      return NULL;
    }

  WindowsMmeDevice * self =
    calloc (1, sizeof (WindowsMmeDevice));
  self->is_input = input;
  self->id = index;
  if (input)
    {
      self->manufacturer_id = in_caps.wMid;
      self->product_id = in_caps.wPid;
      self->driver_ver_major = in_caps.vDriverVersion >> 8;
      self->driver_ver_minor = in_caps.vDriverVersion & 0xff;
      self->name = g_strdup (in_caps.szPname);
    }
  else
    {
      self->manufacturer_id = out_caps.wMid;
      self->product_id = out_caps.wPid;
      self->driver_ver_major = out_caps.vDriverVersion >> 8;
      self->driver_ver_minor = out_caps.vDriverVersion & 0xff;
      self->name = g_strdup (out_caps.szPname);
    }

  self->midi_ring = zix_ring_new (
    zix_default_allocator (),
    sizeof (uint8_t) * (size_t) MIDI_BUFFER_SIZE);

  return self;
}

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
  MidiEvent *        ev)
{
  g_return_val_if_fail (self, 0);

  uint64_t timestamp;
  size_t   data_size;
  int      ret = windows_mme_device_dequeue_midi_event (
         self, timestamp_start, timestamp_end, &timestamp,
         ev->raw_buffer, &data_size);
  if (!ret)
    return 0;

  /* calculate the time in frames */
  uint64_t timestamp_interval =
    timestamp_end - timestamp_start;
  int64_t timestamp_offset =
    (int64_t) timestamp - (int64_t) timestamp_start;
  double ratio =
    (double) timestamp_offset / (double) timestamp_interval;
#  if 0
  if (ratio < 0.001 || ratio >= 1.0)
  {
    g_warning ("Event time is not within this cycle, skipping");
    return 0;
  }
#  endif

  ev->time =
    (midi_time_t) (ratio * (double) AUDIO_ENGINE->block_length);

  return 1;
}

/**
 * Dequeues a MIDI event.
 *
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
  size_t *           data_size)
{
  g_return_val_if_fail (self, 0);

  const uint32_t read_space =
    zix_ring_read_space (self->midi_ring);
  if (read_space <= sizeof (MidiEventHeader))
    {
      /* defer */
      return 0;
    }

  MidiEventHeader h = { 0, 0 };

  zix_ring_peek (self->midi_ring, &h, sizeof (h));
  g_return_val_if_fail (h.time < timestamp_end, 0);

  /* read event header */
  zix_ring_read (self->midi_ring, &h, sizeof (h));

  /* read event body */
  zix_ring_read (self->midi_ring, midi_data, h.size);
  *timestamp = h.time;
  *data_size = h.size;

  if (DEBUGGING)
    {
      g_debug (
        "Denqueing MIDI data device: %s "
        "with timestamp: %llu and size %lld",
        self->name, h.time, h.size);
    }
  g_return_val_if_fail (h.size >= 1 && h.size <= 3, 1);
  g_return_val_if_fail (
    read_space >= sizeof (MidiEventHeader) + h.size, 1);

  if (h.time >= timestamp_end)
    {
      /* this will never be reached but keep just
       * for reference */
      g_critical (
        "MMEMidiInput event %f(ms) early",
        (h.time - timestamp_end) * 1e-3);
      return 1;
    }
  else if (h.time < timestamp_start)
    {
      /* this is a bug that needs to be fixed some
       * time, but for now just clamp */
      g_warning (
        "MMEMidiInput event %f(ms) late (it should "
        "have been included in the previous cycle). "
        "setting its time to 0 in the current cycle "
        "to compensate",
        (timestamp_start - h.time) * 1e-3);
      *timestamp = timestamp_start;
      return 1;
    }

  g_return_val_if_fail (h.size > 0, 0);
  if (h.size > *data_size)
    {
      g_warning ("MMEMidiInput MIDI event too large");
      return 1;
    }

  return 1;
}

static int
enqueue_midi_msg (
  WindowsMmeDevice * self,
  const uint8_t *    midi_data,
  size_t             data_size,
  uint32_t           timestamp)
{
  const uint32_t total_size =
    sizeof (MidiEventHeader) + data_size;

  if (data_size == 0)
    {
      g_critical ("zero length midi data");
      return -1;
    }

  if (zix_ring_write_space (self->midi_ring) < total_size)
    {
      g_critical ("WinMMEMidiInput: ring buffer overflow");
      return -1;
    }

  // don't use winmme timestamps for now
  uint64_t ts = (uint64_t) g_get_monotonic_time ();

  if (DEBUGGING)
    {
      g_debug (
        "Enqueuing MIDI data device: %s "
        "with timestamp: %llu and size %lld",
        self->name, ts, data_size);
    }

  MidiEventHeader h = {
    .time = ts,
    .size = data_size,
  };
  zix_ring_write (
    self->midi_ring, (uint8_t *) &h, sizeof (MidiEventHeader));
  zix_ring_write (self->midi_ring, midi_data, data_size);

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
    self, midi_data, (size_t) length, timestamp);
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
            "ERROR: WinMME driver has returned "
            "sysex header to us with no bytes");
        }
      return;
    }

  uint8_t * data = (uint8_t *) midi_header->lpData;

  g_message ("WinMME sysex flags: %lu", midi_header->dwFlags);

  if ((data[0] != 0xf0) || (data[byte_count - 1] != 0xf7))
    {
      g_message (
        "Discarding %llu byte sysex chunk", byte_count);
    }
  else
    {
      enqueue_midi_msg (self, data, byte_count, timestamp);
    }

  g_debug (
    "Adding sysex buffer back to WinMME buffer "
    "pool");

  midi_header->dwFlags = 0;
  midi_header->dwBytesRecorded = 0;

  MMRESULT result = midiInPrepareHeader (
    self->in_handle, midi_header, sizeof (MIDIHDR));

  if (result != MMSYSERR_NOERROR)
    {
      g_critical ("Unable to prepare header:");
      engine_windows_mme_print_error (result, self->is_input);
      return;
    }

  result = midiInAddBuffer (
    self->in_handle, midi_header, sizeof (MIDIHDR));
  if (result != MMSYSERR_NOERROR)
    {
      g_critical (
        "Unable to add sysex buffer to buffer pool:");
      engine_windows_mme_print_error (result, self->is_input);
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
  WindowsMmeDevice * self = (WindowsMmeDevice *) dwInstance;
  DWORD_PTR          midi_msg = dwParam1;
  DWORD              timestamp = dwParam2;

#  ifdef USE_MMCSS_THREAD_PRIORITIES

  // static HANDLE input_thread = GetCurrentThread ();
  static int priority_boosted = 0;

#    if 0 // GetThreadId() is Vista or later only.
  if (input_thread != GetCurrentThread ()) {
    DWORD otid = GetThreadId (input_thread);
    DWORD ntid = GetThreadId (GetCurrentThread ());
    // There was a reference on the internet
    // somewhere that it is possible
    // for the callback to come from different
    // threads(thread pool) this
    // could be problematic but I haven't seen this
    // behaviour yet
    g_message (
        "WinMME input Thread ID Changed: was %d"
        "now %d\n", otid, ntid);
  }
#    endif

  HANDLE task_handle;

  if (!priority_boosted)
    {
      windows_mmcss_set_thread_characteristics (
        "Pro Audio", &task_handle);
      windows_mmcss_set_thread_priority (
        task_handle, AVRT_PRIORITY_HIGH);
      priority_boosted = 1;
    }
#  endif

  switch (wMsg)
    {
    case MIM_OPEN:
      g_message ("WinMME: [%s] opened", self->name);
      break;
    case MIM_CLOSE:
      g_message ("WinMME: [%s] closed", self->name);
      break;
    case MIM_MOREDATA:
      g_message ("WinMME: [%s] more data ..", self->name);
      // passing MIDI_IO_STATUS to midiInOpen means
      // that MIM_MOREDATA
      // will be sent when the callback isn't
      // processing MIM_DATA messages
      // fast enough to keep up with messages
      // arriving at input device
      // driver. I'm not sure what could be done
      // differently if that occurs
      // so just handle MIM_DATA as per normal
      handle_short_msg (
        self, (const uint8_t *) &midi_msg,
        (uint32_t) timestamp);
      break;
    case MIM_DATA:
      if (DEBUGGING)
        {
          g_debug (
            "WinMME: [%s] short msg at %u", self->name,
            (uint32_t) timestamp);
        }
      handle_short_msg (
        self, (const uint8_t *) &midi_msg,
        (uint32_t) timestamp);
      break;
    case MIM_LONGDATA:
      if (DEBUGGING)
        {
          g_debug (
            "WinMME: [%s] long msg at %u", self->name,
            (uint32_t) timestamp);
        }
      handle_sysex_msg (
        self, (MIDIHDR *) midi_msg, (uint32_t) timestamp);
      break;
    case MIM_ERROR:
      g_warning (
        "WinMME: Driver %s sent an invalid MIDI "
        "message",
        self->name);
      break;
    case MIM_LONGERROR:
      g_warning (
        "WinMME: Driver %s sent an invalid or "
        "incomplete SYSEX message",
        self->name);
      break;
    default:
      g_warning (
        "WinMME: Driver %s sent an unknown message",
        self->name);
      break;
    }
}

static int
add_sysex_buffer (WindowsMmeDevice * self)
{
  self->sysex_header.dwBufferLength = SYSEX_BUFFER_SIZE;
  self->sysex_header.dwFlags = 0;
  self->sysex_header.dwBytesRecorded = 0;
  self->sysex_header.lpData = (LPSTR) self->sysex_buffer;

  MMRESULT result = midiInPrepareHeader (
    self->in_handle, &self->sysex_header, sizeof (MIDIHDR));
  if (result != MMSYSERR_NOERROR)
    {
      engine_windows_mme_print_error (result, 1);
      return -1;
    }

  result = midiInAddBuffer (
    self->in_handle, &self->sysex_header, sizeof (MIDIHDR));
  if (result != MMSYSERR_NOERROR)
    {
      engine_windows_mme_print_error (result, 1);
      return -1;
    }
  else
    {
      g_message ("Added Initial WinMME sysex buffer");
    }

  return 0;
}

/**
 * Opens a device allocated with
 * engine_windows_mme_device_new().
 *
 * @param start Also start the device.
 *
 * @return Non-zero if error.
 */
int
windows_mme_device_open (WindowsMmeDevice * self, int start)
{
  if (self->is_input)
    {
      MMRESULT result;
      if (self->is_input)
        result = midiInOpen (
          &self->in_handle, (UINT) self->id,
          (DWORD_PTR) windows_mme_device_input_cb,
          (DWORD_PTR) self,
          CALLBACK_FUNCTION | MIDI_IO_STATUS);
      else
        {
          /* TODO */
        }
      if (result != MMSYSERR_NOERROR)
        {
          engine_windows_mme_print_error (
            result, self->is_input);
          return -1;
        }

      if (add_sysex_buffer (self))
        {
          windows_mme_device_close (self, 0);
          g_return_val_if_reached (-1);
        }

      g_message (
        "Opened MIDI in device at index %u", self->id);
      self->opened = 1;
    }

  if (start)
    return windows_mme_device_start (self);

  return 0;
}

int
windows_mme_device_start (WindowsMmeDevice * self)
{
  g_return_val_if_fail (self, -1);

  if (!self->started)
    {
      MMRESULT result;
      if (self->is_input)
        result = midiInStart (self->in_handle);
      else
        {
          /* TODO */
          result = MMSYSERR_NOERROR;
        }
      self->started = (result == MMSYSERR_NOERROR);
      if (!self->started)
        {
          engine_windows_mme_print_error (
            result, self->is_input);
          return -1;
        }
      else
        {
          g_message (
            "WinMMEMidiInput: device %s started", self->name);
        }
    }

  return 0;
}

int
windows_mme_device_stop (WindowsMmeDevice * self)
{
  if (self->started)
    {
      MMRESULT result;
      if (self->is_input)
        result = midiInStop (self->in_handle);
      else
        {
          /* TODO */
          result = MMSYSERR_NOERROR;
        }
      self->started = 0;
      if (result != MMSYSERR_NOERROR)
        {
          engine_windows_mme_print_error (
            result, self->is_input);
          return -1;
        }
      else
        {
          g_message (
            "WinMMEMidiInput: device %s stopped", self->name);
        }
    }

  return 0;
}

/**
 * Close the WindowsMmeDevice.
 *
 * @param free Also free the memory.
 */
int
windows_mme_device_close (WindowsMmeDevice * self, int free)
{
  int has_error = 0;

  MMRESULT result;
  if (self->is_input)
    result = midiInReset (self->in_handle);
  else
    result = midiOutReset (self->out_handle);
  if (result != MMSYSERR_NOERROR)
    {
      engine_windows_mme_print_error (result, self->is_input);
      has_error = 1;
    }

  if (self->is_input)
    result = midiInUnprepareHeader (
      self->in_handle, &self->sysex_header, sizeof (MIDIHDR));
  else
    result = midiOutUnprepareHeader (
      self->out_handle, &self->sysex_header, sizeof (MIDIHDR));
  if (result != MMSYSERR_NOERROR)
    {
      engine_windows_mme_print_error (result, self->is_input);
      has_error = 1;
    }

  if (self->is_input)
    result = midiInClose (self->in_handle);
  else
    result = midiOutClose (self->out_handle);
  if (result != MMSYSERR_NOERROR)
    {
      engine_windows_mme_print_error (result, self->is_input);
      has_error = 1;
    }

  self->in_handle = NULL;
  self->out_handle = NULL;

  if (has_error)
    {
      g_warning (
        "Unable to Close MIDI device: %s", self->name);
    }
  else
    {
      g_message ("Closed MIDI device: %s", self->name);
      self->opened = 0;
    }

  if (free)
    {
      windows_mme_device_free (self);
    }

  return has_error;
}

/**
 * Prints info about the device at the given ID
 * (index).
 */
void
windows_mme_device_print_info (WindowsMmeDevice * dev)
{
  g_return_if_fail (dev);

  g_message (
    "Windows MME Device:\n"
    "Manufacturer ID: %u \n"
    "Product identifier: %u \n"
    "Driver version: %u.%u \n"
    "Product name: %s",
    dev->manufacturer_id, dev->product_id,
    dev->driver_ver_major, dev->driver_ver_minor, dev->name);
}

void
windows_mme_device_free (WindowsMmeDevice * self)
{
  g_return_if_fail (self);
  if (self->started)
    {
      g_critical ("MME: Cannot free a running device");
      return;
    }
  if (self->opened)
    {
      g_critical (
        "MME: Please close [%s] before "
        "freeing",
        self->name);
      return;
    }

  if (self->name)
    g_free (self->name);
  if (self->midi_ring)
    zix_ring_free (self->midi_ring);

  free (self);
}

#endif // _WOE32
