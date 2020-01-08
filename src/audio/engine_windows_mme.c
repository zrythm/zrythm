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

#include "config.h"

#ifdef _WIN32

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_windows_mme.h"
#include "audio/master_track.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "project.h"
#include "utils/ui.h"
#include "utils/windows_errors.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * MIDI in signal handler.
 *
 * The meaning of the dwParam1 and dwParam2 parameters
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
static void
midi_in_cb (
  HMIDIIN   hMidiIn,
  UINT      wMsg,
  DWORD_PTR dwInstance,
  DWORD_PTR dwParam1,
  DWORD_PTR dwParam2)
{
  g_message ("windows MIDI callback called");
  switch (wMsg)
    {
    case MIM_OPEN:
      g_message ("MIM_OPEN");
      break;
    case MIM_CLOSE:
      g_message ("MIM_CLOSE");
      break;
    case MIM_DATA:
      g_message ("MIM_DATA");
      /* dwParam1 = dwMidiMessage
         dwParam2 = dwTimestamp */
      break;
    case MIM_LONGDATA:
      g_message ("MIM_LONGDATA");
      break;
    case MIM_ERROR:
      g_message ("MIM_ERROR");
      break;
    case MIM_LONGERROR:
      g_message ("MIM_LONGERROR");
      break;
    case MIM_MOREDATA:
      g_message ("MIM_MOREDATA");
      break;
    default:
      g_message ("unknown message");
      break;
    }
}

/**
 * Set up Port Audio.
 */
int
engine_windows_mme_setup (
  AudioEngine * self,
  int           loading)
{
  UINT num_devs = midiInGetNumDevs ();

  for (UINT i = 0; i < num_devs; i++)
    {
      MIDIINCAPS caps;
      MMRESULT ret =
        midiInGetDevCaps (
          i, &caps, sizeof (MIDIINCAPS));
      if (ret != MMSYSERR_NOERROR)
        {
          windows_errors_print_mmresult (ret);
          return -1;
        }

      /* TODO see https://docs.microsoft.com/en-us/windows/win32/api/mmeapi/ns-mmeapi-midiincaps for what
       * to do with the midi caps */
      g_message (
        "MIDI in device detected:\n"
        "Manufacturer ID: %u \n"
        "Product identifier: %u \n"
        "Driver version: %u . %u (%x)\n"
        "Product name: %s",
        caps.wMid, caps.wPid,
        (caps.vDriverVersion >> 8),
        (caps.vDriverVersion & 0xff),
        caps.vDriverVersion, caps.szPname);

      ret =
        midiInOpen (
          &self->midi_in_handles[i], i,
	  (DWORD_PTR) midi_in_cb,
          (DWORD_PTR) self, CALLBACK_FUNCTION);
      if (ret != MMSYSERR_NOERROR)
        {
          windows_errors_print_mmresult (ret);
          return -1;
        }

      self->midi_headers[i].lpData =
	      calloc (3, sizeof (char));
      self->midi_headers[i].dwBufferLength =
	      3 * sizeof (char);
      self->midi_headers[i].dwFlags = 0;

      ret =
        midiInPrepareHeader (
          self->midi_in_handles[i],
	  &self->midi_headers[i],
	  sizeof (MIDIHDR));
      if (ret != MMSYSERR_NOERROR)
        {
          windows_errors_print_mmresult (ret);
          return -1;
        }
      ret =
        midiInAddBuffer (
          AUDIO_ENGINE->midi_in_handles[i],
	  &AUDIO_ENGINE->midi_headers[i],
	  sizeof (MIDIHDR));
      if (ret != MMSYSERR_NOERROR)
        {
          windows_errors_print_mmresult (ret);
        }

      self->num_midi_in_handles++;
    }

  return 0;
}


/**
 * Tests if PortAudio is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_windows_mme_test (
  GtkWindow * win)
{
  return 0;
}

/**
 * Closes Port Audio.
 */
int
engine_windows_mme_tear_down (
  AudioEngine * engine)
{
	return 0;
}

#endif // _WIN32
