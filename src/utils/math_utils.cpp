// SPDX-FileCopyrightText: © 2018-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cfloat>
#include <cmath>

#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/utf8_string.h"

namespace zrythm::utils::math
{

float
calculate_rms_amp (const float * buf, const uint32_t nframes)
{
  float sum = 0;
  float sample = 0;
  for (unsigned int i = 0; i < nframes; i += RMS_FRAMES)
    {
      sample = buf[i];
      sum += (sample * sample);
    }
  return std::sqrtf (sum / ((float) nframes / (float) RMS_FRAMES));
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
    || utils::Utf8String::from_utf8_encoded_string (val).contains_substr (
      u8"nan"))
    {
      z_error ("nan");
      return false;
    }
  if (
    !std::isfinite (x)
    || utils::Utf8String::from_utf8_encoded_string (val).contains_substr (
      u8"inf"))
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
      if (ret != nullptr)
        {
          *ret = dummy;
        }
      return true;
    }
  else
    return false;
}

} // namespace zrythm::utils::math
