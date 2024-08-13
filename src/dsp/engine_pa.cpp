// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
 */

#include "zrythm-config.h"

#ifdef HAVE_PORT_AUDIO

#  include "dsp/channel.h"
#  include "dsp/engine.h"
#  include "dsp/engine_pa.h"
#  include "dsp/master_track.h"
#  include "dsp/port.h"
#  include "dsp/router.h"
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
port_audio_stream_cb (
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
      outf[i] = AUDIO_ENGINE->port_audio_out_buf[i];
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
      const PaHostApiInfo * info = Pa_GetHostApiInfo (i);
      z_info ("host api {} ({}) found", info->name, i);
    }
  int                   api = Pa_GetDefaultHostApi ();
  const PaHostApiInfo * api_info = Pa_GetHostApiInfo (api);
  /*int api = 2;*/
  z_info ("using api {} ({})", api_info->name, api);

  int device_count = Pa_GetDeviceCount ();
  for (int i = 0; i < device_count; i++)
    {
      const PaDeviceInfo * info = Pa_GetDeviceInfo (i);
      z_info (
        "device %s (%d) found, "
        "max channels (in %d, out %d)",
        info->name, i, info->maxInputChannels, info->maxOutputChannels);
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

  z_info (
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
    (PaStreamCallback *) port_audio_stream_cb,
    /* user data */
    self);
  if (err != paNoError)
    {
      z_warning ("error opening Port Audio stream: {}", Pa_GetErrorText (err));
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
  z_info ("Setting up Port Audio...");
  PaError err = Pa_Initialize ();
  if (err != paNoError)
    {
      z_warning ("error initializing Port Audio: {}", Pa_GetErrorText (err));
      return -1;
    }
  else
    z_info ("Initialized Port Audio");

  /* Set audio engine properties */
  self->sample_rate = 44100;
  self->block_length = 256;

  z_warn_if_fail (TRANSPORT && TRANSPORT->beats_per_bar > 1);

  engine_update_frames_per_tick (
    self, TRANSPORT->beats_per_bar, TRANSPORT->bpm, self->sample_rate, false);

  self->port_audio_out_buf =
    (float *) calloc (self->block_length * 2, sizeof (float));

  self->port_audio_stream = open_stream (self);
  z_return_val_if_fail (self->port_audio_stream, -1);

  z_info ("Starting Port Audio stream...");
  err = Pa_StartStream (self->port_audio_stream);
  if (err != paNoError)
    {
      z_warning ("error starting Port Audio stream: {}", Pa_GetErrorText (err));
      return -1;
    }
  else
    z_info ("Started Port Audio stream");

  z_info ("Port Audio set up");

  return 0;
}

void
engine_pa_fill_out_bufs (AudioEngine * self, const nframes_t nframes)
{
  for (unsigned int i = 0; i < nframes; i++)
    {
      self->port_audio_out_buf[i * 2] = self->monitor_out->get_l ().buf_[i];
      /*z_info ("{:f}",*/
      /*engine->port_audio_out_buf[i]);*/
      self->port_audio_out_buf[i * 2 + 1] = self->monitor_out->get_r ().buf_[i];
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
      msg = g_strdup_printf (_ ("PortAudio Error: %s"), Pa_GetErrorText (err));
      ui_show_error_message (win, msg);
      g_free (msg);
      return 1;
    }

  PaStream * stream;
  err = Pa_OpenDefaultStream (
    &stream, 2, 2, paFloat32, 48000, 512, nullptr, nullptr);
  if (err != paNoError)
    {
      msg = g_strdup_printf (_ ("PortAudio Error: %s"), Pa_GetErrorText (err));
      ui_show_error_message (win, msg);
      g_free (msg);
      return 1;
    }
  else if ((err = Pa_Terminate ()) != paNoError)
    {
      msg = g_strdup_printf (_ ("PortAudio Error: %s"), Pa_GetErrorText (err));
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
  PaError err = Pa_StopStream (engine->port_audio_stream);
  if (err != paNoError)
    z_warning ("error stopping Port Audio stream: {}", Pa_GetErrorText (err));

  err = Pa_CloseStream (engine->port_audio_stream);
  if (err != paNoError)
    z_warning ("error closing Port Audio stream: {}", Pa_GetErrorText (err));

  err = Pa_Terminate ();
  if (err != paNoError)
    z_warning ("error terminating Port Audio: {}", Pa_GetErrorText (err));
}

#endif
