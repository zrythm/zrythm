// SPDX-FileCopyrightText: © 2020-2022, 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Optimized DSP functions.
 *
 * @note More at
 * https://github.com/DISTRHO/DPF-Max-Gen/blob/master/plugins/common/gen_dsp/genlib_ops.h#L313
 */

#pragma once

#include <cstdint>
#include <span>

namespace zrythm::utils::float_ranges
{

/**
 * Fill the buffer with the given value.
 */
[[using gnu: hot]] void
fill (std::span<float> buf, float val);

/**
 * Clamp the buffer to min/max.
 */
[[using gnu: hot]] void
clip (std::span<float> buf, float minf, float maxf);

/**
 * Compute dest[i] = src[i].
 */
[[using gnu: hot]] void
copy (std::span<float> dest, std::span<const float> src);

/**
 * Scale: dst[i] = dst[i] * k.
 */
[[using gnu: hot]] void
mul_k2 (std::span<float> dest, float k);

/**
 * Gets the maximum absolute value of the buffer (as amplitude).
 */
[[nodiscard]] float
abs_max (std::span<const float> buf);

/**
 * Gets the minimum of the buffer.
 */
float
min (std::span<const float> buf);

/**
 * Gets the maximum of the buffer.
 */
float
max (std::span<const float> buf);

/**
 * Calculate dst[i] = dst[i] + src[i].
 */
[[using gnu: hot]] void
add2 (std::span<float> dest, std::span<const float> src);

/**
 * @brief Calculate dest[i] = src[i] * k.
 */
[[using gnu: hot]] void
product (std::span<float> dest, std::span<const float> src, float k);

/**
 * @brief Calculate dest[i] = dest[i] + src[i] * k.
 */
[[using gnu: hot]] void
mix_product (std::span<float> dest, std::span<const float> src, float k);

/**
 * Reverse the order of samples: dst[i] <=> src[count - i - 1].
 */
[[using gnu: hot]] void
reverse (std::span<float> dest, std::span<const float> src);

/**
 * Calculate normalized values:
 * dst[i] = src[i] / (max { abs(src) }).
 */
[[using gnu: hot]] void
normalize (std::span<float> dest, std::span<const float> src);

/**
 * Calculate linear fade by multiplying from 0 to 1 for
 * total_frames_to_fade samples.
 *
 * @note Does not work properly when
 *   fade_from_multiplier is < 1k.
 *
 * @param dest Span of frames to process (size is the number
 *   of frames).
 * @param start_offset Start offset in the fade interval.
 * @param total_frames_to_fade Total frames that should be
 *   faded.
 * @param fade_from_multiplier Multiplier to fade from (0 to
 *   fade from silence.)
 */
void
linear_fade_in_from (
  std::span<float> dest,
  int32_t          start_offset,
  int32_t          total_frames_to_fade,
  float            fade_from_multiplier);

/**
 * Calculate linear fade by multiplying from 1 to 0 for
 * total_frames_to_fade samples.
 *
 * @param dest Span of frames to process (size is the number
 *   of frames).
 * @param start_offset Start offset in the fade interval.
 * @param total_frames_to_fade Total frames that should be
 *   faded.
 * @param fade_to_multiplier Multiplier to fade to (0 to fade
 *   to silence.)
 */
void
linear_fade_out_to (
  std::span<float> dest,
  int32_t          start_offset,
  int32_t          total_frames_to_fade,
  float            fade_to_multiplier);

/**
 * Makes the two signals mono.
 *
 * @param equal_power True for equal power, false for equal
 * amplitude.
 *
 * @note Equal amplitude is more suitable for mono
 * compatibility checking. For reference:
 * - equal power sum = (L+R) * 0.7079 (-3dB)
 * - equal amplitude sum = (L+R) /2 (-6.02dB)
 */
void
make_mono (std::span<float> l, std::span<float> r, bool equal_power);

}; // zrythm::utils::float_ranges
