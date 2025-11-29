// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>

#include "dsp/file_audio_source.h"
#include "utils/audio.h"
#include "utils/io.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class FileAudioSourceTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create temporary test file using thread-safe temporary directory
    temp_dir_obj = zrythm::utils::io::make_tmp_dir ();
    temp_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();
    test_wav = temp_dir / "test.wav";

    // Create minimal WAV file
    juce::WavAudioFormat                format;
    juce::File                          file (test_wav.string ());
    std::unique_ptr<juce::OutputStream> out_stream =
      std::make_unique<juce::FileOutputStream> (file);
    if (dynamic_cast<juce::FileOutputStream &> (*out_stream).openedOk ())
      {
        out_stream->setPosition (0);
        dynamic_cast<juce::FileOutputStream &> (*out_stream).truncate ();
        juce::AudioFormatWriterOptions options;
        options =
          options.withSampleRate (44100)
            .withNumChannels (2)
            .withBitsPerSample (16)
            .withQualityOptionIndex (0);
        auto writer = format.createWriterFor (out_stream, options);
        if (writer)
          {
            juce::AudioBuffer<float> buffer (2, 44100);
            buffer.clear ();
            writer->writeFromAudioSampleBuffer (
              buffer, 0, buffer.getNumSamples ());
          }
      }

    project_sample_rate = units::sample_rate (44100);
    current_bpm = 120.0;
  }

  void TearDown () override
  {
    // QTemporaryDir auto-removes when destroyed, no manual cleanup needed
  }

  std::unique_ptr<QTemporaryDir> temp_dir_obj;
  std::filesystem::path          temp_dir;
  std::filesystem::path          test_wav;
  units::sample_rate_t           project_sample_rate;
  bpm_t                          current_bpm;
};

// Test constructors
TEST_F (FileAudioSourceTest, CreateFromFile)
{
  ASSERT_NO_THROW ({
    FileAudioSource src (test_wav, project_sample_rate, current_bpm, nullptr);
    EXPECT_EQ (src.get_num_channels (), 2);
    EXPECT_EQ (src.get_num_frames (), 44100);
    EXPECT_EQ (src.get_bit_depth (), FileAudioSource::BitDepth::BIT_DEPTH_16);
    EXPECT_EQ (src.get_samplerate (), project_sample_rate);
  });
}

TEST_F (FileAudioSourceTest, CreateFromBuffer)
{
  utils::audio::AudioBuffer buf (2, 100);
  buf.clear ();

  FileAudioSource src (
    buf, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"buffer_test", nullptr);

  EXPECT_EQ (src.get_num_channels (), 2);
  EXPECT_EQ (src.get_num_frames (), 100);
  EXPECT_EQ (src.get_bit_depth (), FileAudioSource::BitDepth::BIT_DEPTH_32);
  EXPECT_EQ (src.get_name (), "buffer_test");
}

TEST_F (FileAudioSourceTest, CreateFromInterleaved)
{
  const int channels = 2;
  const int nframes = 10;
  float     arr[20] = { 0 }; // 2ch * 10frames

  FileAudioSource src (
    arr, nframes, channels, FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"interleaved_test", nullptr);

  EXPECT_EQ (src.get_num_channels (), channels);
  EXPECT_EQ (src.get_num_frames (), nframes);
}

TEST_F (FileAudioSourceTest, CreateRecording)
{
  FileAudioSource src (
    2, 100, project_sample_rate, current_bpm, u8"recording", nullptr);
  EXPECT_EQ (src.get_num_channels (), 2);
  EXPECT_EQ (src.get_num_frames (), 100);
}

// Test core functionality
TEST_F (FileAudioSourceTest, ExpandWithFrames)
{
  utils::audio::AudioBuffer initial (2, 100);
  initial.clear ();
  FileAudioSource src (
    initial, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"expand_test", nullptr);

  utils::audio::AudioBuffer additional (2, 50);
  additional.clear ();
  src.expand_with_frames (additional);

  EXPECT_EQ (src.get_num_frames (), 150);
}

TEST_F (FileAudioSourceTest, ReplaceFrames)
{
  utils::audio::AudioBuffer initial (2, 100);
  initial.clear ();
  FileAudioSource src (
    initial, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"replace_test", nullptr);

  utils::audio::AudioBuffer replacement (2, 50);
  replacement.clear ();
  src.replace_frames (replacement, 25);

  // Verify frame count remains the same
  EXPECT_EQ (src.get_num_frames (), 100);
}

TEST_F (FileAudioSourceTest, ClearFrames)
{
  utils::audio::AudioBuffer buf (2, 100);
  FileAudioSource           src (
    buf, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"clear_test", nullptr);

  src.clear_frames ();
  EXPECT_EQ (src.get_num_frames (), 0);
}

// Test property accessors
TEST_F (FileAudioSourceTest, PropertyAccess)
{
  FileAudioSource src (test_wav, project_sample_rate, current_bpm, nullptr);

  src.set_name (u8"new_name");
  EXPECT_EQ (src.get_name (), "new_name");
}

// Test edge cases
TEST_F (FileAudioSourceTest, InvalidFile)
{
  ASSERT_THROW (
    {
      FileAudioSource src (
        u8"nonexistent.wav", project_sample_rate, current_bpm, nullptr);
    },
    ZrythmException);
}

TEST_F (FileAudioSourceTest, EmptyBuffer)
{
  utils::audio::AudioBuffer emptyBuf (0, 0);
  ASSERT_THROW (
    {
      FileAudioSource src (
        emptyBuf, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
        current_bpm, u8"empty", nullptr);
    },
    std::exception);
}

// Test serialization/deserialization
TEST_F (FileAudioSourceTest, Serialization)
{
  FileAudioSource src (test_wav, project_sample_rate, current_bpm, nullptr);
  src.set_name (u8"serial_test");

  // Serialize to JSON
  nlohmann::json j;
  j = src;

  // Deserialize
  FileAudioSource deserialized (nullptr);
  j.get_to (deserialized);

  // Verify properties
  EXPECT_EQ (deserialized.get_name ().str (), "serial_test");
  EXPECT_EQ (
    deserialized.get_bit_depth (), FileAudioSource::BitDepth::BIT_DEPTH_16);
  EXPECT_EQ (deserialized.get_samplerate (), project_sample_rate);
}

} // namespace zrythm::dsp
