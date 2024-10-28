// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>

#include "common/dsp/engine.h"
#include "common/dsp/engine_dummy.h"
#include "common/dsp/port.h"
#include "common/dsp/tempo_track.h"
#include "common/dsp/tracklist.h"
#include "common/utils/dsp.h"
#include "common/utils/dsp_context.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"

class DummyEngineThread : public juce::Thread
{
public:
  DummyEngineThread (AudioEngine &engine)
      : juce::Thread ("DummyEngineThread"), engine_ (engine)
  {
  }

  void run () override
  {
    double secs_per_block =
      (double) engine_.block_length_ / engine_.sample_rate_;
    auto sleep_time = (gulong) (secs_per_block * 1000.0 * 1000);

    z_info ("Running dummy audio engine thread for first time");

    std::optional<DspContextRAII> dsp_context;
    if (ZRYTHM_USE_OPTIMIZED_DSP)
      {
        dsp_context.emplace ();
      }

    while (!threadShouldExit ())
      {
        engine_.process (engine_.block_length_);
        std::this_thread::sleep_for (std::chrono::microseconds (sleep_time));
      }

    z_info ("Exiting from dummy audio engine thread");
  }

public:
  AudioEngine &engine_;
};

int
engine_dummy_setup (AudioEngine * self)
{
  /* Set audio engine properties */
  self->midi_buf_size_ = 4096;

#if 0
  if (ZRYTHM_HAVE_UI && zrythm_app->buf_size_ > 0)
    {
      self->block_length_ = (nframes_t) zrythm_app->buf_size_;
    }
  else
    {
      self->block_length_ = 256;
    }

  if (zrythm_app->samplerate_ > 0)
    {
      self->sample_rate_ = (nframes_t) zrythm_app->samplerate_;
    }
  else
    {
      self->sample_rate_ = 44100;
    }
#endif

  int beats_per_bar =
    self->project_->tracklist_->tempo_track_->get_beats_per_bar ();
  z_warn_if_fail (beats_per_bar >= 1);

  z_info ("Dummy Engine set up [samplerate: {}]", self->sample_rate_);

  return 0;
}

int
engine_dummy_midi_setup (AudioEngine * self)
{
  z_info ("Setting up dummy MIDI engine");

  self->midi_buf_size_ = 4096;

  return 0;
}

int
engine_dummy_activate (AudioEngine * self, bool activate)
{
  if (activate)
    {
      z_info ("activating...");

      int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      self->update_frames_per_tick (
        beats_per_bar, P_TEMPO_TRACK->get_current_bpm (), self->sample_rate_,
        true, true, false);

      self->dummy_audio_thread_ = std::make_unique<DummyEngineThread> (*self);
      self->dummy_audio_thread_->startThread ();
    }
  else
    {
      z_info ("deactivating...");
      self->dummy_audio_thread_->signalThreadShouldExit ();
      self->dummy_audio_thread_->waitForThreadToExit (1'000);
    }

  z_info ("done");

  return 0;
}

void
engine_dummy_tear_down (AudioEngine * self)
{
}
