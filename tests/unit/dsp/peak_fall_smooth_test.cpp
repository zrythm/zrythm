// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/peak_fall_smooth.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class PeakFallSmoothTest : public ::testing::Test
{
protected:
  static constexpr float EPSILON = 0.0001f;
  static constexpr float SAMPLE_RATE = 44100.0f;

  void SetUp () override { smoother_.calculate_coeff (10.0f, SAMPLE_RATE); }

  PeakFallSmooth smoother_;
};

TEST_F (PeakFallSmoothTest, InitialState)
{
  PeakFallSmooth smoother;
  smoother.calculate_coeff (10.0f, SAMPLE_RATE);
  EXPECT_NEAR (smoother.get_smoothed_value (), 0.0f, EPSILON);
}

TEST_F (PeakFallSmoothTest, SingleValueSmoothing)
{
  smoother_.set_value (1.0f);
  float smoothed = smoother_.get_smoothed_value ();
  EXPECT_GT (smoothed, 0.0f);
  EXPECT_LE (smoothed, 1.0f);
}

TEST_F (PeakFallSmoothTest, PeakHoldBehavior)
{
  // Set initial peak
  smoother_.set_value (1.0f);
  float first = smoother_.get_smoothed_value ();

  // Set lower value
  smoother_.set_value (0.5f);
  float second = smoother_.get_smoothed_value ();

  // Value should fall smoothly
  EXPECT_GT (second, 0.5f);
  EXPECT_LT (second, first);
}

TEST_F (PeakFallSmoothTest, FrequencyResponse)
{
  PeakFallSmooth fast_smoother;
  PeakFallSmooth slow_smoother;

  fast_smoother.calculate_coeff (100.0f, SAMPLE_RATE);
  slow_smoother.calculate_coeff (1.0f, SAMPLE_RATE);

  // Set high value then low value
  fast_smoother.set_value (1.0f);
  slow_smoother.set_value (1.0f);

  fast_smoother.set_value (0.0f);
  slow_smoother.set_value (0.0f);

  float fast_val = fast_smoother.get_smoothed_value ();
  float slow_val = slow_smoother.get_smoothed_value ();

  // Fast smoother should fall more quickly
  EXPECT_LT (fast_val, slow_val);
}

TEST_F (PeakFallSmoothTest, MultipleUpdates)
{
  std::vector<float> values = { 0.0f, 1.0f, 0.5f, 0.8f, 0.2f };
  std::vector<float> smoothed;

  for (float val : values)
    {
      smoother_.set_value (val);
      smoothed.push_back (smoother_.get_smoothed_value ());
    }

  // Verify smoothed values are monotonic when falling
  for (size_t i = 1; i < smoothed.size (); ++i)
    {
      if (values[i] < values[i - 1])
        {
          EXPECT_GT (smoothed[i], values[i]);
        }
    }
}

TEST_F (PeakFallSmoothTest, DifferentSampleRates)
{
  PeakFallSmooth smoother1, smoother2;
  smoother1.calculate_coeff (10.0f, 44100.0f);
  smoother2.calculate_coeff (10.0f, 96000.0f);

  // Set high value then low value
  smoother1.set_value (1.0f);
  smoother2.set_value (1.0f);

  smoother1.set_value (0.0f);
  smoother2.set_value (0.0f);

  float val1 = smoother1.get_smoothed_value ();
  float val2 = smoother2.get_smoothed_value ();

  EXPECT_NE (val1, val2);
}

} // namespace zrythm::dsp
