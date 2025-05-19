// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <numbers>
#include <ranges>

#include "dsp/kmeter_dsp.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class KMeterDspTest : public ::testing::Test
{
protected:
  static constexpr float EPSILON = 0.0001f;
  static constexpr float SAMPLE_RATE = 44100.0f;

  void SetUp () override { meter_.init (SAMPLE_RATE); }

  KMeterDsp meter_;
};

TEST_F (KMeterDspTest, InitializationState)
{
  KMeterDsp meter{};
  meter.init (SAMPLE_RATE);
  auto [rms, peak] = meter.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (KMeterDspTest, ProcessSilence)
{
  std::vector<float> silence (1024, 0.0f);
  meter_.process (silence.data (), (int) silence.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (KMeterDspTest, ProcessConstantSignal)
{
  std::vector<float> signal (1024, 1.0f);
  meter_.process (signal.data (), (int) signal.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_GT (rms, 0.0f);
  EXPECT_GT (peak, 0.0f);
  EXPECT_LE (rms, 1.0f);
  EXPECT_LE (peak, 1.0f);
}

TEST_F (KMeterDspTest, Reset)
{
  std::vector<float> signal (1024, 1.0f);
  meter_.process (signal.data (), (int) signal.size ());
  meter_.reset ();
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (KMeterDspTest, PeakHoldBehavior)
{
  // Create a signal that ramps up then down
  std::vector<float> signal (1024);
  for (size_t i = 0; i < signal.size () / 2; ++i)
    {
      signal[i] =
        static_cast<float> (i) / static_cast<float> (signal.size () / 2);
    }
  for (size_t i = signal.size () / 2; i < signal.size (); ++i)
    {
      signal[i] =
        1.0f
        - static_cast<float> (i - signal.size () / 2)
            / static_cast<float> (signal.size () / 2);
    }

  meter_.process (signal.data (), (int) signal.size ());
  auto [rms1, peak1] = meter_.read ();

  // Process silence and verify peak holds
  std::vector<float> silence (1024, 0.0f);
  meter_.process (silence.data (), (int) silence.size ());
  auto [rms2, peak2] = meter_.read ();

  EXPECT_GT (peak2, 0.0f);
  EXPECT_LE (peak2, peak1);
}

TEST_F (KMeterDspTest, RMSCalculation)
{
  // Test with a sine wave
  std::vector<float> signal (1024);
  for (const auto i : std::views::iota (0_zu, signal.size ()))
    {
      signal[i] = std::sin (
        2.0f * std::numbers::pi_v<float> * static_cast<float> (i) / 32.0f);
    }

  // Process multiple blocks to allow the ballistic filter to settle
  for (int i = 0; i < 10; ++i)
    {
      meter_.process (signal.data (), static_cast<int> (signal.size ()));
    }

  float rms = meter_.read_f ();

  // The actual RMS will be lower than theoretical due to the ballistic filtering
  EXPECT_GT (rms, 0.1f);
  EXPECT_LT (rms, 1.0f);
}

TEST_F (KMeterDspTest, EdgeCases)
{
  // Test with invalid values
  std::vector<float> invalid_signal (1024);
  invalid_signal[0] = std::numeric_limits<float>::infinity ();
  invalid_signal[1] = -std::numeric_limits<float>::infinity ();
  invalid_signal[2] = std::numeric_limits<float>::quiet_NaN ();

  meter_.process (invalid_signal.data (), (int) invalid_signal.size ());
  auto [rms, peak] = meter_.read ();

  EXPECT_TRUE (std::isfinite (rms));
  EXPECT_TRUE (std::isfinite (peak));
  EXPECT_GE (rms, 0.0f);
  EXPECT_GE (peak, 0.0f);
}

} // namespace zrythm::dsp
