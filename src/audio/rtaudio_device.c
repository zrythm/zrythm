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

#ifdef HAVE_RTAUDIO

#include "audio/engine.h"
#include "audio/engine_rtaudio.h"
#include "audio/port.h"
#include "audio/rtaudio_device.h"
#include "project.h"

#include <gtk/gtk.h>

static void
error_cb (
  rtaudio_error_t err,
  const char *    msg)
{
  g_critical ("RtAudio error: %s", msg);
}

static int
myaudio_cb (
  float *                 out_buf,
  float *                 in_buf,
  unsigned int            nframes,
  double                  stream_time,
  rtaudio_stream_status_t status,
  RtAudioDevice *         self)
{
  g_message ("calling callback");
  zix_sem_wait (&self->audio_ring_sem);


#if 0
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

  if (status != 0)
    {
      /* xrun */
      g_warning ("XRUN in RtAudio");
    }

  if (!self->run)
    return 0;

  nframes_t num_frames = (nframes_t) nframes;

  memset (
    out_buf, 0,
    (size_t) (nframes * 2) * sizeof (float));
  for (nframes_t i = 0; i < num_frames; i++)
    {
#ifdef TRIAL_VER
      if (self->limit_reached)
        {
          out_buf[i * 2] = 0;
          out_buf[i * 2 + 1] = 0;
          continue;
        }
#endif
      out_buf[i * 2] =
        self->monitor_out->l->buf[i];
      out_buf[i * 2 + 1] =
        self->monitor_out->r->buf[i];
    }

  /* add to ring buffer */
  MidiEventHeader h = {
    .time = (uint64_t) ts, .size = message_size,
  };
  zix_ring_write (
    self->midi_ring,
    (uint8_t *) &h, sizeof (MidiEventHeader));
  zix_ring_write (
    self->midi_ring, message, message_size);

#endif
  zix_sem_post (&self->audio_ring_sem);

  return 0;
}

RtAudioDevice *
rtaudio_device_new (
  int          is_input,
  unsigned int device_id,
  unsigned int channel_idx,
  Port *       port)
{
  RtAudioDevice * self =
    calloc (1, sizeof (RtAudioDevice));

  self->is_input = is_input;
  self->id = device_id;
  self->port = port;
  self->channel_idx = channel_idx;
  self->handle =
    engine_rtaudio_create_rtaudio (AUDIO_ENGINE);
  g_return_val_if_fail (self->handle, NULL);

  rtaudio_device_info_t dev_nfo =
    rtaudio_get_device_info (
      self->handle, (int) device_id);
  self->name = g_strdup (dev_nfo.name);

  self->audio_ring =
    zix_ring_new (
      sizeof (float) *
      (size_t) RTAUDIO_DEVICE_BUFFER_SIZE);

  zix_sem_init (&self->audio_ring_sem, 1);

  return self;
}

/**
 * Opens a device allocated with
 * rtaudio_device_new().
 *
 * @param start Also start the device.
 *
 * @return Non-zero if error.
 */
int
rtaudio_device_open (
  RtAudioDevice * self,
  int             start)
{
  g_message ("opening rtaudio device");

  rtaudio_device_info_t dev_nfo =
    rtaudio_get_device_info (
      self->handle, (int) self->id);
  g_message (
    "RtAudio device %d: %s",
    self->id, dev_nfo.name);

  /* prepare params */
  struct rtaudio_stream_parameters stream_params = {
    .device_id = (unsigned int) self->id,
    .num_channels = 1,
    .first_channel = self->channel_idx,
  };
  struct rtaudio_stream_options stream_opts = {
    .flags = RTAUDIO_FLAGS_SCHEDULE_REALTIME,
    .num_buffers = 2, .priority = 99,
    .name = "Zrythm",
  };

  unsigned int samplerate =
    AUDIO_ENGINE->sample_rate;
  unsigned int buffer_size =
    AUDIO_ENGINE->block_length;
  int ret =
    rtaudio_open_stream (
      self->handle, &stream_params, NULL,
      RTAUDIO_FORMAT_FLOAT32, samplerate,
      &buffer_size, (rtaudio_cb_t) myaudio_cb, self,
      &stream_opts, (rtaudio_error_cb_t) error_cb);
  if (ret)
    {
      g_critical (
        "An error occurred opening the RtAudio "
        "stream: %s", rtaudio_error (self->handle));
      return -1;
    }
  g_message ("Opened %s with samplerate %u and "
             "buffer size %u",
             dev_nfo.name, samplerate,
             buffer_size);
  self->opened = 1;

  if (start)
    {
      return rtaudio_device_start (self);
    }

  return 0;
}

int
rtaudio_device_start (
  RtAudioDevice * self)
{
  int ret =
    rtaudio_start_stream (self->handle);
  if (ret)
    {
      g_critical (
        "An error occurred starting the RtAudio "
        "stream: %s", rtaudio_error (self->handle));
      return ret;
    }
  rtaudio_device_info_t dev_nfo =
    rtaudio_get_device_info (
      self->handle, (int) self->id);
  g_message ("RtAudio device %s started",
             dev_nfo.name);
  self->started = 1;

  return 0;
}

int
rtaudio_device_stop (
  RtAudioDevice * self)
{
  int ret = rtaudio_stop_stream (self->handle);
  if (ret)
    {
      g_critical (
        "An error occurred stopping the RtAudio "
        "stream: %s", rtaudio_error (self->handle));
      return ret;
    }
  rtaudio_device_info_t dev_nfo =
    rtaudio_get_device_info (
      self->handle, (int) self->id);
  g_message ("RtAudio device %s stopped",
             dev_nfo.name);
  self->started = 0;

  return 0;
}

/**
 * Close the RtAudioDevice.
 *
 * @param free Also free the memory.
 */
int
rtaudio_device_close (
  RtAudioDevice * self,
  int            free_device)
{
  g_message ("closing rtaudio device");
  rtaudio_close_stream (self->handle);
  self->opened = 0;

  if (free_device)
    {
      rtaudio_device_free (self);
    }

  return 0;
}

void
rtaudio_device_free (
  RtAudioDevice *  self)
{
  if (self->audio_ring)
    zix_ring_free (self->audio_ring);

  zix_sem_destroy (&self->audio_ring_sem);

  if (self->name)
    g_free (self->name);

  free (self);
}

#endif // HAVE_RTAUDIO
