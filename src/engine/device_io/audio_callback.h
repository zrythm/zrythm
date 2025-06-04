// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

namespace zrythm::engine::device_io
{
class AudioCallback : public juce::AudioIODeviceCallback
{
public:
  using EngineProcessCallback = std::function<void (nframes_t)>;

  AudioCallback (EngineProcessCallback process_cb);

public:
  void audioDeviceIOCallbackWithContext (
    const float * const *                     inputChannelData,
    int                                       numInputChannels,
    float * const *                           outputChannelData,
    int                                       numOutputChannels,
    int                                       numSamples,
    const juce::AudioIODeviceCallbackContext &context) override;
  void audioDeviceAboutToStart (juce::AudioIODevice * device) override;
  void audioDeviceStopped () override;
  void audioDeviceError (const juce::String &errorMessage) override;

private:
  EngineProcessCallback process_cb_;
};
} // namespace zrythm::engine::device_io
