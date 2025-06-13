// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/stretcher.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

class StretcherTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    sample_rate_ = 44100;
    channels_ = 2;
    time_ratio_ = 1.0;
    pitch_ratio_ = 1.0;
  }

  sample_rate_t sample_rate_{};
  unsigned int  channels_{};
  double        time_ratio_{};
  double        pitch_ratio_{};
};

TEST_F (StretcherTest, Creation)
{
  // Test creation with valid parameters
  auto stretcher = Stretcher::create_rubberband (
    sample_rate_, channels_, time_ratio_, pitch_ratio_, false);
  EXPECT_NE (stretcher, nullptr);

  // Test creation with realtime mode
  auto rt_stretcher = Stretcher::create_rubberband (
    sample_rate_, channels_, time_ratio_, pitch_ratio_, true);
  EXPECT_NE (rt_stretcher, nullptr);

  // Test invalid parameters
  EXPECT_ANY_THROW (
    Stretcher::create_rubberband (
      0, channels_, time_ratio_, pitch_ratio_, false));
}

TEST_F (StretcherTest, BasicStretching)
{
  auto stretcher = Stretcher::create_rubberband (
    sample_rate_, channels_, time_ratio_, pitch_ratio_, false);
  ASSERT_NE (stretcher, nullptr);

  // Create test signal (1 second of 440Hz sine wave)
  const size_t       num_samples = sample_rate_;
  std::vector<float> in_samples_l (num_samples);
  std::vector<float> in_samples_r (num_samples);

  for (size_t i = 0; i < num_samples; i++)
    {
      float t = static_cast<float> (i) / static_cast<float> (sample_rate_);
      in_samples_l[i] = std::sin (2.0f * std::numbers::pi_v<float> * 440.0f * t);
      in_samples_r[i] = in_samples_l[i];
    }

  // Output buffers
  std::vector<float> out_samples_l (num_samples);
  std::vector<float> out_samples_r (num_samples);

  auto frames_processed = stretcher->stretch (
    in_samples_l.data (), in_samples_r.data (), num_samples,
    out_samples_l.data (), out_samples_r.data (), num_samples);

  EXPECT_GT (frames_processed, 0);
}

TEST_F (StretcherTest, TimeRatioChange)
{
  // Test with realtime mode where we expect latency
  auto rt_stretcher = Stretcher::create_rubberband (
    sample_rate_, channels_, 1.0, pitch_ratio_, true);
  ASSERT_NE (rt_stretcher, nullptr);
  EXPECT_GT (rt_stretcher->get_latency (), 0u);

  // Test with offline mode where latency may be 0
  auto offline_stretcher = Stretcher::create_rubberband (
    sample_rate_, channels_, 1.0, pitch_ratio_, false);
  ASSERT_NE (offline_stretcher, nullptr);

  // Test changing time ratio for both
  double new_ratio = 0.5;
  rt_stretcher->set_time_ratio (new_ratio);
  offline_stretcher->set_time_ratio (new_ratio);
}

TEST_F (StretcherTest, MonoProcessing)
{
  // Test with mono audio (1 channel)
  auto mono_stretcher = Stretcher::create_rubberband (
    sample_rate_, 1, time_ratio_, pitch_ratio_, false);
  ASSERT_NE (mono_stretcher, nullptr);

  const size_t       num_samples = 1000;
  std::vector<float> in_samples (num_samples, 0.5f);
  std::vector<float> out_samples (num_samples);

  auto frames_processed = mono_stretcher->stretch (
    in_samples.data (), nullptr, num_samples, out_samples.data (), nullptr,
    num_samples);

  EXPECT_GT (frames_processed, 0);
}

TEST_F (StretcherTest, RealtimeProcessing)
{
  auto rt_stretcher = Stretcher::create_rubberband (
    sample_rate_, channels_, time_ratio_, pitch_ratio_, true);
  ASSERT_NE (rt_stretcher, nullptr);

  const size_t       block_size = 512;
  std::vector<float> in_l (block_size);
  std::vector<float> in_r (block_size);
  std::vector<float> out_l (block_size);
  std::vector<float> out_r (block_size);

  // Process a small block in realtime mode
  auto frames_processed = rt_stretcher->stretch (
    in_l.data (), in_r.data (), block_size, out_l.data (), out_r.data (),
    block_size);

  EXPECT_GT (frames_processed, 0);
}
