/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifdef HAVE_PORT_AUDIO

#include "audio/channel.h"
#include "audio/engine.h"
#include "audio/engine_pa.h"
#include "audio/mixer.h"
#include "audio/port.h"
#include "project.h"
#include "utils/ui.h"

#include <gtk/gtk.h>

#include <glib/gi18n.h>

/**
 * Set up Port Audio.
 */
void
pa_setup (
  AudioEngine * self,
  int           loading)
{
  g_message ("Setting up Port Audio...");
  PaError err = Pa_Initialize();
  if (err != paNoError)
    g_warning ("error initializing Port Audio: %s",
               Pa_GetErrorText (err));
  else
    g_message ("Initialized Port Audio");

  /* Set audio engine properties */
  self->sample_rate   = 44100;
  self->block_length  = 256;

  g_warn_if_fail (TRANSPORT &&
                  TRANSPORT->beats_per_bar > 1);

  engine_update_frames_per_tick (
    TRANSPORT->beats_per_bar,
    TRANSPORT->bpm,
    self->sample_rate);

  /* create ports */
  Port * stereo_out_l, * stereo_out_r,
       * stereo_in_l, * stereo_in_r;

  if (loading)
    {
    }
  else
    {
      stereo_out_l =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "PortAudio Stereo Out / L",
          NULL);
      stereo_out_r =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_OUTPUT,
          "PortAudio Stereo Out / R",
          NULL);
      stereo_in_l =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_INPUT,
          "PortAudio Stereo In / L",
          NULL);
      stereo_in_r =
        port_new_with_data (
          INTERNAL_PA_PORT,
          TYPE_AUDIO,
          FLOW_INPUT,
          "PortAudio Stereo In / R",
          NULL);

      self->stereo_in  =
        stereo_ports_new (stereo_in_l,
                          stereo_in_r);
      self->stereo_out =
        stereo_ports_new (stereo_out_l,
                          stereo_out_r);

      self->pa_out_buf =
        calloc (self->block_length * 2,
                sizeof (float));
    }

  self->pa_stream =
    pa_open_stream (self);

  g_message ("Starting Port Audio stream...");
  err = Pa_StartStream( self->pa_stream );
  if( err != paNoError )
    g_warning (
      "error starting Port Audio stream: %s",
      Pa_GetErrorText (err));
  else
    g_message ("Started Port Audio stream");

  g_message ("Port Audio set up");
}

void
engine_pa_fill_stereo_out_buffs (
  AudioEngine * engine)
{
  int nframes = engine->nframes;
  for (int i = 0; i < nframes; i++)
    {
      engine->pa_out_buf[i * 2] =
        MIXER->master->channel->
          stereo_out->l->buf[i];
      g_message ("%f",
                 engine->pa_out_buf[i]);
      engine->pa_out_buf[i * 2 + 1] =
        MIXER->master->channel->
          stereo_out->r->buf[i];
    }
}

/**
 * This routine will be called by the PortAudio
 * engine when audio is needed.
 *
 * It may called at interrupt level on some
 * machines so don't do anything that could mess up
 * the system like calling malloc() or free().
*/
int
pa_stream_cb (
  const void *                    in,
  void *                          out,
  unsigned long                   nframes,
  const PaStreamCallbackTimeInfo* time_info,
  PaStreamCallbackFlags           status_flags,
  void *                   self)
{
  engine_process (AUDIO_ENGINE, nframes);

  float * outf = (float *) out;
  for (int i = 0; i < nframes * 2; i++)
    {
      outf[i] = AUDIO_ENGINE->pa_out_buf[i];
    }
  return paContinue;
}

/**
 * Opens a Port Audio stream.
 */
PaStream *
pa_open_stream (AudioEngine * self)
{
  PaStream *stream;
  PaError err;

  for (int i = 0; i < Pa_GetHostApiCount (); i++)
    {
      const PaHostApiInfo * info = Pa_GetHostApiInfo (i);
      g_message ("host api %s (%d) found",
                 info->name,
		 i);
    }
  int api = Pa_GetDefaultHostApi ();
  /*int api = 2;*/
  g_message ("using api %s (%d)",
	  Pa_GetHostApiInfo (api)->name, api);

  PaStreamParameters in_param;
  in_param.device =
    Pa_GetHostApiInfo (
      api)->defaultInputDevice;
  in_param.channelCount = 2;
  in_param.sampleFormat = paFloat32;
  in_param.suggestedLatency = 10.0;
  in_param.hostApiSpecificStreamInfo = NULL;

  PaStreamParameters out_param;
  out_param.device =
    Pa_GetHostApiInfo (
      api)->defaultOutputDevice;
  out_param.channelCount = 2;
  out_param.sampleFormat = paFloat32;
  out_param.suggestedLatency = 10.0;
  out_param.hostApiSpecificStreamInfo = NULL;

  /* Open an audio I/O stream. */
  err =
    Pa_OpenStream(
      &stream,
      &in_param,          /* stereo input */
      &out_param,          /* stereo output */
      self->sample_rate,
      256,        /* frames per buffer, i.e. the number
      of sample frames that PortAudio will
      request from the callback. Many apps
      may want to use
      paFramesPerBufferUnspecified, which
      tells PortAudio to pick the best,
      possibly changing, buffer size.*/
      0,
      /* this is your callback function */
      pa_stream_cb,
      /*This is a pointer that will be passed to
      your callback*/
      self);
  if( err != paNoError )
    g_warning ("error opening Port Audio stream: %s",
               Pa_GetErrorText (err));

  return stream;
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
engine_pa_test (
  GtkWindow * win)
{
  char *msg;

  PaError err = Pa_Initialize ();
  if (err != paNoError)
    {
      msg =
        g_strdup_printf (
          _("PortAudio Error: %s"),
          Pa_GetErrorText (err));
      ui_show_error_message (
        win, msg);
      g_free (msg);
      return 1;
    }

  PaStream * stream;
  err =
    Pa_OpenDefaultStream (
      &stream,
      2, 2, paFloat32,
      48000,
      512,
      NULL, NULL);
  if (err != paNoError)
    {
      msg =
        g_strdup_printf (
          _("PortAudio Error: %s"),
          Pa_GetErrorText (err));
      ui_show_error_message (
        win, msg);
      g_free (msg);
      return 1;
    }
  else if ((err = Pa_Terminate ()) != paNoError)
    {
      msg =
        g_strdup_printf (
          _("PortAudio Error: %s"),
          Pa_GetErrorText (err));
      ui_show_error_message (
        win, msg);
      g_free (msg);
      return 1;
    }
  return 0;
}

/**
 * Closes Port Audio.
 */
void
pa_terminate (AudioEngine * engine)
{
  PaError err = Pa_StopStream( engine->pa_stream );
  if( err != paNoError )
    g_warning ("error stopping Port Audio stream: %s",
               Pa_GetErrorText (err));

  err = Pa_CloseStream( engine->pa_stream );
  if( err != paNoError )
    g_warning ("error closing Port Audio stream: %s",
               Pa_GetErrorText (err));

  err = Pa_Terminate();
  if( err != paNoError )
    g_warning ("error terminating Port Audio: %s",
               Pa_GetErrorText (err));
}

#endif
