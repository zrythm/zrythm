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

#include "zrythm-config.h"

#ifdef HAVE_RTMIDI

#include "audio/engine.h"
#include "audio/port.h"
#include "audio/rtmidi_device.h"
#include "project.h"

#include <gtk/gtk.h>

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
  RtMidiDevice *        self)
{
  zix_sem_wait (&self->midi_ring_sem);

  /* generate timestamp */
  gint64 cur_time = g_get_monotonic_time ();
  gint64 ts =
    cur_time - self->port->last_midi_dequeue;
  g_return_if_fail (ts >= 0);
  char portname[900];
  port_get_full_designation (self->port, portname);
  g_message (
    "[%s] message received of size %lu at %ld",
    portname, message_size, ts);

  /* add to ring buffer */
  MidiEventHeader h = {
    .time = (uint64_t) ts, .size = message_size,
  };
  zix_ring_write (
    self->midi_ring,
    (uint8_t *) &h, sizeof (MidiEventHeader));
  zix_ring_write (
    self->midi_ring, message, message_size);

  zix_sem_post (&self->midi_ring_sem);
}

RtMidiDevice *
rtmidi_device_new (
  int    is_input,
  unsigned int    device_id,
  Port * port)
{
  RtMidiDevice * self =
    calloc (1, sizeof (RtMidiDevice));

  self->is_input = is_input;
  self->id = device_id;
  self->port = port;
  if (is_input)
    {
      self->in_handle =
        rtmidi_in_create (
#ifdef _WOE32
          RTMIDI_API_WINDOWS_MM,
#elif defined(__APPLE__)
          RTMIDI_API_MACOSX_CORE,
#else
          RTMIDI_API_LINUX_ALSA,
#endif
          "Zrythm",
          AUDIO_ENGINE->midi_buf_size);
      if (!self->in_handle->ok)
        {
          g_warning (
            "An error occurred creating an RtMidi "
            "in handle: %s", self->in_handle->msg);
          return NULL;
        }
    }
  else
    {
      /* TODO */
    }
  self->midi_ring =
    zix_ring_new (
      sizeof (uint8_t) * (size_t) MIDI_BUFFER_SIZE);

  self->events = midi_events_new (port);

  zix_sem_init (&self->midi_ring_sem, 1);

  return self;
}

/**
 * Opens a device allocated with
 * rtmidi_device_new().
 *
 * @param start Also start the device.
 *
 * @return Non-zero if error.
 */
int
rtmidi_device_open (
  RtMidiDevice * self,
  int            start)
{
  g_message ("opening rtmidi device");
  char designation[800];
  port_get_full_designation (
    self->port, designation);
  char lbl[1200];
  sprintf (lbl, "%s [%u]", designation, self->id);
  rtmidi_open_port (self->in_handle, self->id, lbl);
  if (!self->in_handle->ok)
    {
      g_warning (
        "An error occurred opening the RtMidi "
        "device: %s", self->in_handle->msg);
      return -1;
    }

  if (start)
    {
      rtmidi_device_start (self);
    }

  return 0;
}

/**
 * Close the RtMidiDevice.
 *
 * @param free Also free the memory.
 */
int
rtmidi_device_close (
  RtMidiDevice * self,
  int            free_device)
{
  g_message ("closing rtmidi device");
  rtmidi_device_stop (self);

  if (self->is_input)
    {
      rtmidi_close_port (self->in_handle);
    }
  else
    {
      rtmidi_close_port (self->out_handle);
    }

  if (free_device)
    {
      rtmidi_device_free (self);
    }

  return 0;
}

int
rtmidi_device_start (
  RtMidiDevice * self)
{
  g_message ("starting rtmidi device");
  if (self->is_input)
    {
      rtmidi_in_set_callback (
        self->in_handle,
        (RtMidiCCallback) midi_in_cb, self);
      if (!self->in_handle->ok)
        {
          g_warning (
            "An error occurred setting the RtMidi "
            "device callback: %s",
            self->in_handle->msg);
          return -1;
        }
    }
  self->started = 1;

  return 0;
}

int
rtmidi_device_stop (
  RtMidiDevice * self)
{
  g_message ("stopping rtmidi device");
  if (self->is_input)
    {
      rtmidi_in_cancel_callback (self->in_handle);
    }
  self->started = 0;

  return 0;
}

void
rtmidi_device_free (
  RtMidiDevice *  self)
{
  if (self->is_input)
    {
      rtmidi_in_free (self->in_handle);
    }

  if (self->midi_ring)
    zix_ring_free (self->midi_ring);

  if (self->events)
    midi_events_free (self->events);

  zix_sem_destroy (&self->midi_ring_sem);

  free (self);
}

#endif // HAVE_RTMIDI
