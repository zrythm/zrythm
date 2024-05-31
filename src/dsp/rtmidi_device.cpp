// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifdef HAVE_RTMIDI

#  include "dsp/engine.h"
#  include "dsp/midi_event.h"
#  include "dsp/port.h"
#  include "dsp/rtmidi_device.h"
#  include "project.h"
#  include "utils/objects.h"
#  include "utils/string.h"

#  include <gtk/gtk.h>

static enum RtMidiApi
get_api_from_midi_backend (MidiBackend backend)
{
  enum RtMidiApi apis[20];
  int            num_apis = (int) rtmidi_get_compiled_api (apis, 20);
  if (num_apis < 0)
    {
      g_critical ("RtMidi: an error occurred fetching compiled APIs");
      return (enum RtMidiApi) - 1;
    }
  for (int i = 0; i < num_apis; i++)
    {
      if (
        backend == MidiBackend::MIDI_BACKEND_ALSA_RTMIDI
        && apis[i] == RTMIDI_API_LINUX_ALSA)
        {
          return apis[i];
        }
      if (
        backend == MidiBackend::MIDI_BACKEND_JACK_RTMIDI
        && apis[i] == RTMIDI_API_UNIX_JACK)
        {
          return apis[i];
        }
      if (
        backend == MidiBackend::MIDI_BACKEND_WINDOWS_MME_RTMIDI
        && apis[i] == RTMIDI_API_WINDOWS_MM)
        {
          return apis[i];
        }
      if (
        backend == MidiBackend::MIDI_BACKEND_COREMIDI_RTMIDI
        && apis[i] == RTMIDI_API_MACOSX_CORE)
        {
          return apis[i];
        }
#  ifdef HAVE_RTMIDI_6
      if (
        backend == MidiBackend::MIDI_BACKEND_WINDOWS_UWP_RTMIDI
        && apis[i] == RTMIDI_API_WINDOWS_UWP)
        {
          return apis[i];
        }
#  endif
    }

  return RTMIDI_API_RTMIDI_DUMMY;
}

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
  gint64 ts = cur_time - self->port->last_midi_dequeue_;
  g_return_if_fail (ts >= 0);
  char portname[900];
  self->port->get_full_designation (portname);
  if (DEBUGGING)
    {
      g_debug (
        "[%s] message received of size %zu at %ld", portname, message_size, ts);
    }

  /* add to ring buffer */
  MidiEventHeader h = {
    .time = (uint64_t) ts,
    .size = message_size,
  };
  zix_ring_write (self->midi_ring, (uint8_t *) &h, sizeof (MidiEventHeader));
  zix_ring_write (self->midi_ring, message, message_size);

  zix_sem_post (&self->midi_ring_sem);
}

static bool rtmidi_device_first_run = false;

static int
rtmidi_device_get_id_from_name (bool is_input, const char * name)
{
  if (is_input)
    {
      RtMidiDevice * dev = rtmidi_device_new (1, NULL, 0, NULL);
      if (!dev)
        return 0;

      unsigned int num_ports = rtmidi_get_port_count (dev->in_handle);
      for (unsigned int i = 0; i < num_ports; i++)
        {
          int buf_len;
          rtmidi_get_port_name (dev->in_handle, i, NULL, &buf_len);
          char dev_name[buf_len];
          rtmidi_get_port_name (dev->in_handle, i, dev_name, &buf_len);
          if (string_is_equal (dev_name, name))
            {
              rtmidi_device_free (dev);
              return (int) i;
            }
        }
      rtmidi_device_free (dev);
    }

  g_warning ("could not find RtMidi device with name %s", name);

  return -1;
}

/**
 * @param name If non-NUL, search by name instead of
 *   by @ref device_id.
 */
RtMidiDevice *
rtmidi_device_new (
  bool         is_input,
  const char * name,
  unsigned int device_id,
  Port *       port)
{
  RtMidiDevice * self = object_new (RtMidiDevice);

  enum RtMidiApi apis[20];
  int            num_apis = (int) rtmidi_get_compiled_api (apis, 20);
  if (num_apis < 0)
    {
      g_warning (
        "RtMidi: an error occurred fetching "
        "compiled APIs");
      object_zero_and_free (self);
      return NULL;
    }
  if (rtmidi_device_first_run)
    {
      for (int i = 0; i < num_apis; i++)
        {
          g_message ("RtMidi API found: %s", rtmidi_api_name (apis[i]));
        }
      rtmidi_device_first_run = false;
    }

  enum RtMidiApi api = get_api_from_midi_backend (AUDIO_ENGINE->midi_backend);
  if (api == RTMIDI_API_RTMIDI_DUMMY)
    {
      g_warning (
        "RtMidi API for %s not enabled", ENUM_NAME (AUDIO_ENGINE->midi_backend));
      object_zero_and_free (self);
      return NULL;
    }

  if (name)
    {
      int id_from_name = rtmidi_device_get_id_from_name (is_input, name);
      if (id_from_name < 0)
        {
          g_warning ("Could not find RtMidi Device '%s'", name);
          object_zero_and_free (self);
          return NULL;
        }
      self->id = (unsigned int) id_from_name;
    }
  else
    {
      self->id = device_id;
    }
  self->is_input = is_input;
  self->port = port;
  if (is_input)
    {
      self->in_handle =
        rtmidi_in_create (api, "Zrythm", AUDIO_ENGINE->midi_buf_size);
      if (!self->in_handle->ok)
        {
          g_warning (
            "An error occurred creating an RtMidi "
            "in handle: %s",
            self->in_handle->msg);
          object_zero_and_free (self);
          return NULL;
        }
    }
  else
    {
      /* TODO */
    }
  self->midi_ring = zix_ring_new (
    zix_default_allocator (), sizeof (uint8_t) * (size_t) MIDI_BUFFER_SIZE);

  self->events = midi_events_new ();

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
rtmidi_device_open (RtMidiDevice * self, int start)
{
  g_message ("opening rtmidi device");
  char designation[800];
  self->port->get_full_designation (designation);
  char lbl[1200];
  sprintf (lbl, "%s [%u]", designation, self->id);
  rtmidi_close_port (self->in_handle);
  rtmidi_open_port (self->in_handle, self->id, lbl);
  if (!self->in_handle->ok)
    {
      g_warning (
        "An error occurred opening the RtMidi "
        "device: %s",
        self->in_handle->msg);
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
rtmidi_device_close (RtMidiDevice * self, int free_device)
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
rtmidi_device_start (RtMidiDevice * self)
{
  g_message ("starting rtmidi device");
  if (self->is_input)
    {
      rtmidi_in_set_callback (
        self->in_handle, (RtMidiCCallback) midi_in_cb, self);
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
rtmidi_device_stop (RtMidiDevice * self)
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
rtmidi_device_free (RtMidiDevice * self)
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
