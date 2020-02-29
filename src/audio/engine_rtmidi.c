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

  g_message ("setting up Rtmidi...");

  self->rtmidi_in =
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

  rtmidi_open_virtual_port (
    self->rtmidi_in, "Zrythm MIDI in");
  rtmidi_open_virtual_port (
    self->rtmidi_in, "Zrythm MIDI in2");

  rtmidi_in_set_callback (
    self->rtmidi_in,
    (RtMidiCCallback) midi_in_cb, self);

  /* don't ignore any messages */
  rtmidi_in_ignore_types (
    self->rtmidi_in, 0, 0, 0);

  g_message ("Rtmidi set up");

  return 0;
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
