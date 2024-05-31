// SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#ifdef HAVE_SDL

#  include "dsp/channel.h"
#  include "dsp/engine.h"
#  include "dsp/engine_sdl.h"
#  include "dsp/master_track.h"
#  include "dsp/port.h"
#  include "dsp/router.h"
#  include "dsp/tempo_track.h"
#  include "project.h"
#  include "settings/settings.h"
#  include "utils/ui.h"
#  include "zrythm_app.h"

#  include <glib/gi18n.h>
#  include <gtk/gtk.h>

#  include <SDL2/SDL.h>
#  include <SDL2/SDL_audio.h>

static void
sdl_callback (void * user_data, Uint8 * buf, int len)
{
  AudioEngine * self = (AudioEngine *) user_data;
  if (!self->run)
    return;

  nframes_t num_frames = AUDIO_ENGINE->block_length;
  /*g_message (*/
  /*"processing for num frames %u (len %d)",*/
  /*num_frames, len);*/
  engine_process (self, num_frames);

  memset (buf, 0, (size_t) len);
  float * float_buf = (float *) buf;
  for (nframes_t i = 0; i < num_frames; i++)
    {
      float_buf[i * 2] = self->monitor_out->get_l ().buf_[i];
      float_buf[i * 2 + 1] = self->monitor_out->get_r ().buf_[i];
    }
}

/**
 * Returns a list of names inside \ref names that
 * must be free'd.
 *
 * @param input 1 for input, 0 for output.
 */
void
engine_sdl_get_device_names (
  AudioEngine * self,
  int           input,
  char **       names,
  int *         num_names)
{
  *num_names = SDL_GetNumAudioDevices (input);
  for (int i = 0; i < *num_names; i++)
    {
      names[i] = g_strdup (SDL_GetAudioDeviceName (i, input));
      g_message ("Output audio device %d: %s", i, names[i]);
    }
}

/**
 * Set up Port Audio.
 */
int
engine_sdl_setup (AudioEngine * self)
{
  g_message ("Setting up SDL...");

  if (SDL_Init (SDL_INIT_AUDIO | SDL_INIT_NOPARACHUTE) < 0)
    {
      g_critical ("Failed to initialize SDL: %s", SDL_GetError ());
      return -1;
    }

  SDL_AudioSpec req_specs;
  memset (&req_specs, 0, sizeof (req_specs));
  req_specs.freq =
    engine_samplerate_enum_to_int ((AudioEngineSamplerate) g_settings_get_enum (
      S_P_GENERAL_ENGINE, "sample-rate"));
  req_specs.format = AUDIO_F32SYS;
  req_specs.channels = 2;
  req_specs.samples = engine_buffer_size_enum_to_int (
    (AudioEngineBufferSize) g_settings_get_enum (
      S_P_GENERAL_ENGINE, "buffer-size"));
  req_specs.callback = (SDL_AudioCallback) sdl_callback;
  req_specs.userdata = self;

  int num_out_devices = SDL_GetNumAudioDevices (0);
  for (int i = 0; i < num_out_devices; i++)
    {
      g_message ("Output audio device %d: %s", i, SDL_GetAudioDeviceName (i, 0));
    }

  char * out_device =
    g_settings_get_string (S_P_GENERAL_ENGINE, "sdl-audio-device-name");
  if (!out_device || strlen (out_device) < 1)
    {
      out_device = NULL;
    }
  g_message (
    "Attempting to open device [%s] with sample "
    "rate %d and buffer size %d",
    out_device, req_specs.freq, req_specs.samples);

  SDL_AudioSpec actual_specs;
  self->dev = SDL_OpenAudioDevice (
    out_device, 0, &req_specs, &actual_specs, SDL_AUDIO_ALLOW_FREQUENCY_CHANGE);
  if (self->dev == 0)
    {
      g_critical ("Couldn't open audio device: %s", SDL_GetError ());
      return -1;
    }
  g_message ("[SDL] Opened output device (ID %d)", (int) self->dev);

  /* Set audio engine properties */
  self->sample_rate = (sample_rate_t) actual_specs.freq;
  self->block_length = actual_specs.samples;

  g_message (
    "Setting sample rate to %u and buffer size to "
    "%d",
    self->sample_rate, self->block_length);

  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  g_warn_if_fail (beats_per_bar > 1);

  g_message ("SDL set up");

  return 0;
}

void
engine_sdl_activate (AudioEngine * self, bool activate)
{
  if (activate)
    {
      g_message ("%s: activating...", __func__);
    }
  else
    {
      g_message ("%s: deactivating...", __func__);
    }

  /* start playing */
  SDL_PauseAudioDevice (self->dev, !activate);

  switch (SDL_GetAudioDeviceStatus (self->dev))
    {
    case SDL_AUDIO_STOPPED:
      g_message ("SDL audio stopped");
      break;
    case SDL_AUDIO_PLAYING:
      g_message ("SDL audio playing");
      break;
    case SDL_AUDIO_PAUSED:
      g_message ("SDL audio paused");
      break;
    default:
      g_critical ("[SDL] Unknown audio device status");
      break;
    }

  g_message ("%s: done", __func__);
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
engine_sdl_test (GtkWindow * win)
{
  return 0;
}

/**
 * Closes Port Audio.
 */
void
engine_sdl_tear_down (AudioEngine * self)
{
  SDL_CloseAudioDevice (self->dev);
  SDL_Quit ();
}

#endif
