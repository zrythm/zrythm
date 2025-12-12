// SPDX-FileCopyrightText: © 2018-2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cfloat>
#include <cmath>

#include "utils/logger.h"
#include "utils/math_utils.h"
#include "utils/utf8_string.h"

namespace zrythm::utils::math
{

audio_sample_type_t
calculate_rms_amp (const audio_sample_type_t * buf, const nframes_t nframes)
{
  audio_sample_type_t sum = 0, sample = 0;
  for (unsigned int i = 0; i < nframes; i += RMS_FRAMES)
    {
      sample = buf[i];
      sum += (sample * sample);
    }
  return std::sqrtf (
    sum / ((audio_sample_type_t) nframes / (audio_sample_type_t) RMS_FRAMES));
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
