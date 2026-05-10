// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cassert>

#include "dsp/hardware_audio_interface.h"
#include "dsp/iaudio_callback.h"
#include "utils/utf8_string.h"

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
    units::sample_rate_t   sample_rate = units::sample_rate (48000),
    units::sample_u32_t    block_length = units::samples (256),
    units::channel_count_t input_channels = units::channels (2),
    units::channel_count_t output_channels = units::channels (2))
      : sample_rate_ (sample_rate), block_length_ (block_length),
        input_channels_ (input_channels), output_channels_ (output_channels)
  {
  }

  [[nodiscard]] units::sample_u32_t get_block_length () const override
  {
    return block_length_;
  }
  [[nodiscard]] units::sample_rate_t get_sample_rate () const override
  {
    return sample_rate_;
  }
  [[nodiscard]] utils::Utf8String get_device_name () const override
  {
    return device_name_;
  }

  void set_device_name (utils::Utf8String name)
  {
    device_name_ = std::move (name);
  }

  void add_audio_callback (dsp::IAudioCallback * callback) override
  {
    assert (callback_ == nullptr);
    callback_ = callback;
    if (callback_ != nullptr)
      {
        callback_->about_to_start (make_device_info ());
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
  dsp::AudioDeviceInfo make_device_info () const
  {
    return {
      .sample_rate = sample_rate_,
      .block_length = block_length_,
      .input_channel_count = input_channels_,
      .output_channel_count = output_channels_,
    };
  }

  units::sample_rate_t   sample_rate_;
  units::sample_u32_t    block_length_;
  units::channel_count_t input_channels_;
  units::channel_count_t output_channels_;
  utils::Utf8String      device_name_;
  dsp::IAudioCallback *  callback_ = nullptr;
};

} // namespace zrythm::test_helpers
