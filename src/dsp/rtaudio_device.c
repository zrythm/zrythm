// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifdef HAVE_RTAUDIO

#  include "dsp/engine.h"
#  include "dsp/engine_rtaudio.h"
#  include "dsp/port.h"
#  include "dsp/rtaudio_device.h"
#  include "project.h"
#  include "utils/objects.h"
#  include "utils/string.h"

#  include <gtk/gtk.h>

static void
error_cb (rtaudio_error_t err, const char * msg)
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
  /*g_debug ("calling callback");*/

  /* if input device, receive data */
  if (in_buf)
    {
      zix_sem_wait (&self->audio_ring_sem);

      if (status != 0)
        {
          /* xrun */
          g_message ("XRUN in RtAudio");
        }

      zix_ring_write (self->audio_ring, in_buf, nframes * sizeof (float));

#  if 0
      for (unsigned int i = 0; i < nframes; i++)
        {
          if (in_buf[i] > 0.08f)
            {
              g_message (
                "have input %f", (double) in_buf[i]);
            }
        }
#  endif

      zix_sem_post (&self->audio_ring_sem);
    }

  return 0;
}

RtAudioDevice *
rtaudio_device_new (
  int          is_input,
  const char * device_name,
  unsigned int device_id,
  unsigned int channel_idx,
  Port *       port)
{
  RtAudioDevice * self = object_new (RtAudioDevice);

  self->is_input = is_input;
  self->id = device_id;
  self->port = port;
  self->channel_idx = channel_idx;
  self->handle =
    engine_rtaudio_create_rtaudio (AUDIO_ENGINE, AUDIO_ENGINE->audio_backend);
  if (!self->handle)
    {
      g_warning ("Failed to create RtAudio handle");
      free (self);
      return NULL;
    }

  rtaudio_device_info_t dev_nfo;
  if (device_name)
    {
      int dev_count = rtaudio_device_count (self->handle);
      g_return_val_if_fail (dev_count >= 0, NULL);
      for (unsigned int i = 0; i < (unsigned int) dev_count; i++)
        {
          rtaudio_device_info_t cur_dev_nfo =
            rtaudio_get_device_info (self->handle, i);
          if (string_is_equal (cur_dev_nfo.name, device_name))
            {
              dev_nfo = cur_dev_nfo;
              break;
            }
        }
    }
  else
    {
      dev_nfo = rtaudio_get_device_info (self->handle, device_id);
    }
  self->name = g_strdup (dev_nfo.name);

  self->audio_ring = zix_ring_new (
    zix_default_allocator (),
    sizeof (float) * (size_t) RTAUDIO_DEVICE_BUFFER_SIZE);

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
rtaudio_device_open (RtAudioDevice * self, int start)
{
  g_message ("opening rtaudio device");

  rtaudio_device_info_t dev_nfo =
    rtaudio_get_device_info (self->handle, self->id);
  g_message ("RtAudio device %d: %s", self->id, dev_nfo.name);

  /* prepare params */
  struct rtaudio_stream_parameters stream_params = {
    .device_id = self->id,
    .num_channels = 1,
    .first_channel = self->channel_idx,
  };
  struct rtaudio_stream_options stream_opts = {
    .flags = RTAUDIO_FLAGS_SCHEDULE_REALTIME,
    .num_buffers = 2,
    .priority = 99,
    .name = "Zrythm",
  };

  unsigned int samplerate = AUDIO_ENGINE->sample_rate;
  unsigned int buffer_size = AUDIO_ENGINE->block_length;
  /* input stream */
  int ret = rtaudio_open_stream (
    self->handle, NULL, &stream_params, RTAUDIO_FORMAT_FLOAT32, samplerate,
    &buffer_size, (rtaudio_cb_t) myaudio_cb, self, &stream_opts,
    (rtaudio_error_cb_t) error_cb);
  if (ret)
    {
      g_warning (
        "An error occurred opening the RtAudio "
        "stream: %s",
        rtaudio_error (self->handle));
      return -1;
    }
  g_message (
    "Opened %s with samplerate %u and "
    "buffer size %u",
    dev_nfo.name, samplerate, buffer_size);
  self->opened = 1;

  if (start)
    {
      return rtaudio_device_start (self);
    }

  return 0;
}

int
rtaudio_device_start (RtAudioDevice * self)
{
  int ret = rtaudio_start_stream (self->handle);
  if (ret)
    {
      g_critical (
        "An error occurred starting the RtAudio "
        "stream: %s",
        rtaudio_error (self->handle));
      return ret;
    }
  rtaudio_device_info_t dev_nfo =
    rtaudio_get_device_info (self->handle, self->id);
  g_message ("RtAudio device %s started", dev_nfo.name);
  self->started = 1;

  return 0;
}

int
rtaudio_device_stop (RtAudioDevice * self)
{
  int ret = rtaudio_stop_stream (self->handle);
  if (ret)
    {
      g_critical (
        "An error occurred stopping the RtAudio "
        "stream: %s",
        rtaudio_error (self->handle));
      return ret;
    }
  rtaudio_device_info_t dev_nfo =
    rtaudio_get_device_info (self->handle, self->id);
  g_message ("RtAudio device %s stopped", dev_nfo.name);
  self->started = 0;

  return 0;
}

/**
 * Close the RtAudioDevice.
 *
 * @param free Also free the memory.
 */
int
rtaudio_device_close (RtAudioDevice * self, int free_device)
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
rtaudio_device_free (RtAudioDevice * self)
{
  if (self->audio_ring)
    zix_ring_free (self->audio_ring);

  zix_sem_destroy (&self->audio_ring_sem);

  if (self->name)
    g_free (self->name);

  free (self);
}

#endif // HAVE_RTAUDIO
