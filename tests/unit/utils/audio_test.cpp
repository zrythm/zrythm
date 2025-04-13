// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::utils::audio;

TEST (AudioTest, BitDepthConversion)
{
  EXPECT_EQ (bit_depth_enum_to_int (BitDepth::BIT_DEPTH_16), 16);
  EXPECT_EQ (bit_depth_enum_to_int (BitDepth::BIT_DEPTH_24), 24);
  EXPECT_EQ (bit_depth_enum_to_int (BitDepth::BIT_DEPTH_32), 32);

  EXPECT_EQ (bit_depth_int_to_enum (16), BitDepth::BIT_DEPTH_16);
  EXPECT_EQ (bit_depth_int_to_enum (24), BitDepth::BIT_DEPTH_24);
  EXPECT_EQ (bit_depth_int_to_enum (32), BitDepth::BIT_DEPTH_32);
}

TEST (AudioTest, GetNumFrames)
{
  auto num_frames = get_num_frames (TEST_WAV_FILE_PATH);
  EXPECT_GT (num_frames, 0);
}

TEST (AudioTest, FrameComparison)
{
  float buf1[1024];
  float buf2[1024];

  // Test equal buffers
  std::fill_n (buf1, 1024, 0.5f);
  std::fill_n (buf2, 1024, 0.5f);
  EXPECT_TRUE (frames_equal (buf1, buf2, 1024, 0.0001f));

  // Test different buffers
  buf2[500] = 0.6f;
  EXPECT_FALSE (frames_equal (buf1, buf2, 1024, 0.0001f));
}

TEST (AudioTest, AudioFileComparison)
{
  EXPECT_TRUE (
    audio_files_equal (TEST_WAV_FILE_PATH, TEST_WAV_FILE_PATH, 0, 0.0001f));
}

TEST (AudioTest, FramesEmpty)
{
  float buf[1024] = { 0.f };
  EXPECT_TRUE (frames_empty (buf, 1024));

  buf[500] = 0.1f;
  EXPECT_FALSE (frames_empty (buf, 1024));
}

// TODO
#if 0
TEST (AudioTest, BpmDetection)
{
  std::vector<float> candidates;
  float              bpm = detect_bpm (nullptr, 0, 44100, candidates);
  EXPECT_GE (bpm, 0.f);
}
#endif

TEST (AudioTest, AudioFileSilence)
{
  EXPECT_FALSE (audio_file_is_silent (TEST_WAV_FILE_PATH));
}

TEST (AudioTest, NumCores)
{
  int cores = get_num_cores ();
  EXPECT_GT (cores, 0);
}

TEST (AudioBufferTest, FromInterleaved)
{
  // Test valid interleaved data conversion
  const float interleaved[] = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f };
  auto        buffer = AudioBuffer::from_interleaved (interleaved, 3, 2);
  EXPECT_EQ (buffer->getNumChannels (), 2);
  EXPECT_EQ (buffer->getNumSamples (), 3);
  EXPECT_FLOAT_EQ (buffer->getSample (0, 0), 0.1f);
  EXPECT_FLOAT_EQ (buffer->getSample (1, 0), 0.2f);
  EXPECT_FLOAT_EQ (buffer->getSample (0, 1), 0.3f);

  // Test invalid arguments
  EXPECT_THROW (
    AudioBuffer::from_interleaved (interleaved, 0, 2), std::invalid_argument);
  EXPECT_THROW (
    AudioBuffer::from_interleaved (interleaved, 3, 0), std::invalid_argument);
}

TEST (AudioBufferTest, InvertPhase)
{
  AudioBuffer buffer (2, 4);
  buffer.setSample (0, 0, 0.5f);
  buffer.setSample (0, 1, -0.3f);
  buffer.setSample (1, 0, 0.7f);
  buffer.setSample (1, 1, -0.2f);

  buffer.invert_phase ();

  EXPECT_FLOAT_EQ (buffer.getSample (0, 0), -0.5f);
  EXPECT_FLOAT_EQ (buffer.getSample (0, 1), 0.3f);
  EXPECT_FLOAT_EQ (buffer.getSample (1, 0), -0.7f);
  EXPECT_FLOAT_EQ (buffer.getSample (1, 1), 0.2f);
}

TEST (AudioBufferTest, NormalizePeak)
{
  AudioBuffer buffer (2, 2);
  // Channel 0: peak 0.5
  buffer.setSample (0, 0, 0.5f);
  buffer.setSample (0, 1, -0.3f);
  // Channel 1: peak 0.25
  buffer.setSample (1, 0, 0.25f);
  buffer.setSample (1, 1, -0.2f);

  buffer.normalize_peak ();

  EXPECT_FLOAT_EQ (buffer.getSample (0, 0), 1.0f);
  EXPECT_FLOAT_EQ (buffer.getSample (0, 1), -0.6f);
  EXPECT_FLOAT_EQ (buffer.getSample (1, 0), 1.0f);
  EXPECT_FLOAT_EQ (buffer.getSample (1, 1), -0.8f);
}

TEST (AudioBufferTest, MultiChannelInterleaving)
{
  AudioBuffer buffer (3, 2); // 3 channels, 2 frames each
  for (int ch = 0; ch < 3; ch++)
    {
      for (int i = 0; i < 2; i++)
        {
          buffer.setSample (ch, i, static_cast<float> (ch * 2 + i));
        }
    }

  buffer.interleave_samples ();
  EXPECT_EQ (buffer.getNumChannels (), 1);
  EXPECT_EQ (buffer.getNumSamples (), 6);

  buffer.deinterleave_samples (3);
  EXPECT_EQ (buffer.getNumChannels (), 3);
  EXPECT_EQ (buffer.getNumSamples (), 2);

  // Verify original values preserved
  for (int ch = 0; ch < 3; ch++)
    {
      for (int i = 0; i < 2; i++)
        {
          EXPECT_FLOAT_EQ (
            buffer.getSample (ch, i), static_cast<float> (ch * 2 + i));
        }
    }
}
