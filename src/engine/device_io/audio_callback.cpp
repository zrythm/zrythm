// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <utility>

#include "engine/device_io/audio_callback.h"

namespace zrythm::engine::device_io
{
AudioCallback::AudioCallback (EngineProcessCallback process_cb)
    : process_cb_ (std::move (process_cb))
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
  for (const auto ch : std::views::iota (0, numInputChannels))
    {
      auto * ch_data = outputChannelData[ch];
      utils::float_ranges::fill (ch_data, 0.f, numSamples);
    }

  process_cb_ (numSamples);

  // TODO: forward master ouptut to outputChannelData
}

void
AudioCallback::audioDeviceAboutToStart (juce::AudioIODevice * device)
{
  z_debug (
    "{} device '{}' about to start", device->getTypeName (), device->getName ());
}

void
AudioCallback::audioDeviceStopped ()
{
  z_debug ("audio device stopped");
}

void
AudioCallback::audioDeviceError (const juce::String &errorMessage)
{
  z_error ("audio device error: {}", errorMessage);
}
} // namespace zrythm::engine::device_io
