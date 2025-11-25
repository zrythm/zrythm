// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "dsp/audio_callback.h"

namespace zrythm::dsp
{
AudioCallback::AudioCallback (
  EngineProcessCallback      process_cb,
  DeviceAboutToStartCallback device_about_to_start_cb,
  DeviceStoppedCallback      device_stopped_cb)
    : process_cb_ (std::move (process_cb)),
      device_about_to_start_cb_ (device_about_to_start_cb),
      device_stopped_cb_ (device_stopped_cb)
{
}

void
AudioCallback::audioDeviceIOCallbackWithContext (
  const float * const *                     inputChannelData,
  int                                       numInputChannels,
  float * const *                           outputChannelData,
  int                                       numOutputChannels,
  int                                       numSamples,
  const juce::AudioIODeviceCallbackContext &context)
{
  juce::ScopedNoDenormals no_denormals;

  // Clear buffers
  for (const auto ch : std::views::iota (0, numOutputChannels))
    {
      auto * ch_data = outputChannelData[ch];
      utils::float_ranges::fill (ch_data, 0.f, numSamples);
    }

  process_cb_ (
    inputChannelData, numInputChannels, outputChannelData, numOutputChannels,
    numSamples);
}

void
AudioCallback::audioDeviceAboutToStart (juce::AudioIODevice * device)
{
  z_info (
    "{} device '{}' about to start", device->getTypeName (), device->getName ());
  if (device_about_to_start_cb_.has_value ())
    {
      std::invoke (device_about_to_start_cb_.value (), device);
    }
}

void
AudioCallback::audioDeviceStopped ()
{
  z_info ("audio device stopped");
  if (device_stopped_cb_.has_value ())
    {
      std::invoke (device_stopped_cb_.value ());
    }
}

void
AudioCallback::audioDeviceError (const juce::String &errorMessage)
{
  z_warning ("audio device error: {}", errorMessage);
}
} // namespace zrythm::engine::device_io
