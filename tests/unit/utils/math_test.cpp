// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/math_utils.h"

using namespace zrythm::utils::math;

TEST (MathTest, FloatComparison)
{
  EXPECT_TRUE (floats_equal (1.0f, 1.0f));
  EXPECT_TRUE (floats_near (1.0f, 1.0000001f, 0.0001f));
  EXPECT_FALSE (floats_equal (1.0f, 1.1f));
}

TEST (MathTest, DoubleComparison)
{
  EXPECT_TRUE (floats_equal (1.0, 1.0));
  EXPECT_TRUE (floats_near (1.0, 1.0000001, 0.0001));
  EXPECT_FALSE (floats_equal (1.0, 1.1));
}

TEST (MathTest, Rounding)
{
  EXPECT_EQ (round_to_signed_32 (3.7), 4);
  EXPECT_EQ (round_to_signed_32 (-3.7), -4);
  EXPECT_EQ (round_to_signed_32 (3.7f), 4);
  EXPECT_EQ (round_to_signed_32 (-3.7f), -4);
}

TEST (MathTest, FastLog)
{
  float val = 2.0f;
  EXPECT_NEAR (fast_log (val), std::log (val), 0.01f);
  EXPECT_NEAR (fast_log2 (val), std::log2 (val), 0.01f);
  EXPECT_NEAR (fast_log10 (val), std::log10 (val), 0.01f);
}

TEST (MathTest, FaderAmpConversion)
{
  // Test fader to amp conversion
  float fader_val = 0.5f;
  float amp = get_amp_val_from_fader (fader_val);
  float back_to_fader = get_fader_val_from_amp (amp);
  EXPECT_NEAR (fader_val, back_to_fader, 0.001f);

  // Test edge cases
  EXPECT_NEAR (get_amp_val_from_fader (0.0f), 0.0f, 0.001f);
  EXPECT_NEAR (get_amp_val_from_fader (1.0f), 2.0f, 0.001f);
}

TEST (MathTest, DbConversion)
{
  // Test amp to db conversion
  float amp = 1.0f;
  float db = amp_to_dbfs (amp);
  EXPECT_NEAR (db, 0.0f, 0.001f); // 1.0 amplitude = 0 dBFS

  // Test db to amp conversion
  EXPECT_NEAR (dbfs_to_amp (0.0f), 1.0f, 0.001f);
  EXPECT_NEAR (dbfs_to_amp (-6.0f), 0.501187f, 0.001f);
}

TEST (MathTest, RmsCalculation)
{
  const nframes_t     nframes = 4;
  audio_sample_type_t buf[nframes] = { 0.5f, -0.5f, 0.5f, -0.5f };

  float rms_amp = calculate_rms_amp (buf, nframes);
  EXPECT_NEAR (rms_amp, 0.5f, 0.001f);

  float rms_db = calculate_rms_db (buf, nframes);
  EXPECT_NEAR (rms_db, amp_to_dbfs (rms_amp), 0.001f);
}

TEST (MathTest, StringValidation)
{
  float result;
  EXPECT_TRUE (is_string_valid_float ("1.234", &result));
  EXPECT_NEAR (result, 1.234f, 0.001f);

  EXPECT_FALSE (is_string_valid_float ("not a number", &result));
  EXPECT_FALSE (is_string_valid_float ("", &result));
}
