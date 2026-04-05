// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <string_view>

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
   *
   * @param input_channel_data Array of pointers to input channel data
   * @param num_input_channels Number of input channels
   * @param output_channel_data Array of pointers to output channel data
   * @param num_output_channels Number of output channels
   * @param num_samples Number of samples in this block
   */
  virtual void process_audio (
    const float * const * input_channel_data,
    int                   num_input_channels,
    float * const *       output_channel_data,
    int                   num_output_channels,
    int                   num_samples) noexcept = 0;

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
