// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>

#include "dsp/engine.h"
#include "dsp/engine_dummy.h"
#include "dsp/port.h"
#include "dsp/tempo_track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "zrythm.h"
#include "zrythm_app.h"

#include "gtk_wrapper.h"

static void
process_cb (std::stop_token stop_token, AudioEngine &engine)
{
  double secs_per_block = (double) engine.block_length_ / engine.sample_rate_;
  gulong sleep_time = (gulong) (secs_per_block * 1000.0 * 1000);

  g_message ("Running dummy audio engine for first time");

  while (!stop_token.stop_requested ())
    {
      engine.process (engine.block_length_);
      std::this_thread::sleep_for (std::chrono::microseconds (sleep_time));
    }
}

int
engine_dummy_setup (AudioEngine * self)
{
  /* Set audio engine properties */
  self->midi_buf_size_ = 4096;

  if (ZRYTHM_HAVE_UI && zrythm_app->buf_size > 0)
    {
      self->block_length_ = (nframes_t) zrythm_app->buf_size;
    }
  else
    {
      self->block_length_ = 256;
    }

  if (zrythm_app->samplerate > 0)
    {
      self->sample_rate_ = (nframes_t) zrythm_app->samplerate;
    }
  else
    {
      self->sample_rate_ = 44100;
    }

  int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
  g_warn_if_fail (beats_per_bar >= 1);

  z_info ("Dummy Engine set up [samplerate: %u]", self->sample_rate_);

  return 0;
}

int
engine_dummy_midi_setup (AudioEngine * self)
{
  g_message ("Setting up dummy MIDI engine");

  self->midi_buf_size_ = 4096;

  return 0;
}

int
engine_dummy_activate (AudioEngine * self, bool activate)
{
  if (activate)
    {
      g_message ("%s: activating...", __func__);

      int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      self->update_frames_per_tick (
        beats_per_bar, P_TEMPO_TRACK->get_current_bpm (), self->sample_rate_,
        true, true, false);

      self->dummy_audio_thread_ =
        std::make_unique<std::jthread> (process_cb, std::ref (*self));
    }
  else
    {
      g_message ("%s: deactivating...", __func__);
      self->dummy_audio_thread_->request_stop ();
      self->dummy_audio_thread_->join ();
    }

  g_message ("%s: done", __func__);

  return 0;
}

void
engine_dummy_tear_down (AudioEngine * self)
{
}
