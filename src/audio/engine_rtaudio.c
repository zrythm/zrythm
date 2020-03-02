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

#ifdef HAVE_RTAUDIO

#include "audio/engine.h"
#include "audio/engine_rtaudio.h"
#include "settings/settings.h"
#include "project.h"
#include "zrythm.h"

#include <gtk/gtk.h>

#include <rtaudio/rtaudio_c.h>

static int
audio_cb (
  float *                 out_buf,
  float *                 in_buf,
  unsigned int            nframes,
  double                  stream_time,
  rtaudio_stream_status_t status,
  AudioEngine *           self)
{
  if (status != 0)
    {
      /* xrun */
      g_warning ("XRUN in RtAudio");
    }

  return 0;
}

static void
error_cb (
  rtaudio_error_t err,
  const char *    msg)
{
  g_critical ("RtAudio error: %s", msg);
}

static void
get_error (
  int    code,
  char * str)
{
  switch (code)
    {
    case RTAUDIO_ERROR_WARNING:
      strcpy (str, "A non-critical error.");
      break;
    case RTAUDIO_ERROR_DEBUG_WARNING:
      strcpy (str, "-critical error which might be useful for debugging.");
      break;
    case RTAUDIO_ERROR_UNSPECIFIED:
      strcpy (str, "default, unspecified error type.");
      break;
    case RTAUDIO_ERROR_NO_DEVICES_FOUND:
      strcpy (str, "ces found on system.");
      break;
    case RTAUDIO_ERROR_INVALID_DEVICE:
      strcpy (str, "alid device ID was specified.");
      break;
    case RTAUDIO_ERROR_MEMORY_ERROR:
      strcpy (str, "rror occured during memory allocation.");
      break;
    case RTAUDIO_ERROR_INVALID_PARAMETER:
      strcpy (str, "lid parameter was specified to a function.");
      break;
    case RTAUDIO_ERROR_INVALID_USE:
      strcpy (str, "function was called incorrectly.");
      break;
    case RTAUDIO_ERROR_DRIVER_ERROR:
      strcpy (str, "stem driver error occured.");
      break;
    case RTAUDIO_ERROR_SYSTEM_ERROR:
      strcpy (str, "stem error occured.");
      break;
    case RTAUDIO_ERROR_THREAD_ERROR:
      strcpy (str, "read error occured.");
      break;
    }
}

/**
 * Set up the engine.
 */
int
engine_rtaudio_setup (
  AudioEngine * self,
  int           loading)
{
  g_message (
    "Setting up RtAudio %s...", rtaudio_version ());

  self->rtaudio =
    rtaudio_create (
#ifdef _WOE32
      RTAUDIO_API_WINDOWS_WASAPI
#elif defined(__APPLE__)
      RTAUDIO_API_MACOSX_CORE
#else
      RTAUDIO_API_LINUX_ALSA
#endif
      );
  if (rtaudio_error (self->rtaudio))
    {
      g_critical (
        "RtAudio: %s",
        rtaudio_error (self->rtaudio));
      return -1;
    }

  int dev_count =
    rtaudio_device_count (self->rtaudio);
  if (dev_count == 0)
    {
      g_critical ("No devices found");
      return -1;
    }

  /* check if selected device is found in the list
   * of devices and get the id of the output device
   * to open */
  char * out_device =
    g_settings_get_string (
      S_PREFERENCES, "rtaudio-audio-device-name");
  int out_device_id = -1;
  for (int i = 0; i < dev_count; i++)
    {
      rtaudio_device_info_t dev_nfo =
        rtaudio_get_device_info (self->rtaudio, i);
      g_message (
        "RtAudio device %d: %s",
        i, dev_nfo.name);
      if (string_is_equal (
            dev_nfo.name, out_device, 1))
        {
          out_device_id = i;
        }
    }
  if (out_device_id == -1)
    {
      out_device_id =
        (int)
        rtaudio_get_default_output_device (
          self->rtaudio);
      rtaudio_device_info_t dev_nfo =
        rtaudio_get_device_info (
          self->rtaudio, out_device_id);
      out_device = g_strdup (dev_nfo.name);
    }

  /* prepare params */
  struct rtaudio_stream_parameters out_stream_params = {
    .device_id = (unsigned int) out_device_id,
    .num_channels = 2, .first_channel = 0,
  };
  struct rtaudio_stream_options stream_opts = {
    .flags = RTAUDIO_FLAGS_SCHEDULE_REALTIME,
    .num_buffers = 2, .priority = 99,
    .name = "Zrythm",
  };

  unsigned int samplerate =
    (unsigned int)
    engine_samplerate_enum_to_int (
      (AudioEngineSamplerate)
      g_settings_get_enum (
        S_PREFERENCES, "samplerate"));
  unsigned int buffer_size =
    (unsigned int)
    engine_buffer_size_enum_to_int (
      (AudioEngineBufferSize)
      g_settings_get_enum (
        S_PREFERENCES, "buffer-size"));
  g_message (
    "Attempting to open device [%s] with sample "
    "rate %u and buffer size %d",
    out_device, samplerate, buffer_size);

  int ret =
    rtaudio_open_stream (
      self->rtaudio, &out_stream_params, NULL,
      RTAUDIO_FORMAT_FLOAT32, samplerate,
      &buffer_size, (rtaudio_cb_t) audio_cb, self,
      &stream_opts, (rtaudio_error_cb_t) error_cb);
  if (ret)
    {
      char err_str[8000];
      get_error (ret, err_str);
      g_critical (
        "An error occurred opening the RtAudio "
        "stream: %s", err_str);
      return -1;
    }
  self->block_length = buffer_size;
  self->sample_rate = (sample_rate_t) samplerate;

  engine_update_frames_per_tick (
    self, TRANSPORT->beats_per_bar,
    TRANSPORT->bpm, self->sample_rate);

  /* create ports */
  Port * monitor_out_l, * monitor_out_r;
  if (loading)
    {
    }
  else
    {
      monitor_out_l =
        port_new_with_type (
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "RtAudio Stereo Out / L");
      monitor_out_r =
        port_new_with_type (
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "RtAudio Stereo Out / R");

      self->monitor_out =
        stereo_ports_new_from_existing (
          monitor_out_l,
          monitor_out_r);
    }

  g_message ("RtAudio set up");

  return 0;
}

void
engine_rtaudio_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes)
{
}

void
engine_rtaudio_activate (
  AudioEngine * self)
{
  engine_realloc_port_buffers (
    self, AUDIO_ENGINE->block_length);

  g_message ("RtAudio activated");
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
engine_rtaudio_test (
  GtkWindow * win)
{
  return 0;
}

/**
 * Closes the engine.
 */
void
engine_rtaudio_tear_down (
  AudioEngine * engine)
{
}

#endif // HAVE_RTAUDIO
