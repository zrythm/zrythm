// SPDX-FileCopyrightText: © 2018-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cfloat>
#include <cmath>

#include "utils/logger.h"
#include "utils/math.h"
#include "utils/string.h"

namespace zrythm::utils::math
{

sample_t
calculate_rms_amp (const sample_t * buf, const nframes_t nframes)
{
  sample_t sum = 0, sample = 0;
  for (unsigned int i = 0; i < nframes; i += RMS_FRAMES)
    {
      sample = buf[i];
      sum += (sample * sample);
    }
  return std::sqrtf (sum / ((sample_t) nframes / (sample_t) RMS_FRAMES));
}

/**
 * Asserts that the value is non-nan.
 *
 * Not real-time safe.
 *
 * @return Whether the value is valid (nonnan).
 */
bool
assert_nonnann (float x)
{
  auto val = fmt::format ("{:f}", (double) x);
  if (
    std::isnan (x)
    || zrythm::utils::string::contains_substr (val.c_str (), "nan"))
    {
      z_error ("nan");
      return false;
    }
  if (
    !std::isfinite (x)
    || zrythm::utils::string::contains_substr (val.c_str (), "inf"))
    {
      z_error ("inf");
      return false;
    }
  return true;
}

bool
is_string_valid_float (const std::string &str, float * ret)
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

} // namespace zrythm::utils::math
