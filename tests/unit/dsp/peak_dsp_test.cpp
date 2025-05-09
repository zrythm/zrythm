// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/peak_dsp.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class PeakDspTest : public ::testing::Test
{
protected:
  static constexpr float EPSILON = 0.0001f;
  static constexpr float SAMPLE_RATE = 44100.0f;

  void SetUp () override { meter_.init (SAMPLE_RATE); }

  PeakDsp meter_{};
};

TEST_F (PeakDspTest, InitializationState)
{
  PeakDsp meter{};
  meter.init (SAMPLE_RATE);
  auto [rms, peak] = meter.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (PeakDspTest, ProcessSilence)
{
  std::vector<float> silence (1024, 0.0f);
  meter_.process (silence.data (), silence.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (PeakDspTest, ProcessConstantSignal)
{
  std::vector<float> signal (1024, 0.5f);
  meter_.process (signal.data (), signal.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.5f, EPSILON);
  EXPECT_NEAR (peak, 0.5f, EPSILON);
}

TEST_F (PeakDspTest, ProcessAlternatingSignal)
{
  std::vector<float> signal (1024);
  for (size_t i = 0; i < signal.size (); ++i)
    {
      signal[i] = (i % 2 == 0) ? 0.5f : -0.5f;
    }
  meter_.process (signal.data (), signal.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.5f, EPSILON);
  EXPECT_NEAR (peak, 0.5f, EPSILON);
}

TEST_F (PeakDspTest, Reset)
{
  std::vector<float> signal (1024, 1.0f);
  meter_.process (signal.data (), signal.size ());
  meter_.reset ();
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (PeakDspTest, PeakHoldBehavior)
{
  // Create a signal that peaks at 0.8
  std::vector<float> signal (1024, 0.5f);
  signal[512] = 0.8f;

  meter_.process (signal.data (), signal.size ());
  auto [rms1, peak1] = meter_.read ();
  EXPECT_NEAR (peak1, 0.8f, EPSILON);

  // Process lower amplitude signal and verify peak holds
  std::vector<float> lower_signal (1024, 0.3f);
  meter_.process (lower_signal.data (), lower_signal.size ());
  auto [rms2, peak2] = meter_.read ();

  EXPECT_GT (peak2, 0.3f);
  EXPECT_LE (peak2, peak1);
}

TEST_F (PeakDspTest, AbsoluteValueDetection)
{
  // Test with positive and negative values
  std::vector<float> signal (1024, 0.0f);
  signal[100] = 0.7f;
  signal[200] = -0.7f;
  signal[300] = 0.3f;
  signal[400] = -0.3f;

  meter_.process (signal.data (), signal.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (peak, 0.7f, EPSILON);
}

TEST_F (PeakDspTest, EdgeCases)
{
  std::vector<float> signal (1024, 0.0f);
  // Mix invalid values with valid ones
  signal[0] = std::numeric_limits<float>::infinity ();
  signal[1] = -std::numeric_limits<float>::infinity ();
  signal[2] = std::numeric_limits<float>::quiet_NaN ();
  signal[3] = 0.5f; // Valid value
  signal[4] = 0.7f; // Valid value

  // Process multiple times to allow the meter to stabilize
  for (const auto _ : std::views::iota (0, 3))
    {
      meter_.process (signal.data (), signal.size ());
    }

  auto [rms, peak] = meter_.read ();

  // Verify the results are valid and bounded
  EXPECT_TRUE (std::isfinite (peak));
  EXPECT_GE (peak, 0.0f);
  EXPECT_LE (peak, 1.0f);
}

} // namespace zrythm::dsp
