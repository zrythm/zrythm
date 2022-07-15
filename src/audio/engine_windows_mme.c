// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
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

#  include "audio/channel.h"
#  include "audio/engine.h"
#  include "audio/engine_windows_mme.h"
#  include "audio/master_track.h"
#  include "audio/port.h"
#  include "audio/router.h"
#  include "audio/windows_mmcss.h"
#  include "audio/windows_mme_device.h"
#  include "project.h"
#  include "settings/settings.h"
#  include "utils/ui.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

/**
 * Returns the number of MIDI devices available.
 *
 * @param input 1 for input, 0 for output.
 */
int
engine_windows_mme_get_num_devices (int input)
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
  g_return_val_if_fail (error_code != MMSYSERR_NOERROR, -1);

  MMRESULT result;
  if (input)
    {
      result =
        midiInGetErrorText (error_code, buf, (UINT) buf_size);
    }
  else
    {
      result = midiOutGetErrorText (
        error_code, buf, (UINT) buf_size);
    }
  g_return_val_if_fail (result == MMSYSERR_NOERROR, -1);

  return 0;
}

/**
 * Prints the error in the log.
 */
void
engine_windows_mme_print_error (MMRESULT error_code, int input)
{
  g_return_if_fail (error_code != MMSYSERR_NOERROR);

  char msg[600];
  int  ret = engine_windows_mme_get_error (
     error_code, input, msg, 600);
  if (!ret)
    {
      g_critical ("Windows MME error: %s", msg);
    }
}

/**
 * Starts all previously scanned devices.
 */
int
engine_windows_mme_activate (AudioEngine * self, bool activate)
{
  if (activate)
    {
      g_message ("%s: activating...", __func__);
    }
  else
    {
      g_message ("%s: deactivating...", __func__);
    }

  for (int i = 0; i < self->num_mme_in_devs; i++)
    {
      WindowsMmeDevice * dev = self->mme_in_devs[i];

      if (activate)
        {
          g_return_val_if_fail (
            dev->opened == 1 && dev->started == 0, -1);
          int ret = windows_mme_device_start (dev);
          g_return_val_if_fail (ret == 0, -1);
        }
      else
        {
          windows_mme_device_stop (dev);
        }
    }
  for (int i = 0; i < self->num_mme_out_devs; i++)
    {
      /* skip for now */
      break;
      WindowsMmeDevice * dev = self->mme_out_devs[i];
      if (activate)
        {
          g_return_val_if_fail (
            dev->opened == 1 && dev->started == 0, -1);
          int ret = windows_mme_device_start (dev);
          g_return_val_if_fail (ret == 0, -1);
        }
      else
        {
          windows_mme_device_stop (dev);
        }
    }

  g_message ("%s: done", __func__);

  return 0;
}

/**
 * Rescans for MIDI devices, opens them and keeps
 * track of them.
 *
 * @param start Whether to start receiving events
 *   on the devices or not.
 */
int
engine_windows_mme_rescan_devices (AudioEngine * self, int start)
{
  for (int i = 0; i < self->num_mme_in_devs; i++)
    {
      WindowsMmeDevice * dev = self->mme_in_devs[i];

      if (dev->started)
        windows_mme_device_stop (dev);
      if (dev->opened)
        windows_mme_device_close (dev, 0);
      windows_mme_device_free (dev);
    }
  for (int i = 0; i < self->num_mme_out_devs; i++)
    {
      WindowsMmeDevice * dev = self->mme_out_devs[i];

      if (dev->started)
        windows_mme_device_stop (dev);
      if (dev->opened)
        windows_mme_device_close (dev, 0);
      windows_mme_device_free (dev);
    }
  self->num_mme_in_devs = 0;
  self->num_mme_out_devs = 0;

  int num_devs = engine_windows_mme_get_num_devices (
    WINDOWS_MME_DEVICE_FLOW_INPUT);
  for (int i = 0; i < num_devs; i++)
    {
      WindowsMmeDevice * dev = windows_mme_device_new (
        WINDOWS_MME_DEVICE_FLOW_INPUT, i);
      g_return_val_if_fail (dev, -1);
      int ret = windows_mme_device_open (dev, 0);
      g_return_val_if_fail (ret == 0, -1);
      self->mme_in_devs[self->num_mme_in_devs++] = dev;
    }

  num_devs = engine_windows_mme_get_num_devices (
    WINDOWS_MME_DEVICE_FLOW_OUTPUT);

  for (int i = 0; i < num_devs; i++)
    {
      WindowsMmeDevice * dev = windows_mme_device_new (
        WINDOWS_MME_DEVICE_FLOW_OUTPUT, i);
      g_return_val_if_fail (dev, -1);
      g_message ("found midi output device %s", dev->name);
      int ret = windows_mme_device_open (dev, 0);
      g_return_val_if_fail (ret == 0, -1);
      self->mme_out_devs[self->num_mme_out_devs++] = dev;
    }

  if (start)
    {
      engine_windows_mme_activate (self, true);
    }

  return 0;
}

/**
 * Set up Port Audio.
 */
int
engine_windows_mme_setup (AudioEngine * self)
{
  g_message ("Initing MMCSS...");
  windows_mmcss_initialize ();

  g_message ("Rescanning MIDI devices...");
  engine_windows_mme_rescan_devices (self, 0);

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
engine_windows_mme_test (GtkWindow * win)
{
  return 0;
}

/**
 * Closes Port Audio.
 */
int
engine_windows_mme_tear_down (AudioEngine * engine)
{
  return 0;
}

#endif // _WOE32
