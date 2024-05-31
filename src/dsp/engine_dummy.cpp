// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 */

#include "dsp/engine.h"
#include "dsp/engine_dummy.h"
#include "dsp/port.h"
#include "dsp/router.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

static gpointer
process_cb (gpointer data)
{
  AudioEngine * self = (AudioEngine *) data;

  double secs_per_block = (double) self->block_length / self->sample_rate;
  gulong sleep_time = (gulong) (secs_per_block * 1000.0 * 1000);

  g_return_val_if_fail (g_thread_self () != zrythm_app->gtk_thread, NULL);

  g_message ("Running dummy audio engine for first time");

  while (1)
    {
      if (self->stop_dummy_audio_thread)
        {
          break;
        }

      engine_process (self, self->block_length);
      g_usleep (sleep_time);
    }

  return NULL;
}

int
engine_dummy_setup (AudioEngine * self)
{
  /* Set audio engine properties */
  self->midi_buf_size = 4096;

  if (ZRYTHM_HAVE_UI && zrythm_app->buf_size > 0)
    {
      self->block_length = (nframes_t) zrythm_app->buf_size;
    }
  else
    {
      self->block_length = 256;
    }

  if (zrythm_app->samplerate > 0)
    {
      self->sample_rate = (nframes_t) zrythm_app->samplerate;
    }
  else
    {
      self->sample_rate = 44100;
    }

  int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
  g_warn_if_fail (beats_per_bar >= 1);

  g_message ("Dummy Engine set up [samplerate: %u]", self->sample_rate);

  return 0;
}

int
engine_dummy_midi_setup (AudioEngine * self)
{
  g_message ("Setting up dummy MIDI engine");

  self->midi_buf_size = 4096;

  return 0;
}

int
engine_dummy_activate (AudioEngine * self, bool activate)
{
  if (activate)
    {
      g_message ("%s: activating...", __func__);

      self->stop_dummy_audio_thread = false;

      int beats_per_bar = tempo_track_get_beats_per_bar (P_TEMPO_TRACK);
      engine_update_frames_per_tick (
        self, beats_per_bar, tempo_track_get_current_bpm (P_TEMPO_TRACK),
        self->sample_rate, true, true, false);

      self->dummy_audio_thread = g_thread_new ("process_cb", process_cb, self);
    }
  else
    {
      g_message ("%s: deactivating...", __func__);

      self->stop_dummy_audio_thread = true;
      g_thread_join (self->dummy_audio_thread);
    }

  g_message ("%s: done", __func__);

  return 0;
}

void
engine_dummy_tear_down (AudioEngine * self)
{
}
