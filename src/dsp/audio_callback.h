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
    std::span<const float * const> inputChannels,
    std::span<float * const>       outputChannels,
    units::sample_u32_t            numSamples)>;
  using DeviceAboutToStartCallback =
    std::function<void (const AudioDeviceInfo &)>;
  using DeviceStoppedCallback = std::function<void ()>;

  AudioCallback (
    EngineProcessCallback      process_cb,
    DeviceAboutToStartCallback device_about_to_start_cb,
    DeviceStoppedCallback      device_stopped_cb);

public:
  void process_audio (
    std::span<const float * const> input_channels,
    std::span<float * const>       output_channels,
    units::sample_u32_t            num_samples) noexcept override;
  void about_to_start (const AudioDeviceInfo &info) override;
  void stopped () override;
  void error (std::string_view error_message) override;

private:
  EngineProcessCallback                     process_cb_;
  std::optional<DeviceAboutToStartCallback> device_about_to_start_cb_;
  std::optional<DeviceStoppedCallback>      device_stopped_cb_;
};
} // namespace zrythm::dsp
