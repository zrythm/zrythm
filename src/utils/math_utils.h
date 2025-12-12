// SPDX-FileCopyrightText: © 2018-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
 * Copyright (C) 2017-2019 Robin Gareus <robin@gareus.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ---
 */

#pragma once

#include <cmath>
#include <limits>
#include <numbers>
#include <string>

#include "utils/types.h"

/**
 * @brief Math utils.
 *
 * For more, look at libs/pbd/pbd/control_math.h in ardour.
 */
namespace zrythm::utils::math
{

/**
 * Frames to skip when calculating the RMS.
 *
 * The lower the more CPU intensive.
 */
constexpr unsigned RMS_FRAMES = 1;

/** Tiny number to be used for denormaml prevention
 * (-140dB). */
constexpr float ALMOST_SILENCE = 0.0000001f;

constexpr auto MINUS_INFINITY = -HUGE_VAL;

/**
 * Returns whether 2 floating point numbers are equal.
 *
 * @param e The allowed difference (epsilon).
 */
template <std::floating_point T>
constexpr bool
floats_near (T a, T b, T e)
{
  return std::abs (a - b) < e;
}

/**
 * Checks if 2 floating point numbers are equal.
 */
template <std::floating_point T>
constexpr bool
floats_equal (T a, T b)
{
  return floats_near (a, b, std::numeric_limits<T>::epsilon ());
}

/**
 * Rounds a double to a (minimum) signed 32-bit integer.
 */
template <std::floating_point T>
constexpr long
round_to_signed_32 (T x)
{
  if constexpr (std::is_same_v<T, float>)
    {
      return lroundf (x);
    }
  else
    {
      return lround (x);
    }
}

/**
 * Rounds a double to a (minimum) signed 64-bit integer.
 */
template <std::floating_point T>
constexpr long long
round_to_signed_64 (T x)
{
  if constexpr (std::is_same_v<T, float>)
    {
      return llroundf (x);
    }
  else
    {
      return llround (x);
    }
}

/**
 * Fast log calculation to be used where precision
 * is not required (like log curves).
 *
 * Taken from ardour from code in the public domain.
 */
[[gnu::const]]
constexpr float
fast_log2 (float val)
{
  union
  {
    float f;
    int   i;
  } t{};
  t.f = val;
  int * const exp_ptr = &t.i;
  int         x = *exp_ptr;
  const auto  log_2 = (float) (((x >> 23) & 255) - 128);

  x &= ~(255 << 23);
  x += 127 << 23;

  *exp_ptr = x;

  val = ((-1.0f / 3) * t.f + 2) * t.f - 2.0f / 3;

  return (val + log_2);
}

[[gnu::const]]
constexpr auto
fast_log (const float val)
{
  return (fast_log2 (val) * std::numbers::ln2_v<float>);
}

[[gnu::const]]
constexpr auto
fast_log10 (const float val)
{
  return fast_log2 (val) / 3.312500f;
}

/**
 * Returns fader value 0.0 to 1.0 from amp value
 * 0.0 to 2.0 (+6 dbFS).
 */
template <std::floating_point T>
[[gnu::const]]
static inline T
get_fader_val_from_amp (T amp)
{
  constexpr T fader_coefficient1 =
    /*192.f * logf (2.f);*/
    133.084258667509499408f;
  constexpr T fader_coefficient2 =
    /*powf (logf (2.f), 8.f) * powf (198.f, 8.f);*/
    1.25870863180257576e17f;

  /* to prevent weird values when amp is very
   * small */
  if (amp <= 0.00001f)
    {
      return 1e-20f;
    }

  if (floats_equal (amp, 1.f))
    {
      amp = 1.f + 1e-20f;
    }
  T fader =
    std::powf (
      // note: don't use fast_log here - it causes weirdness in faders
      (6.f * std::logf (amp)) + fader_coefficient1, 8.f)
    / fader_coefficient2;
  return fader;
}

/**
 * Returns amp value 0.0 to 2.0 (+6 dbFS) from fader value 0.0 to 1.0.
 */
template <std::floating_point T>
[[gnu::const]]
static inline T
get_amp_val_from_fader (T fader)
{
  constexpr float val1 = 1.f / 6.f;
  return std::powf (
    2.f, (val1) * (-192.f + 198.f * std::powf (fader, 1.f / 8.f)));
}

/**
 * Convert from amplitude 0.0 to 2.0 to dbFS.
 */
template <std::floating_point T>
[[gnu::const]]
static inline T
amp_to_dbfs (T amp)
{
  return 20.f * std::log10f (amp);
}

/**
 * Gets the RMS of the given signal as amplitude
 * (0-2).
 */
float
calculate_rms_amp (const float * buf, nframes_t nframes);

/**
 * Calculate db using RMS method.
 *
 * @param buf Buffer containing the samples.
 * @param nframes Number of samples.
 */
template <std::floating_point T>
static inline T
calculate_rms_db (const T * buf, nframes_t nframes)
{
  return amp_to_dbfs (calculate_rms_amp (buf, nframes));
}

/**
 * Convert form dbFS to amplitude 0.0 to 2.0.
 */
template <std::floating_point T>
[[gnu::const]]
static inline T
dbfs_to_amp (T dbfs)
{
  return std::powf (10.f, (dbfs / 20.f));
}

/**
 * Convert form dbFS to fader val 0.0 to 1.0.
 */
template <std::floating_point T>
[[gnu::const]]
static inline T
dbfs_to_fader_val (T dbfs)
{
  return get_fader_val_from_amp (dbfs_to_amp (dbfs));
}

/**
 * Asserts that the value is non-nan.
 *
 * Not real-time safe.
 *
 * @return Whether the value is valid (nonnan).
 */
bool
assert_nonnann (float x);

/**
 * Returns whether the given string is a valid float.
 *
 * @param ret If non-nullptr, the result will be placed here.
 */
bool
is_string_valid_float (const std::string &str, float * ret);

} // namespace zrythm::utils::math
