// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/audio_file.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::utils::audio;

TEST (AudioFileTest, ReadMetadata)
{
  AudioFile file (TEST_WAV_FILE_PATH, false);
  auto      metadata = file.read_metadata ();
  EXPECT_GT (metadata.channels, 0);
  EXPECT_GT (metadata.num_frames, 0);
  EXPECT_GT (metadata.samplerate, 0);
  EXPECT_GT (metadata.bit_depth, 0);
  EXPECT_GT (metadata.bit_rate, 0);
  EXPECT_GT (metadata.length, 0);
}

TEST (AudioFileTest, ReadSamplesInterleaved)
{
  AudioFile file (TEST_WAV_FILE_PATH, false);
  auto      metadata = file.read_metadata ();

  std::vector<float> samples (metadata.num_frames * metadata.channels);
  file.read_samples_interleaved (false, samples.data (), 0, metadata.num_frames);

  // Verify samples are non-zero
  bool has_nonzero = false;
  for (float sample : samples)
    {
      if (sample != 0.0f)
        {
          has_nonzero = true;
          break;
        }
    }
  EXPECT_TRUE (has_nonzero);
}

TEST (AudioFileTest, ReadInParts)
{
  AudioFile file (TEST_WAV_FILE_PATH, false);
  auto      metadata = file.read_metadata ();

  size_t             chunk_size = 1024;
  std::vector<float> samples (chunk_size * metadata.channels);

  file.read_samples_interleaved (true, samples.data (), 0, chunk_size);
  EXPECT_EQ (samples.size (), chunk_size * metadata.channels);
}

TEST (AudioFileTest, ReadFullWithResampling)
{
  AudioFile file (TEST_WAV_FILE_PATH, false);
  auto      metadata = file.read_metadata ();

  zrythm::utils::audio::AudioBuffer buffer;

  // Test without resampling
  file.read_full (buffer, std::nullopt);
  EXPECT_EQ (buffer.getNumChannels (), metadata.channels);
  EXPECT_EQ (buffer.getNumSamples (), metadata.num_frames);

  // Test with resampling
  size_t target_samplerate = metadata.samplerate * 2;
  file.read_full (buffer, target_samplerate);
  EXPECT_EQ (buffer.getNumChannels (), metadata.channels);
}
