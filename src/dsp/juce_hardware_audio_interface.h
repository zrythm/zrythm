// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>

#include "dsp/hardware_audio_interface.h"

#include <juce_wrapper.h>

namespace zrythm::dsp
{

/**
 * @brief JUCE-based implementation of IHardwareAudioInterface.
 *
 * Wraps a juce::AudioDeviceManager to provide the hardware audio interface.
 */
class JuceHardwareAudioInterface : public IHardwareAudioInterface
{
public:
  explicit JuceHardwareAudioInterface (
    std::shared_ptr<juce::AudioDeviceManager> device_manager);

  ~JuceHardwareAudioInterface () override;

  [[nodiscard]] nframes_t            get_block_length () const override;
  [[nodiscard]] units::sample_rate_t get_sample_rate () const override;

  void add_audio_callback (juce::AudioIODeviceCallback * callback) override;

  void remove_audio_callback (juce::AudioIODeviceCallback * callback) override;

  juce::AudioWorkgroup get_device_audio_workgroup () const override;

  /**
   * @brief Creates a JUCE-based hardware audio interface.
   */
  static std::unique_ptr<IHardwareAudioInterface>
  create (std::shared_ptr<juce::AudioDeviceManager> device_manager);

private:
  std::shared_ptr<juce::AudioDeviceManager> device_manager_;
};

} // namespace zrythm::dsp
