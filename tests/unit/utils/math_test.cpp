#include "utils/gtest_wrapper.h"
#include "utils/math.h"

TEST (MathTest, FloatComparison)
{
  EXPECT_TRUE (math_floats_equal (1.0f, 1.0f));
  EXPECT_TRUE (math_floats_equal_epsilon (1.0f, 1.0000001f, 0.0001f));
  EXPECT_FALSE (math_floats_equal (1.0f, 1.1f));
}

TEST (MathTest, DoubleComparison)
{
  EXPECT_TRUE (math_doubles_equal (1.0, 1.0));
  EXPECT_TRUE (math_doubles_equal_epsilon (1.0, 1.0000001, 0.0001));
  EXPECT_FALSE (math_doubles_equal (1.0, 1.1));
}

TEST (MathTest, Rounding)
{
  EXPECT_EQ (math_round_double_to_signed_32 (3.7), 4);
  EXPECT_EQ (math_round_double_to_signed_32 (-3.7), -4);
  EXPECT_EQ (math_round_float_to_signed_32 (3.7f), 4);
  EXPECT_EQ (math_round_float_to_signed_32 (-3.7f), -4);
}

TEST (MathTest, FastLog)
{
  float val = 2.0f;
  EXPECT_NEAR (math_fast_log (val), std::log (val), 0.01f);
  EXPECT_NEAR (math_fast_log2 (val), std::log2 (val), 0.01f);
  EXPECT_NEAR (math_fast_log10 (val), std::log10 (val), 0.01f);
}

TEST (MathTest, FaderAmpConversion)
{
  // Test fader to amp conversion
  float fader_val = 0.5f;
  float amp = math_get_amp_val_from_fader (fader_val);
  float back_to_fader = math_get_fader_val_from_amp (amp);
  EXPECT_NEAR (fader_val, back_to_fader, 0.001f);

  // Test edge cases
  EXPECT_NEAR (math_get_amp_val_from_fader (0.0f), 0.0f, 0.001f);
  EXPECT_NEAR (math_get_amp_val_from_fader (1.0f), 2.0f, 0.001f);
}

TEST (MathTest, DbConversion)
{
  // Test amp to db conversion
  float amp = 1.0f;
  float db = math_amp_to_dbfs (amp);
  EXPECT_NEAR (db, 0.0f, 0.001f); // 1.0 amplitude = 0 dBFS

  // Test db to amp conversion
  EXPECT_NEAR (math_dbfs_to_amp (0.0f), 1.0f, 0.001f);
  EXPECT_NEAR (math_dbfs_to_amp (-6.0f), 0.501187f, 0.001f);
}

TEST (MathTest, RmsCalculation)
{
  const nframes_t nframes = 4;
  sample_t        buf[nframes] = { 0.5f, -0.5f, 0.5f, -0.5f };

  float rms_amp = math_calculate_rms_amp (buf, nframes);
  EXPECT_NEAR (rms_amp, 0.5f, 0.001f);

  float rms_db = math_calculate_rms_db (buf, nframes);
  EXPECT_NEAR (rms_db, math_amp_to_dbfs (rms_amp), 0.001f);
}

TEST (MathTest, StringValidation)
{
  float result;
  EXPECT_TRUE (math_is_string_valid_float ("1.234", &result));
  EXPECT_NEAR (result, 1.234f, 0.001f);

  EXPECT_FALSE (math_is_string_valid_float ("not a number", &result));
  EXPECT_FALSE (math_is_string_valid_float ("", &result));
}
