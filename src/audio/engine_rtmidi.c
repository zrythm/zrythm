/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifdef HAVE_RTMIDI

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_rtmidi.h"
#include "audio/ext_port.h"
#include "audio/midi.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "audio/routing.h"
#include "audio/transport.h"
#include "gui/widgets/main_window.h"
#include "plugins/plugin.h"
#include "plugins/lv2_plugin.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>
#include <glib/gi18n.h>

#include <rtmidi/rtmidi_c.h>

/**
 * Midi message callback.
 *
 * @param timestamp The time at which the message
 *   has been received.
 * @param message The midi message.
 */
static void
midi_in_cb (
  double                timestamp,
  const unsigned char * message,
  size_t                message_size,
  AudioEngine *         self)
{
  g_message ("message received of size %lu",
    message_size);

  /* set the event to the engine's MIDI in */
  /* TODO guard */
}

/**
 * Sets up the MIDI engine to use Rtmidi.
 *
 * @param loading Loading a Project or not.
 */
int
engine_rtmidi_setup (
  AudioEngine * self,
  int           loading)
{
  self->midi_buf_size = 4096;
  (void) midi_in_cb;

  g_message ("Rtmidi set up");

  return 0;
}

static RtMidiInPtr
create_port (
  AudioEngine * self)
{
  RtMidiInPtr midi_in =
    rtmidi_in_create (
#ifdef _WOE32
      RTMIDI_API_WINDOWS_MM,
#elif defined(__APPLE__)
      RTMIDI_API_MACOSX_CORE,
#else
      RTMIDI_API_LINUX_ALSA,
#endif
      "Zrythm",
      self->midi_buf_size);
  return midi_in;
}

static void
free_port (RtMidiInPtr port)
{
  rtmidi_in_free (port);
}

/**
 * Gets the number of input ports (devices).
 */
unsigned int
engine_rtmidi_get_num_in_ports (
  AudioEngine * self)
{
  RtMidiPtr ptr = create_port (self);
  unsigned int num_ports =
    rtmidi_get_port_count (ptr);
  free_port (ptr);
  return num_ports;
}

/**
 * Creates an input port, optinally opening it with
 * the given device ID (from rtmidi port count) and
 * label.
 */
RtMidiPtr
engine_rtmidi_create_in_port (
  AudioEngine * self,
  int           open_port,
  unsigned int  device_id,
  const char *  label)
{
  RtMidiInPtr midi_in =
    rtmidi_in_create (
#ifdef _WOE32
      RTMIDI_API_WINDOWS_MM,
#elif defined(__APPLE__)
      RTMIDI_API_MACOSX_CORE,
#else
      RTMIDI_API_LINUX_ALSA,
#endif
      "Zrythm",
      self->midi_buf_size);

  if (open_port)
    {
      /* don't ignore any messages */
      /*rtmidi_in_ignore_types (midi_in, 0, 0, 0);*/

      char lbl[800];
      sprintf (lbl, "%s [%u]", label, device_id);
      rtmidi_open_port (midi_in, device_id, lbl);
    }

  return midi_in;
}

/**
 * Tests if the backend is working properly.
 *
 * Returns 0 if ok, non-null if has errors.
 *
 * If win is not null, it displays error messages
 * to it.
 */
int
engine_rtmidi_test (
  GtkWindow * win)
{
  return 0;
}

void
engine_rtmidi_tear_down (
  AudioEngine * self)
{
  /* init semaphore */
  zix_sem_init (
    &self->port_operation_lock, 1);
}

int
engine_rtmidi_activate (
  AudioEngine * self)
{
  return 0;
}

#endif /* HAVE_RTMIDI */
