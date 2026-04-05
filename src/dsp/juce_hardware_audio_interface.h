// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <memory>
#include <optional>
#include <unordered_map>

#include "dsp/hardware_audio_interface.h"
#include "dsp/iaudio_callback.h"

#include <juce_wrapper.h>

namespace zrythm::dsp
{

/**
 * @brief JUCE-based implementation of IHardwareAudioInterface.
 *
 * Wraps a juce::AudioDeviceManager to provide the hardware audio interface.
 * Bridges IAudioCallback to juce::AudioIODeviceCallback via an internal
 * adapter.
 */
class JuceHardwareAudioInterface : public IHardwareAudioInterface
{
public:
  explicit JuceHardwareAudioInterface (
    std::shared_ptr<juce::AudioDeviceManager> device_manager);

  ~JuceHardwareAudioInterface () override;

  [[nodiscard]] nframes_t            get_block_length () const override;
  [[nodiscard]] units::sample_rate_t get_sample_rate () const override;

  void add_audio_callback (IAudioCallback * callback) override;
  void remove_audio_callback (IAudioCallback * callback) override;

  /**
   * @brief Returns the audio workgroup for the current device.
   *
   * This is not part of the IHardwareAudioInterface interface to avoid
   * JUCE dependencies in the interface. Callers that need the workgroup
   * should use this method on the concrete type.
   */
  [[nodiscard]] std::optional<juce::AudioWorkgroup>
  get_device_audio_workgroup () const;

  /**
   * @brief Creates a JUCE-based hardware audio interface.
   */
  static std::unique_ptr<IHardwareAudioInterface>
  create (std::shared_ptr<juce::AudioDeviceManager> device_manager);

private:
  /**
   * @brief Adapter that bridges IAudioCallback to
   * juce::AudioIODeviceCallback.
   */
  class JuceCallbackAdapter : public juce::AudioIODeviceCallback
  {
  public:
    explicit JuceCallbackAdapter (IAudioCallback &callback)
        : callback_ (callback)
    {
    }

    void audioDeviceIOCallbackWithContext (
      const float * const *                     inputChannelData,
      int                                       numInputChannels,
      float * const *                           outputChannelData,
      int                                       numOutputChannels,
      int                                       numSamples,
      const juce::AudioIODeviceCallbackContext &context) override
    {
      juce::ScopedNoDenormals no_denormals;
      callback_.process_audio (
        inputChannelData, numInputChannels, outputChannelData,
        numOutputChannels, numSamples);
    }

    void audioDeviceAboutToStart (juce::AudioIODevice * device) override;
    void audioDeviceStopped () override;
    void audioDeviceError (const juce::String &errorMessage) override;

  private:
    IAudioCallback &callback_;
  };

  std::shared_ptr<juce::AudioDeviceManager> device_manager_;

  /** Maps IAudioCallback pointers to their JUCE adapters. */
  std::unordered_map<IAudioCallback *, std::unique_ptr<JuceCallbackAdapter>>
    callback_adapters_;
};

} // namespace zrythm::dsp
