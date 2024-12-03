// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/true_peak_dsp.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class TruePeakDspTest : public ::testing::Test
{
protected:
  static constexpr float EPSILON = 0.0001f;
  static constexpr float SAMPLE_RATE = 44100.0f;

  void SetUp () override { meter_.init (SAMPLE_RATE); }

  TruePeakDsp meter_;
};

TEST_F (TruePeakDspTest, InitializationState)
{
  TruePeakDsp meter;
  meter.init (SAMPLE_RATE);
  auto [rms, peak] = meter.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (TruePeakDspTest, ProcessSilence)
{
  std::vector<float> silence (1024, 0.0f);
  meter_.process (silence.data (), silence.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (TruePeakDspTest, ProcessConstantSignal)
{
  std::vector<float> signal (1024, 0.5f);
  meter_.process (signal.data (), signal.size ());
  auto [rms, peak] = meter_.read ();
  EXPECT_GT (rms, 0.0f);
  EXPECT_GT (peak, 0.0f);
}

TEST_F (TruePeakDspTest, ProcessMaxValue)
{
  std::vector<float> signal (1024, 0.0f);
  signal[512] = 1.0f; // Single maximum value
  meter_.process_max (signal.data (), signal.size ());
  float max_val = meter_.read_f ();
  EXPECT_GT (max_val, 0.0f);
  EXPECT_LE (max_val, 1.0f);
}

TEST_F (TruePeakDspTest, Reset)
{
  std::vector<float> signal (1024, 0.5f);
  meter_.process (signal.data (), signal.size ());
  meter_.reset ();
  auto [rms, peak] = meter_.read ();
  EXPECT_NEAR (rms, 0.0f, EPSILON);
  EXPECT_NEAR (peak, 0.0f, EPSILON);
}

TEST_F (TruePeakDspTest, InterpolatedPeakDetection)
{
  // Create a signal that will cause intersample peaks
  std::vector<float> signal (1024, 0.0f);
  for (size_t i = 0; i < signal.size (); i++)
    {
      signal[i] = std::sin (2.0f * M_PI * static_cast<float> (i) / 4.0f);
    }

  meter_.process (signal.data (), signal.size ());
  auto [rms, peak] = meter_.read ();

  // Due to 4x oversampling, peak should detect intersample peaks
  EXPECT_GT (peak, 0.0f);
  EXPECT_LT (peak, 2.0f);
}

TEST_F (TruePeakDspTest, ConsecutiveReads)
{
  std::vector<float> signal (1024, 0.0f);
  for (size_t i = 0; i < signal.size (); i++)
    {
      signal[i] = 0.5f * std::sin (2.0f * M_PI * static_cast<float> (i) / 32.0f);
    }

  meter_.process (signal.data (), signal.size ());
  auto [rms1, peak1] = meter_.read ();

  // Process different signal before second read
  for (size_t i = 0; i < signal.size (); i++)
    {
      signal[i] = 0.7f * std::sin (2.0f * M_PI * static_cast<float> (i) / 16.0f);
    }

  meter_.process (signal.data (), signal.size ());
  auto [rms2, peak2] = meter_.read ();

  EXPECT_GT (peak2, peak1);
}

TEST_F (TruePeakDspTest, DifferentSampleRates)
{
  TruePeakDsp meter1, meter2;
  meter1.init (44100.0f);
  meter2.init (96000.0f);

  // Create a high-frequency signal that will be affected by the sample rate
  std::vector<float> signal (1024);
  for (size_t i = 0; i < signal.size (); i++)
    {
      signal[i] = 0.5f * std::sin (2.0f * M_PI * static_cast<float> (i) / 8.0f);
    }

  meter1.process (signal.data (), signal.size ());
  meter2.process (signal.data (), signal.size ());

  float peak1 = meter1.read_f ();
  float peak2 = meter2.read_f ();

  EXPECT_GT (peak1, 0.0f);
  EXPECT_GT (peak2, 0.0f);
}

} // namespace zrythm::dsp
