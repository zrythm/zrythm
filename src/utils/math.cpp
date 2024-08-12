// SPDX-FileCopyrightText: © 2018-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cfloat>
#include <cmath>

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
  char * val = g_strdup_printf ("%f", (double) x);
  if (std::isnan (x) || string_contains_substr (val, "nan"))
    {
      g_critical ("nan");
      g_free (val);
      return false;
    }
  if (!std::isfinite (x) || string_contains_substr (val, "inf"))
    {
      g_critical ("inf");
      g_free (val);
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
math_is_string_valid_float (const char * str, float * ret)
{
  int   len;
  float dummy = 0.0;
  if (sscanf (str, "%f %n", &dummy, &len) == 1 && len == (int) strlen (str))
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
