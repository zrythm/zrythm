// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/audio_device_info.h"

namespace zrythm::dsp
{

class IAudioCallback;

/**
 * @brief Abstraction for hardware audio interface.
 *
 * This interface decouples AudioEngine and DspGraphDispatcher from
 * juce::AudioDeviceManager, allowing for easier testing and flexibility.
 *
 * The hardware interface is the source of truth for device configuration.
 * It guarantees that get_device_info() returns up-to-date values during
 * any about_to_start() callback.
 */
class IHardwareAudioInterface
{
public:
  virtual ~IHardwareAudioInterface () = default;

  /**
   * @brief Returns the current audio device information.
   *
   * Guaranteed to return up-to-date values during about_to_start() callbacks.
   */
  [[nodiscard]] virtual AudioDeviceInfo get_device_info () const = 0;

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
