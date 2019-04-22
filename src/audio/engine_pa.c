/*
 * audio/engine_pa.c - Port Audio engine
 *
 * Copyright (C) 2019 Alexandros Theodotou
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
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

#include <gtk/gtk.h>

#define SAMPLE_RATE (48000)

/**
 * Set up Port Audio.
 */
void
pa_setup (AudioEngine * self)
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
  self->midi_buf_size = 4096;

  /* create ports */
  Port * stereo_out_l =
    port_new_with_data (
      INTERNAL_PA_PORT,
      TYPE_AUDIO,
      FLOW_OUTPUT,
      "PortAudio Stereo Out / L",
      NULL);
  Port * stereo_out_r =
    port_new_with_data (
      INTERNAL_PA_PORT,
      TYPE_AUDIO,
      FLOW_OUTPUT,
      "PortAudio Stereo Out / R",
      NULL);
  Port * stereo_in_l =
    port_new_with_data (
      INTERNAL_PA_PORT,
      TYPE_AUDIO,
      FLOW_INPUT,
      "PortAudio Stereo In / L",
      NULL);
  Port * stereo_in_r =
    port_new_with_data (
      INTERNAL_PA_PORT,
      TYPE_AUDIO,
      FLOW_INPUT,
      "PortAudio Stereo In / R",
      NULL);
  Port * midi_in =
    port_new_with_data (
      INTERNAL_PA_PORT,
      TYPE_EVENT,
      FLOW_INPUT,
      "PortAudio MIDI In",
      NULL);

  /* FIXME is this backend-specific? */
  Port * midi_editor_manual_press =
    port_new_with_type (
      TYPE_EVENT,
      FLOW_INPUT,
      "MIDI Editor Manual Press");


  self->stereo_in  =
    stereo_ports_new (stereo_in_l,
                      stereo_in_r);
  self->stereo_out =
    stereo_ports_new (stereo_out_l,
                      stereo_out_r);
  self->midi_in    = midi_in;
  self->midi_editor_manual_press = midi_editor_manual_press;

  /* init MIDI queues for manual presse/piano roll */
  self->midi_editor_manual_press->midi_events->
    queue =
      calloc (1, sizeof (MidiEvents));

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
  for (int i = 0;
       i < AUDIO_ENGINE->nframes;
       i++)
    {
      *engine->pa_out_buf++ =
        MIXER->master->stereo_out->l->buf[i];
      *engine->pa_out_buf++ =
        MIXER->master->stereo_out->r->buf[i];
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
  void *                          user_data)
{
  return engine_process (AUDIO_ENGINE, nframes);
}

/**
 * Opens a Port Audio stream.
 */
PaStream *
pa_open_stream (AudioEngine * engine)
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
      SAMPLE_RATE,
      256,        /* frames per buffer, i.e. the number
      of sample frames that PortAudio will
      request from the callback. Many apps
      may want to use
      paFramesPerBufferUnspecified, which
      tells PortAudio to pick the best,
      possibly changing, buffer size.*/
      0,
      pa_stream_cb, /* this is your callback function */
      engine ); /*This is a pointer that will be passed to
      your callback*/
  if( err != paNoError )
    g_warning ("error opening Port Audio stream: %s",
               Pa_GetErrorText (err));

  return stream;
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
