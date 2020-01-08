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

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_windows_mme.h"
#include "audio/master_track.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "audio/windows_mme_device.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

/**
 * Returns the number of MIDI devices available.
 *
 * @param input 1 for input, 0 for output.
 */
int
engine_windows_mme_get_num_devices (
  int input)
{
  if (input)
    return (int) midiInGetNumDevs ();
  else
    return (int) midiOutGetNumDevs ();
}

/**
 * Gets the error text for a MIDI input error.
 *
 * @return 0 if successful, non-zero if failed.
 */
int
engine_windows_mme_get_error (
  MMRESULT error_code,
  int      input,
  char *   buf,
  int      buf_size)
{
  g_return_val_if_fail (
    error_code != MMSYSERR_NOERROR, -1);

  MMRESULT result;
  if (input)
  {
    result =
    midiInGetErrorText (
      error_code, buf, (UINT) buf_size);
  }
  else
  {
    result =
    midiOutGetErrorText (
      error_code, buf, (UINT) buf_size);
  }
  g_return_val_if_fail (
    result == MMSYSERR_NOERROR, -1);

  return 0;
}

/**
 * Prints the error in the log.
 */
void
engine_windows_mme_print_error (
  MMRESULT error_code,
  int      input)
{
  g_return_if_fail (
    error_code != MMSYSERR_NOERROR);

  char msg[600];
  int ret =
    engine_windows_mme_get_error (
      error_code, input, msg, 600);
  if (!ret)
    {
      g_critical ("Windows MME error: %s", msg);
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
  int num_devs =
    engine_windows_mme_get_num_devices (1);

  for (int i = 0; i < num_devs; i++)
    {
      WindowsMmeDevice * dev =
        windows_mme_device_new (
            1, i);
      g_return_val_if_fail (dev, -1);
      int ret =
      windows_mme_device_open (
          dev);
      g_return_val_if_fail (ret == 0, -1);
    }

  num_devs =
    engine_windows_mme_get_num_devices (0);

  for (int i = 0; i < num_devs; i++)
    {
      WindowsMmeDevice * dev =
        windows_mme_device_new (
            0, i);
      g_return_val_if_fail (dev, -1);
      g_message ("found midi output device %s",
          dev->name);
      int ret =
      windows_mme_device_open (
          dev);
      g_return_val_if_fail (ret == 0, -1);
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
