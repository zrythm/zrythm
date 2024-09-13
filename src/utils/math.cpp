// SPDX-FileCopyrightText: © 2018-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cfloat>
#include <cmath>

#include "utils/logger.h"
#include "utils/math.h"
#include "utils/string.h"

#include "gtk_wrapper.h"

/**
 * Gets the RMS of the given signal as amplitude
 * (0-2).
 */
sample_t
math_calculate_rms_amp (sample_t * buf, const nframes_t nframes)
{
  sample_t sum = 0, sample = 0;
  for (unsigned int i = 0; i < nframes; i += MATH_RMS_FRAMES)
    {
      sample = buf[i];
      sum += (sample * sample);
    }
  return sqrtf (sum / ((sample_t) nframes / (sample_t) MATH_RMS_FRAMES));
}

/**
 * Calculate db using RMS method.
 *
 * @param buf Buffer containing the samples.
 * @param nframes Number of samples.
 */
sample_t
math_calculate_rms_db (sample_t * buf, const nframes_t nframes)
{
  return math_amp_to_dbfs (math_calculate_rms_amp (buf, nframes));
}

/**
 * Asserts that the value is non-nan.
 *
 * Not real-time safe.
 *
 * @return Whether the value is valid (nonnan).
 */
bool
math_assert_nonnann (float x)
{
  auto val = fmt::format ("{:f}", (double) x);
  if (std::isnan (x) || string_contains_substr (val.c_str (), "nan"))
    {
      z_error ("nan");
      return false;
    }
  if (!std::isfinite (x) || string_contains_substr (val.c_str (), "inf"))
    {
      z_error ("inf");
      return false;
    }
  return true;
}

/**
 * Returns whether the given string is a valid float.
 *
 * @param ret If non-nullptr, the result will be placed here.
 */
bool
math_is_string_valid_float (const std::string &str, float * ret)
{
  int   len;
  float dummy = 0.0;
  if (
    sscanf (str.c_str (), "%f %n", &dummy, &len) == 1
    && len == (int) strlen (str.c_str ()))
    {
      if (ret)
        {
          *ret = dummy;
        }
      return true;
    }
  else
    return false;
}
