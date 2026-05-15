// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>
#include <string_view>

#include "utils/units.h"

namespace zrythm::dsp
{

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
   */
  virtual void about_to_start () = 0;

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
