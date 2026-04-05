// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/hardware_audio_interface.h"
#include "dsp/iaudio_callback.h"

namespace zrythm::test_helpers
{

/**
 * @brief Mock hardware audio interface for unit tests.
 *
 * Returns fixed sample rate and block length values without any
 * JUCE or ALSA/MIDI dependencies.
 */
class MockHardwareAudioInterface : public dsp::IHardwareAudioInterface
{
public:
  explicit MockHardwareAudioInterface (
    units::sample_rate_t sample_rate = units::sample_rate (48000),
    nframes_t            block_length = 256)
      : sample_rate_ (sample_rate), block_length_ (block_length)
  {
  }

  [[nodiscard]] nframes_t get_block_length () const override
  {
    return block_length_;
  }
  [[nodiscard]] units::sample_rate_t get_sample_rate () const override
  {
    return sample_rate_;
  }

  void add_audio_callback (dsp::IAudioCallback * callback) override
  {
    assert (callback_ == nullptr);
    callback_ = callback;
    if (callback_ != nullptr)
      {
        callback_->about_to_start ();
      }
  }

  void remove_audio_callback (dsp::IAudioCallback * callback) override
  {
    assert (callback == callback_);
    if (callback_ != nullptr)
      {
        callback_->stopped ();
      }
    callback_ = nullptr;
  }

private:
  units::sample_rate_t  sample_rate_;
  nframes_t             block_length_;
  dsp::IAudioCallback * callback_ = nullptr;
};

} // namespace zrythm::test_helpers
