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
  const float * const * input_channel_data,
  int                   num_input_channels,
  float * const *       output_channel_data,
  int                   num_output_channels,
  int                   num_samples) noexcept
{
  assert (num_samples >= 0);
  const auto samples = static_cast<size_t> (num_samples);

  for (const auto ch : std::views::iota (0, num_output_channels))
    {
      auto * ch_data = output_channel_data[ch];
      utils::float_ranges::fill ({ ch_data, samples }, 0.f);
    }

  process_cb_ (
    input_channel_data, num_input_channels, output_channel_data,
    num_output_channels, num_samples);
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
