// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cassert>

#include "dsp/hardware_audio_interface.h"
#include "dsp/iaudio_callback.h"

namespace zrythm::test_helpers
{

/**
 * @brief Mock hardware audio interface for unit tests.
 *
 * Returns fixed device info without any audio backend dependencies.
 */
class MockHardwareAudioInterface : public dsp::IHardwareAudioInterface
{
public:
  explicit MockHardwareAudioInterface (
    units::sample_rate_t   sample_rate = units::sample_rate (48000),
    units::sample_u32_t    block_length = units::samples (256),
    units::channel_count_t input_channels = units::channels (2),
    units::channel_count_t output_channels = units::channels (2))
      : device_info_{
          .device_name = {},
          .sample_rate = sample_rate,
          .block_length = block_length,
          .input_channel_count = input_channels,
          .output_channel_count = output_channels,
        }
  {
  }

  [[nodiscard]] dsp::AudioDeviceInfo get_device_info () const override
  {
    return device_info_;
  }

  /**
   * @brief Updates the device info.
   *
   * Must not be called while audio processing is active (between
   * about_to_start() and stopped()).
   */
  void set_device_info (dsp::AudioDeviceInfo info)
  {
    assert (!processing_active_);
    device_info_ = std::move (info);
  }

  void add_audio_callback (dsp::IAudioCallback * callback) override
  {
    assert (callback_ == nullptr);
    callback_ = callback;
    if (callback_ != nullptr)
      {
        processing_active_ = true;
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
    processing_active_ = false;
  }

private:
  dsp::AudioDeviceInfo  device_info_;
  dsp::IAudioCallback * callback_ = nullptr;
  bool                  processing_active_ = false;
};

} // namespace zrythm::test_helpers
