// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <optional>

#include "dsp/iaudio_callback.h"

namespace zrythm::dsp
{
class AudioCallback : public IAudioCallback
{
public:
  using EngineProcessCallback = std::function<void (
    const float * const * inputChannelData,
    int                   numInputChannels,
    float * const *       outputChannelData,
    int                   numOutputChannels,
    int                   numSamples)>;
  using DeviceAboutToStartCallback = std::function<void ()>;
  using DeviceStoppedCallback = std::function<void ()>;

  AudioCallback (
    EngineProcessCallback      process_cb,
    DeviceAboutToStartCallback device_about_to_start_cb,
    DeviceStoppedCallback      device_stopped_cb);

public:
  void process_audio (
    const float * const * input_channel_data,
    int                   num_input_channels,
    float * const *       output_channel_data,
    int                   num_output_channels,
    int                   num_samples) noexcept override;
  void about_to_start () override;
  void stopped () override;
  void error (std::string_view error_message) override;

private:
  EngineProcessCallback                     process_cb_;
  std::optional<DeviceAboutToStartCallback> device_about_to_start_cb_;
  std::optional<DeviceStoppedCallback>      device_stopped_cb_;
};
} // namespace zrythm::dsp
