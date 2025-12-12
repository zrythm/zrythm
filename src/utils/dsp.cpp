// SPDX-FileCopyrightText: © 2020-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-config.h"

#include <ranges>

#include "utils/dsp.h"
#include "utils/math_utils.h"

namespace zrythm::utils::float_ranges
{

/**
 * Calculate linear fade by multiplying from 0 to 1 for
 * @param total_frames_to_fade samples.
 *
 * @note Does not work properly when @param
 *   fade_from_multiplier is < 1k.
 *
 * @param start_offset Start offset in the fade interval.
 * @param total_frames_to_fade Total frames that should be
 *   faded.
 * @param size Number of frames to process.
 * @param fade_from_multiplier Multiplier to fade from (0 to
 *   fade from silence.)
 */
void
linear_fade_in_from (
  float * dest,
  int32_t start_offset,
  int32_t total_frames_to_fade,
  size_t  size,
  float   fade_from_multiplier)
{
  // TODO check juce::AudioSampleBuffer::applyGainRamp ()

  for (size_t i = 0; i < size; ++i)
    {
      float k =
        (float) (i + (size_t) start_offset) / (float) (total_frames_to_fade - 1);
      k = fade_from_multiplier + (1.f - fade_from_multiplier) * k;
      dest[i] *= k;
    }
}

/**
 * Calculate linear fade by multiplying from 0 to 1 for
 * @param total_frames_to_fade samples.
 *
 * @param start_offset Start offset in the fade interval.
 * @param total_frames_to_fade Total frames that should be
 *   faded.
 * @param size Number of frames to process.
 * @param fade_to_multiplier Multiplier to fade to (0 to fade
 *   to silence.)
 */
[[gnu::nonnull]] void
linear_fade_out_to (
  float * dest,
  int32_t start_offset,
  int32_t total_frames_to_fade,
  size_t  size,
  float   fade_to_multiplier)
{
  // TODO check juce::AudioSampleBuffer::applyGainRamp ()

  for (const auto i : std::views::iota (0_zu, size))
    {
      float k =
        (float) ((size_t) total_frames_to_fade - (i + (size_t) start_offset) - 1)
        / (float) (total_frames_to_fade - 1);
      k = fade_to_multiplier + (1.f - fade_to_multiplier) * k;
      dest[i] *= k;
    }
}

/**
 * Makes the two signals mono.
 *
 * @param equal_power True for equal power, false for equal
 *   amplitude.
 *
 * @note Equal amplitude is more suitable for mono
 * compatibility checking. For reference:
 * - equal power sum = (L+R) * 0.7079 (-3dB)
 * - equal amplitude sum = (L+R) /2 (-6.02dB)
 */
void
make_mono (float * l, float * r, size_t size, bool equal_power, bool optimize)
{
  float multiple = equal_power ? 0.7079f : 0.5f;
  add2 (l, r, size, optimize);
  mul_k2 (l, multiple, size, optimize);
  copy (r, l, size, optimize);
}

} // zrythm::dsp::float_ranges
