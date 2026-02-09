// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"
#include "utils/units.h"

#include <juce_wrapper.h>

namespace zrythm::dsp
{

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
  virtual nframes_t get_block_length () const = 0;

  /**
   * @brief Returns the current sample rate.
   */
  virtual units::sample_rate_t get_sample_rate () const = 0;

  /**
   * @brief Adds an audio callback to receive audio I/O events.
   */
  virtual void add_audio_callback (juce::AudioIODeviceCallback * callback) = 0;

  /**
   * @brief Removes an audio callback.
   */
  virtual void
  remove_audio_callback (juce::AudioIODeviceCallback * callback) = 0;

  /**
   * @brief Returns the audio workgroup for the current device (if available).
   */
  virtual juce::AudioWorkgroup get_device_audio_workgroup () const = 0;
};

} // namespace zrythm::dsp
