// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>
#include <ranges>
#include <utility>

#include "dsp/audio_callback.h"
#include "utils/float_ranges.h"
#include "utils/logger.h"

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
AudioCallback::process_audio (
  std::span<const float * const> input_channels,
  std::span<float * const>       output_channels,
  units::sample_u32_t            num_samples) noexcept
{
  for (auto * ch_data : output_channels)
    {
      utils::float_ranges::fill (
        { ch_data, num_samples.in<size_t> (units::samples) }, 0.f);
    }

  process_cb_ (input_channels, output_channels, num_samples);
}

void
AudioCallback::about_to_start ()
{
  if (device_about_to_start_cb_.has_value ())
    {
      std::invoke (device_about_to_start_cb_.value ());
    }
}

void
AudioCallback::stopped ()
{
  if (device_stopped_cb_.has_value ())
    {
      std::invoke (device_stopped_cb_.value ());
    }
}

void
AudioCallback::error (std::string_view error_message)
{
  z_warning ("audio device error: {}", error_message);
}
} // namespace zrythm::dsp
