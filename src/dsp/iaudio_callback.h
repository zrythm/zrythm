// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>
#include <string_view>

#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Information about an audio device, provided when the device starts.
 *
 * Contains the device configuration that callbacks need to prepare for
 * processing.
 */
struct AudioDeviceInfo
{
  units::sample_rate_t   sample_rate;
  units::sample_u32_t    block_length;
  units::channel_count_t input_channel_count;
  units::channel_count_t output_channel_count;
};

/**
 * @brief Pure-abstract audio callback interface.
 *
 * Decouples the audio engine from any specific audio backend (e.g., JUCE).
 * Implementations receive audio I/O callbacks from the hardware audio
 * interface.
 */
class IAudioCallback
{
public:
  virtual ~IAudioCallback () = default;

  /**
   * @brief Called when the audio device wants to process a block of audio data.
   */
  virtual void process_audio (
    std::span<const float * const> input_channels,
    std::span<float * const>       output_channels,
    units::sample_u32_t            num_samples) noexcept = 0;

  /**
   * @brief Called when the audio device is about to start processing.
   *
   * @param info Device configuration (sample rate, buffer size, channel counts).
   */
  virtual void about_to_start (const AudioDeviceInfo &info) = 0;

  /**
   * @brief Called when the audio device has stopped.
   */
  virtual void stopped () = 0;

  /**
   * @brief Called when an error occurs with the audio device.
   */
  virtual void error (std::string_view error_message) = 0;
};

} // namespace zrythm::dsp
