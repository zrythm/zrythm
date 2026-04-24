// SPDX-FileCopyrightText: © 2020-2021, 2024, 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cassert>
#include <ranges>

#include "utils/float_ranges.h"
#include "utils/math_utils.h"

#include <juce_dsp/juce_dsp.h>

namespace zrythm::utils::float_ranges
{

void
fill (std::span<float> buf, float val)
{
  juce::FloatVectorOperations::fill (buf.data (), val, buf.size ());
}

void
clip (std::span<float> buf, float minf, float maxf)
{
  juce::FloatVectorOperations::clip (
    buf.data (), buf.data (), minf, maxf, buf.size ());
}

void
copy (std::span<float> dest, std::span<const float> src)
{
  assert (dest.size () == src.size ());
  juce::FloatVectorOperations::copy (dest.data (), src.data (), dest.size ());
}

void
mul_k2 (std::span<float> dest, float k)
{
  juce::FloatVectorOperations::multiply (dest.data (), k, dest.size ());
}

float
abs_max (std::span<const float> buf)
{
  auto min_and_max =
    juce::FloatVectorOperations::findMinAndMax (buf.data (), buf.size ());
  return std::max (
    std::abs (min_and_max.getStart ()), std::abs (min_and_max.getEnd ()));
}

float
min (std::span<const float> buf)
{
  return juce::FloatVectorOperations::findMinimum (buf.data (), buf.size ());
}

float
max (std::span<const float> buf)
{
  return juce::FloatVectorOperations::findMaximum (buf.data (), buf.size ());
}

void
add2 (std::span<float> dest, std::span<const float> src)
{
  assert (dest.size () == src.size ());
  juce::FloatVectorOperations::add (dest.data (), src.data (), dest.size ());
}

void
product (std::span<float> dest, std::span<const float> src, float k)
{
  assert (dest.size () == src.size ());
  juce::FloatVectorOperations::copyWithMultiply (
    dest.data (), src.data (), k, dest.size ());
}

void
mix_product (std::span<float> dest, std::span<const float> src, float k)
{
  assert (dest.size () == src.size ());
  juce::FloatVectorOperations::addWithMultiply (
    dest.data (), src.data (), k, dest.size ());
}

void
reverse (std::span<float> dest, std::span<const float> src)
{
  assert (dest.size () == src.size ());
  std::reverse_copy (src.data (), src.data () + src.size (), dest.data ());
}

void
normalize (std::span<float> dest, std::span<const float> src)
{
  assert (dest.size () == src.size ());
  copy (dest, src);
  const float abs_peak = abs_max (dest);
  if (abs_peak > 0.f)
    mul_k2 (dest, 1.f / abs_peak);
}

void
linear_fade_in_from (
  std::span<float> dest,
  int32_t          start_offset,
  int32_t          total_frames_to_fade,
  float            fade_from_multiplier)
{
  const auto size = dest.size ();
  assert (total_frames_to_fade > 1);
  for (const auto i : std::views::iota (0zu, size))
    {
      float k =
        (float) (i + (size_t) start_offset) / (float) (total_frames_to_fade - 1);
      k = fade_from_multiplier + (1.f - fade_from_multiplier) * k;
      dest[i] *= k;
    }
}

void
linear_fade_out_to (
  std::span<float> dest,
  int32_t          start_offset,
  int32_t          total_frames_to_fade,
  float            fade_to_multiplier)
{
  const auto size = dest.size ();
  assert (total_frames_to_fade > 1);
  for (const auto i : std::views::iota (0zu, size))
    {
      float k =
        (float) ((size_t) total_frames_to_fade - (i + (size_t) start_offset) - 1)
        / (float) (total_frames_to_fade - 1);
      k = fade_to_multiplier + (1.f - fade_to_multiplier) * k;
      dest[i] *= k;
    }
}

void
make_mono (std::span<float> l, std::span<float> r, bool equal_power)
{
  assert (l.size () == r.size ());
  float multiple = equal_power ? 0.7079f : 0.5f;
  add2 (l, r);
  mul_k2 (l, multiple);
  copy (r, l);
}

} // zrythm::utils::float_ranges
