// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/units.h"
#include "utils/utf8_string.h"

namespace zrythm::dsp
{

/**
 * @brief Information about an audio device.
 *
 * Contains the device configuration such as sample rate, buffer size,
 * channel counts, and device name.
 */
struct AudioDeviceInfo
{
  utils::Utf8String      device_name;
  units::sample_rate_t   sample_rate;
  units::sample_u32_t    block_length;
  units::channel_count_t input_channel_count;
  units::channel_count_t output_channel_count;
};

} // namespace zrythm::dsp
