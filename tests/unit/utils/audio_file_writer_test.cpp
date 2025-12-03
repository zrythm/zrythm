// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>

#include "utils/audio_file_writer.h"
#include "utils/io.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
using namespace zrythm::units;

namespace zrythm::utils
{

class AudioFileWriterTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create temporary directory for test files
    temp_dir_ = utils::io::make_tmp_dir ();
    temp_dir_path_ =
      utils::Utf8String::from_qstring (temp_dir_->path ()).to_path ();
  }

  void TearDown () override { }

  // Creates a test buffer with sine wave samples
  static juce::AudioSampleBuffer create_test_buffer (
    int   num_channels,
    int   num_samples,
    float frequency = 440.0f,
    float amplitude = 0.1f,
    float sample_rate = 48000.0f)
  {
    juce::AudioSampleBuffer buffer (num_channels, num_samples);

    for (int ch = 0; ch < num_channels; ++ch)
      {
        const auto channel_multiplier = static_cast<float> (ch + 1);

        for (int i = 0; i < num_samples; ++i)
          {
            const auto sample =
              amplitude * channel_multiplier
              * std::sin (
                2.0f * std::numbers::pi_v<float>
                * frequency * static_cast<float> (i) / sample_rate);

            buffer.setSample (ch, i, sample);
          }
      }

    return buffer;
  }

  // Verifies that a file exists and has expected content
  void verify_file_exists_and_size (
    const fs::path &file_path,
    size_t          expected_min_size)
  {
    EXPECT_TRUE (std::filesystem::exists (file_path))
      << "Output file should exist: " << file_path;

    if (expected_min_size > 0)
      {
        const auto file_size = std::filesystem::file_size (file_path);
        EXPECT_GE (file_size, expected_min_size)
          << "Output file should be at least " << expected_min_size
          << " bytes, got " << file_size;
      }
  }

  // Creates test writer options
  static AudioFileWriter::WriteOptions create_test_options (
    int sample_rate = 48000,
    int bit_depth = 16,
    int num_channels = 2)
  {
    juce::AudioFormatWriterOptions options;
    options =
      options.withSampleRate (sample_rate)
        .withNumChannels (num_channels)
        .withBitsPerSample (bit_depth);

    return AudioFileWriter::WriteOptions{
      .writer_options_ = options, .block_length_ = units::samples (512)
    };
  }

  std::unique_ptr<QTemporaryDir> temp_dir_;
  fs::path                       temp_dir_path_;
};

TEST_F (AudioFileWriterTest, WriteWavFile)
{
  const auto file_path = temp_dir_path_ / "test_output.wav";
  auto       buffer = create_test_buffer (2, 1024);
  const auto options = create_test_options ();

  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer));
  future.waitForFinished ();

  EXPECT_TRUE (future.isValid ());
  EXPECT_FALSE (future.isCanceled ());

  verify_file_exists_and_size (file_path, 1000); // At least 1KB
}

TEST_F (AudioFileWriterTest, WriteFlacFile)
{
  const auto file_path = temp_dir_path_ / "test_output.flac";
  auto       buffer = create_test_buffer (2, 2048);
  const auto options = create_test_options ();

  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer));
  future.waitForFinished ();

  EXPECT_TRUE (future.isValid ());
  EXPECT_FALSE (future.isCanceled ());

  verify_file_exists_and_size (file_path, 1500); // At least 1.5KB
}

TEST_F (AudioFileWriterTest, WriteOggFile)
{
  const auto file_path = temp_dir_path_ / "test_output.ogg";
  auto       buffer = create_test_buffer (2, 1536);
  const auto options = create_test_options ();

  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer));
  future.waitForFinished ();

  EXPECT_TRUE (future.isValid ());
  EXPECT_FALSE (future.isCanceled ());

  verify_file_exists_and_size (file_path, 1500); // At least 1.5KB
}

TEST_F (AudioFileWriterTest, WriteAiffFile)
{
  const auto file_path = temp_dir_path_ / "test_output.aiff";
  auto       buffer = create_test_buffer (2, 512);
  const auto options = create_test_options ();

  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer));
  future.waitForFinished ();

  EXPECT_TRUE (future.isValid ());
  EXPECT_FALSE (future.isCanceled ());

  verify_file_exists_and_size (file_path, 500); // At least 500 bytes
}

TEST_F (AudioFileWriterTest, WriteWithDifferentBitDepths)
{
  std::vector<int> bit_depths = { 8, 16, 24, 32 };

  for (auto bit_depth : bit_depths)
    {
      const auto file_path =
        temp_dir_path_ / ("test_depth_" + std::to_string (bit_depth) + ".wav");
      auto       buffer = create_test_buffer (2, 256);
      const auto options = create_test_options (48000, bit_depth);

      auto future =
        AudioFileWriter::write_async (options, file_path, std::move (buffer));
      future.waitForFinished ();

      EXPECT_TRUE (future.isValid ());
      EXPECT_FALSE (future.isCanceled ());

      verify_file_exists_and_size (file_path, 200); // At least some data
    }
}

TEST_F (AudioFileWriterTest, WriteWithDifferentChannels)
{
  std::vector<int> channel_counts = { 1, 2, 4, 8 };

  for (auto num_channels : channel_counts)
    {
      const auto file_path =
        temp_dir_path_
        / ("test_channels_" + std::to_string (num_channels) + ".wav");
      auto       buffer = create_test_buffer (num_channels, 256);
      const auto options = create_test_options (48000, 16, num_channels);

      auto future =
        AudioFileWriter::write_async (options, file_path, std::move (buffer));
      future.waitForFinished ();

      EXPECT_TRUE (future.isValid ());
      EXPECT_FALSE (future.isCanceled ());

      verify_file_exists_and_size (
        file_path, 100 * num_channels); // Rough estimate
    }
}

TEST_F (AudioFileWriterTest, WriteWithProgressReporting)
{
  const auto file_path = temp_dir_path_ / "test_progress.wav";
  auto       buffer = create_test_buffer (2, 2048); // 4 blocks of 512
  const auto options = create_test_options ();

  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer));

  // Initially no progress
  EXPECT_EQ (future.progressValue (), 0);

  // Wait for completion
  future.waitForFinished ();

  EXPECT_TRUE (future.isValid ());
  EXPECT_FALSE (future.isCanceled ());
  EXPECT_EQ (future.progressValue (), 2048); // Should reach full progress

  verify_file_exists_and_size (file_path, 2000);
}

TEST_F (AudioFileWriterTest, WriteWithCancellation)
{
  const auto file_path = temp_dir_path_ / "test_cancel.wav";
  auto       buffer = create_test_buffer (2, 4096); // Large buffer
  const auto options = create_test_options ();

  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer));

  // Cancel immediately
  future.cancel ();
  future.waitForFinished ();

  EXPECT_TRUE (future.isCanceled ());
  EXPECT_FALSE (future.isValid ());

  // File should not exist or be incomplete due to cancellation
  if (std::filesystem::exists (file_path))
    {
      const auto file_size = std::filesystem::file_size (file_path);
      // File might be partially written or empty
      EXPECT_LT (file_size, 4000) // Should be smaller than full size
        << "Cancelled file should be smaller than full size";
    }
}

TEST_F (AudioFileWriterTest, WriteWithDifferentBlockSizes)
{
  std::vector<int> block_sizes = { 64, 128, 256, 512, 1024, 4096 };

  for (auto block_size : block_sizes)
    {
      const auto file_path =
        temp_dir_path_ / ("test_block_" + std::to_string (block_size) + ".wav");
      auto buffer = create_test_buffer (2, 1024);
      auto options = create_test_options ();
      options.block_length_ = units::samples (block_size);

      auto future =
        AudioFileWriter::write_async (options, file_path, std::move (buffer));
      future.waitForFinished ();

      EXPECT_TRUE (future.isValid ());
      EXPECT_FALSE (future.isCanceled ());

      verify_file_exists_and_size (file_path, 1000);
    }
}

TEST_F (AudioFileWriterTest, WriteEmptyBuffer)
{
  const auto file_path = temp_dir_path_ / "test_empty.wav";
  auto       buffer = create_test_buffer (2, 0); // Empty buffer
  const auto options = create_test_options ();

  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer));
  future.waitForFinished ();

  EXPECT_TRUE (future.isValid ());
  EXPECT_FALSE (future.isCanceled ());

  verify_file_exists_and_size (file_path, 44); // WAV header should exist
}

TEST_F (AudioFileWriterTest, WriteUnsupportedFormat)
{
  const auto file_path = temp_dir_path_ / "test_unsupported.xyz";
  auto       buffer = create_test_buffer (2, 256);
  const auto options = create_test_options ();

  bool fail_callback_called{};
  auto future =
    AudioFileWriter::write_async (options, file_path, std::move (buffer))
      .onFailed ([&fail_callback_called] () { fail_callback_called = true; });
  future.waitForFinished ();

  EXPECT_TRUE (future.isFinished ());
  EXPECT_TRUE (fail_callback_called);

  // File should not exist due to unsupported format
  EXPECT_FALSE (std::filesystem::exists (file_path));
}

} // namespace zrythm::utils
