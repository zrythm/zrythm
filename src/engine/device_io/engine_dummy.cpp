// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// SPDX-FileCopyrightText: © 2019-2021, 2024-2025 Alexandros Theodotou <alex@zrythm.org>

#include "engine/device_io/engine_dummy.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/port.h"
#include "structure/tracks/tempo_track.h"
#include "structure/tracks/tracklist.h"
#include "utils/dsp.h"
#include "utils/dsp_context.h"

namespace zrythm::engine::device_io
{

class DummyEngineThread : public juce::Thread
{
public:
  DummyEngineThread (DummyDriver &driver)
      : juce::Thread ("DummyEngineThread"), driver_ (driver),
        engine_ (driver.engine_)
  {
  }

  void run () override
  {
    double secs_per_block =
      (double) engine_.block_length_ / engine_.sample_rate_;
    auto sleep_time = (unsigned_frame_t) (secs_per_block * 1000.0 * 1000);

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
  DummyDriver &driver_;
  AudioEngine &engine_;
};

bool
DummyDriver::setup_audio ()
{
  /* Set audio engine properties */
  engine_.midi_buf_size_ = 4096;

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
    engine_.project_->tracklist_->tempo_track_->get_beats_per_bar ();
  z_warn_if_fail (beats_per_bar >= 1);

  z_info ("Dummy Engine set up [samplerate: {}]", engine_.sample_rate_);

  return true;
}

bool
DummyDriver::setup_midi ()
{
  z_info ("Setting up dummy MIDI engine");

  engine_.midi_buf_size_ = 4096;

  return true;
}

bool
DummyDriver::activate_audio (bool activate)
{
  if (activate)
    {
      z_info ("activating...");

      int beats_per_bar = P_TEMPO_TRACK->get_beats_per_bar ();
      engine_.update_frames_per_tick (
        beats_per_bar, P_TEMPO_TRACK->get_current_bpm (), engine_.sample_rate_,
        true, true, false);

      dummy_audio_thread_ = std::make_unique<DummyEngineThread> (*this);
      dummy_audio_thread_->startThread ();
    }
  else
    {
      z_info ("deactivating...");
      dummy_audio_thread_->signalThreadShouldExit ();
      dummy_audio_thread_->waitForThreadToExit (1'000);
    }

  z_info ("done");

  return true;
}

void
DummyDriver::tear_down_audio ()
{
}
}
