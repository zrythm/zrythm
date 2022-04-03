/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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

#ifdef HAVE_PORT_AUDIO

#  include "audio/channel.h"
#  include "audio/engine.h"
#  include "audio/engine_pa.h"
#  include "audio/master_track.h"
#  include "audio/port.h"
#  include "audio/router.h"
#  include "project.h"
#  include "utils/ui.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

/**
 * This routine will be called by the PortAudio
 * engine when audio is needed.
 *
 * It may called at interrupt level on some
 * machines so don't do anything that could mess up
 * the system like calling malloc() or free().
*/
static int
pa_stream_cb (
  const void *                     in,
  void *                           out,
  unsigned long                    nframes,
  const PaStreamCallbackTimeInfo * time_info,
  PaStreamCallbackFlags            status_flags,
  AudioEngine *                    self)
{
  engine_process (self, (nframes_t) nframes);

  float * outf = (float *) out;
  for (unsigned int i = 0; i < nframes * 2; i++)
    {
      outf[i] = AUDIO_ENGINE->pa_out_buf[i];
    }
  return paContinue;
}

/**
 * Opens a Port Audio stream.
 */
static PaStream *
open_stream (AudioEngine * self)
{
  PaStream * stream;
  PaError    err;

  int api_count = Pa_GetHostApiCount ();
  for (int i = 0; i < api_count; i++)
    {
      const PaHostApiInfo * info =
        Pa_GetHostApiInfo (i);
      g_message (
        "host api %s (%d) found", info->name, i);
    }
  int api = Pa_GetDefaultHostApi ();
  const PaHostApiInfo * api_info =
    Pa_GetHostApiInfo (api);
  /*int api = 2;*/
  g_message (
    "using api %s (%d)", api_info->name, api);

  int device_count = Pa_GetDeviceCount ();
  for (int i = 0; i < device_count; i++)
    {
      const PaDeviceInfo * info =
        Pa_GetDeviceInfo (i);
      g_message (
        "device %s (%d) found, "
        "max channels (in %d, out %d)",
        info->name, i, info->maxInputChannels,
        info->maxOutputChannels);
    }

  PaStreamParameters in_param;
  in_param.device = api_info->defaultInputDevice;
  in_param.channelCount = 2;
  in_param.sampleFormat = paFloat32;
  in_param.suggestedLatency = 10.0;
  in_param.hostApiSpecificStreamInfo = NULL;

  PaStreamParameters out_param;
  out_param.device = api_info->defaultOutputDevice;
  out_param.channelCount = 2;
  out_param.sampleFormat = paFloat32;
  out_param.suggestedLatency = 10.0;
  out_param.hostApiSpecificStreamInfo = NULL;

  g_message (
    "Attempting to open PA stream with input device "
    "%d and output device %d",
    in_param.device, out_param.device);

  /* Open an audio I/O stream. */
  err = Pa_OpenStream (
    &stream,
    /* stereo input */
    &in_param,
    /* stereo output */
    &out_param, self->sample_rate,
    /* block size */
    paFramesPerBufferUnspecified, 0,
    /* callback function */
    (PaStreamCallback *) pa_stream_cb,
    /* user data */
    self);
  if (err != paNoError)
    {
      g_warning (
        "error opening Port Audio stream: %s",
        Pa_GetErrorText (err));
      return NULL;
    }

  return stream;
}

/**
 * Set up Port Audio.
 */
int
engine_pa_setup (AudioEngine * self)
{
  g_message ("Setting up Port Audio...");
  PaError err = Pa_Initialize ();
  if (err != paNoError)
    {
      g_warning (
        "error initializing Port Audio: %s",
        Pa_GetErrorText (err));
      return -1;
    }
  else
    g_message ("Initialized Port Audio");

  /* Set audio engine properties */
  self->sample_rate = 44100;
  self->block_length = 256;

  g_warn_if_fail (
    TRANSPORT && TRANSPORT->beats_per_bar > 1);

  engine_update_frames_per_tick (
    self, TRANSPORT->beats_per_bar, TRANSPORT->bpm,
    self->sample_rate, false);

  self->pa_out_buf =
    calloc (self->block_length * 2, sizeof (float));

  self->pa_stream = open_stream (self);
  g_return_val_if_fail (self->pa_stream, -1);

  g_message ("Starting Port Audio stream...");
  err = Pa_StartStream (self->pa_stream);
  if (err != paNoError)
    {
      g_warning (
        "error starting Port Audio stream: %s",
        Pa_GetErrorText (err));
      return -1;
    }
  else
    g_message ("Started Port Audio stream");

  g_message ("Port Audio set up");

  return 0;
}

void
engine_pa_fill_out_bufs (
  AudioEngine *   self,
  const nframes_t nframes)
{
  for (unsigned int i = 0; i < nframes; i++)
    {
      self->pa_out_buf[i * 2] =
        self->monitor_out->l->buf[i];
      /*g_message ("%f",*/
      /*engine->pa_out_buf[i]);*/
      self->pa_out_buf[i * 2 + 1] =
        self->monitor_out->r->buf[i];
    }
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
engine_pa_test (GtkWindow * win)
{
  char * msg;

  PaError err = Pa_Initialize ();
  if (err != paNoError)
    {
      msg = g_strdup_printf (
        _ ("PortAudio Error: %s"),
        Pa_GetErrorText (err));
      ui_show_error_message (win, msg);
      g_free (msg);
      return 1;
    }

  PaStream * stream;
  err = Pa_OpenDefaultStream (
    &stream, 2, 2, paFloat32, 48000, 512, NULL,
    NULL);
  if (err != paNoError)
    {
      msg = g_strdup_printf (
        _ ("PortAudio Error: %s"),
        Pa_GetErrorText (err));
      ui_show_error_message (win, msg);
      g_free (msg);
      return 1;
    }
  else if ((err = Pa_Terminate ()) != paNoError)
    {
      msg = g_strdup_printf (
        _ ("PortAudio Error: %s"),
        Pa_GetErrorText (err));
      ui_show_error_message (win, msg);
      g_free (msg);
      return 1;
    }
  return 0;
}

/**
 * Closes Port Audio.
 */
void
engine_pa_tear_down (AudioEngine * engine)
{
  PaError err = Pa_StopStream (engine->pa_stream);
  if (err != paNoError)
    g_warning (
      "error stopping Port Audio stream: %s",
      Pa_GetErrorText (err));

  err = Pa_CloseStream (engine->pa_stream);
  if (err != paNoError)
    g_warning (
      "error closing Port Audio stream: %s",
      Pa_GetErrorText (err));

  err = Pa_Terminate ();
  if (err != paNoError)
    g_warning (
      "error terminating Port Audio: %s",
      Pa_GetErrorText (err));
}

#endif
