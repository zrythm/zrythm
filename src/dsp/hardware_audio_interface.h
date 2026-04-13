// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/units.h"

namespace zrythm::dsp
{

class IAudioCallback;

/**
 * @brief Abstraction for hardware audio interface.
 *
 * This interface decouples AudioEngine and DspGraphDispatcher from
 * juce::AudioDeviceManager, allowing for easier testing and flexibility.
 */
class IHardwareAudioInterface
{
public:
  virtual ~IHardwareAudioInterface () = default;

  /**
   * @brief Returns the current block length (buffer size) in frames.
   */
  virtual units::sample_u32_t get_block_length () const = 0;

  /**
   * @brief Returns the current sample rate.
   */
  virtual units::sample_rate_t get_sample_rate () const = 0;

  /**
   * @brief Adds an audio callback to receive audio I/O events.
   *
   * The caller must ensure @p callback remains alive until after
   * remove_audio_callback() is called with the same pointer.
   */
  virtual void add_audio_callback (IAudioCallback * callback) = 0;

  /**
   * @brief Removes a previously added audio callback.
   *
   * @p callback must be the same pointer passed to add_audio_callback().
   */
  virtual void remove_audio_callback (IAudioCallback * callback) = 0;
};

} // namespace zrythm::dsp
