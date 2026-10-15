// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <filesystem>
#include <ranges>

#include "dsp/audio_block_store.h"
#include "dsp/file_audio_source.h"
#include "dsp/panning.h"
#include "utils/audio.h"
#include "utils/exceptions.h"
#include "utils/io_utils.h"
#include "utils/serialization.h"
#include "utils/utf8_string.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

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

    test_mono_wav = temp_dir / "test_mono.wav";
    {
      juce::File                          mono_file (test_mono_wav.string ());
      std::unique_ptr<juce::OutputStream> mono_out =
        std::make_unique<juce::FileOutputStream> (mono_file);
      if (dynamic_cast<juce::FileOutputStream &> (*mono_out).openedOk ())
        {
          mono_out->setPosition (0);
          dynamic_cast<juce::FileOutputStream &> (*mono_out).truncate ();
          juce::AudioFormatWriterOptions options;
          options =
            options.withSampleRate (44100)
              .withNumChannels (1)
              .withBitsPerSample (16)
              .withQualityOptionIndex (0);
          auto writer = format.createWriterFor (mono_out, options);
          if (writer)
            {
              juce::AudioBuffer<float> buffer (1, 44100);
              float *                  ch = buffer.getWritePointer (0);
              for (int i = 0; i < 44100; ++i)
                {
                  ch[i] = 1.0f;
                }
              writer->writeFromAudioSampleBuffer (
                buffer, 0, buffer.getNumSamples ());
            }
        }
    }

    store = std::make_shared<AudioBlockStore> (temp_dir / "blocks");

    project_sample_rate = units::sample_rate (44100);
    current_bpm = units::bpm (120.0);
  }

  static utils::audio::AudioBuffer make_ramp_buffer (int num_frames, float base)
  {
    utils::audio::AudioBuffer buf (2, num_frames);
    for (int i = 0; i < num_frames; ++i)
      {
        buf.setSample (0, i, base + static_cast<float> (i));
        buf.setSample (1, i, -(base + static_cast<float> (i)));
      }
    return buf;
  }

  std::unique_ptr<QTemporaryDir>   temp_dir_obj;
  std::filesystem::path            temp_dir;
  std::filesystem::path            test_wav;
  std::filesystem::path            test_mono_wav;
  std::shared_ptr<AudioBlockStore> store;
  units::sample_rate_t             project_sample_rate;
  units::bpm_t                     current_bpm;
};

TEST_F (FileAudioSourceTest, CreateFromFile)
{
  ASSERT_NO_THROW ({
    FileAudioSource src (test_wav, project_sample_rate, nullptr);
    EXPECT_EQ (src.get_num_channels (), 2);
    EXPECT_EQ (src.get_num_frames (), units::samples (44100));
    EXPECT_EQ (src.get_bit_depth (), FileAudioSource::BitDepth::BIT_DEPTH_16);
    EXPECT_EQ (src.get_samplerate (), project_sample_rate);
    EXPECT_TRUE (src.has_pending_tail ());
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
  EXPECT_EQ (src.get_num_frames (), units::samples (100));
  EXPECT_EQ (src.get_bit_depth (), FileAudioSource::BitDepth::BIT_DEPTH_32);
  EXPECT_EQ (src.get_name (), "buffer_test");
}

TEST_F (FileAudioSourceTest, MonoBufferConvertedToStereo)
{
  utils::audio::AudioBuffer mono_buf (1, 100);
  float *                   left = mono_buf.getWritePointer (0);
  for (int i = 0; i < 100; ++i)
    {
      left[i] = 1.0f;
    }

  FileAudioSource src (
    mono_buf, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"mono_test", nullptr);
  src.set_store (store);
  src.flush_tail ();

  EXPECT_EQ (src.get_num_channels (), 2);
  EXPECT_EQ (src.get_num_frames (), units::samples (100));

  const auto [expected_left_gain, expected_right_gain] =
    calculate_panning (PanLaw::Minus3dB, PanAlgorithm::SquareRoot, 0.5f);
  utils::audio::AudioBuffer result (2, 100);
  src.read_span (units::samples (0), units::samples (100), result);
  for (int i = 0; i < 100; ++i)
    {
      EXPECT_NEAR (result.getReadPointer (0)[i], expected_left_gain, 0.0001f);
      EXPECT_NEAR (result.getReadPointer (1)[i], expected_right_gain, 0.0001f);
    }
}

TEST_F (FileAudioSourceTest, MonoFileConvertedToStereo)
{
  FileAudioSource src (test_mono_wav, project_sample_rate, nullptr);
  src.set_store (store);
  src.flush_tail ();

  EXPECT_EQ (src.get_num_channels (), 2);
  EXPECT_EQ (src.get_num_frames (), units::samples (44100));

  const auto [expected_left_gain, expected_right_gain] =
    calculate_panning (PanLaw::Minus3dB, PanAlgorithm::SquareRoot, 0.5f);
  utils::audio::AudioBuffer result (2, 1);
  src.read_span (units::samples (0), units::samples (1), result);
  EXPECT_NEAR (result.getReadPointer (0)[0], expected_left_gain, 0.0001f);
  EXPECT_NEAR (result.getReadPointer (1)[0], expected_right_gain, 0.0001f);
}

TEST_F (FileAudioSourceTest, AppendFramesFlushesFullBlocks)
{
  const auto block_frames = static_cast<int64_t> (
    AudioBlockStore::kTargetBlockPayloadBytes / (2 * sizeof (float)));

  FileAudioSource src (
    make_ramp_buffer (1000, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"append_test", nullptr);
  src.set_store (store);

  // Appending past the block boundary flushes exactly one block; the
  // remainder stays pending.
  src.append_frames (make_ramp_buffer (static_cast<int> (block_frames), 1000.f));
  EXPECT_EQ (src.get_spans ().size (), 1);
  EXPECT_EQ (
    src.get_spans ().front ().num_frames, units::samples (block_frames));
  EXPECT_EQ (src.get_num_frames (), units::samples (1000 + block_frames));
}

TEST_F (FileAudioSourceTest, FlushTailWritesPartialBlock)
{
  FileAudioSource src (
    make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"flush_test", nullptr);
  src.set_store (store);

  src.flush_tail ();

  EXPECT_FALSE (src.has_pending_tail ());
  ASSERT_EQ (src.get_spans ().size (), 1);
  EXPECT_EQ (src.get_spans ().front ().num_frames, units::samples (100));
  EXPECT_EQ (store->get_all_block_ids ().size (), 1);
}

TEST_F (FileAudioSourceTest, ReadSpanAcrossBlocks)
{
  FileAudioSource src (
    make_ramp_buffer (200000, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"read_test", nullptr);
  src.set_store (store);
  src.flush_tail ();
  ASSERT_GE (src.get_spans ().size (), 2);

  // A window overlapping both spans, read from disk through the store.
  utils::audio::AudioBuffer result (2, 1000);
  src.read_span (units::samples (130000), units::samples (1000), result);
  const auto expected = make_ramp_buffer (200000, 0.f);
  for (int ch = 0; ch < 2; ++ch)
    {
      for (int i = 0; i < 1000; ++i)
        {
          EXPECT_EQ (
            result.getSample (ch, i), expected.getSample (ch, 130000 + i))
            << "Mismatch at ch=" << ch << " sample=" << i;
        }
    }
}

TEST_F (FileAudioSourceTest, AppendFramesMultiplePacketsPreserveData)
{
  constexpr int   chunk_size = 100;
  FileAudioSource src (
    make_ramp_buffer (chunk_size, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"multi_append_test", nullptr);
  src.set_store (store);

  constexpr int num_appends = 20;
  for (int append = 0; append < num_appends; ++append)
    {
      src.append_frames (make_ramp_buffer (
        chunk_size, static_cast<float> ((append + 1) * chunk_size)));
    }

  const int expected_total = chunk_size + num_appends * chunk_size;
  EXPECT_EQ (src.get_num_frames (), units::samples (expected_total));

  utils::audio::AudioBuffer result (2, expected_total);
  src.read_span (units::samples (0), units::samples (expected_total), result);
  const auto expected = make_ramp_buffer (expected_total, 0.f);
  for (int ch = 0; ch < 2; ++ch)
    {
      for (int i = 0; i < expected_total; ++i)
        {
          EXPECT_FLOAT_EQ (result.getSample (ch, i), expected.getSample (ch, i))
            << "Mismatch at ch=" << ch << " sample=" << i;
        }
    }
}

// Flushing content larger than two blocks shifts the tail over itself by
// more than a block's worth of frames, so the shift must tolerate overlap
TEST_F (FileAudioSourceTest, FlushContentLargerThanTwoBlocksRoundTrips)
{
  const auto block_frames = static_cast<int64_t> (
    AudioBlockStore::kTargetBlockPayloadBytes / (2 * sizeof (float)));
  const auto num_frames = block_frames * 3 + block_frames / 2;

  FileAudioSource src (
    make_ramp_buffer (static_cast<int> (num_frames), 0.f),
    FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate, current_bpm,
    u8"large_flush_test", nullptr);
  src.set_store (store);

  src.flush_tail ();
  EXPECT_EQ (src.get_num_frames (), units::samples (num_frames));
  EXPECT_EQ (src.get_spans ().size (), 4);

  const auto expected = make_ramp_buffer (static_cast<int> (num_frames), 0.f);
  for (
    const auto offset :
    { int64_t{ 0 }, block_frames - 100, block_frames + 50, 2 * block_frames,
      num_frames - 100 })
    {
      utils::audio::AudioBuffer result (2, 100);
      src.read_span (units::samples (offset), units::samples (100), result);
      for (int ch = 0; ch < 2; ++ch)
        {
          for (int i = 0; i < 100; ++i)
            {
              EXPECT_EQ (
                result.getSample (ch, i),
                expected.getSample (ch, static_cast<int> (offset) + i))
                << "Mismatch at ch=" << ch << " offset=" << offset
                << " sample=" << i;
            }
        }
    }
}

TEST_F (FileAudioSourceTest, SetPropertyAccess)
{
  FileAudioSource src (test_wav, project_sample_rate, nullptr);

  src.set_name (u8"new_name");
  EXPECT_EQ (src.get_name (), "new_name");
}

TEST_F (FileAudioSourceTest, InvalidFile)
{
  ASSERT_THROW (
    { FileAudioSource src (u8"nonexistent.wav", project_sample_rate, nullptr); },
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

TEST_F (FileAudioSourceTest, SerializationRoundTripsManifest)
{
  FileAudioSource src (
    make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"serial_test", nullptr);
  src.set_store (store);
  src.flush_tail ();

  nlohmann::json j;
  j = src;

  FileAudioSource deserialized (nullptr);
  j.get_to (deserialized);

  EXPECT_EQ (deserialized.get_name ().str (), "serial_test");
  EXPECT_EQ (deserialized.get_num_channels (), src.get_num_channels ());
  EXPECT_EQ (deserialized.get_samplerate (), src.get_samplerate ());
  ASSERT_EQ (deserialized.get_spans ().size (), src.get_spans ().size ());
  EXPECT_EQ (
    deserialized.get_spans ().front ().block.hex,
    src.get_spans ().front ().block.hex);

  deserialized.set_store (store);
  utils::audio::AudioBuffer result (2, 100);
  deserialized.read_span (units::samples (0), units::samples (100), result);
  const auto expected = make_ramp_buffer (100, 0.f);
  for (int ch = 0; ch < 2; ++ch)
    {
      for (int i = 0; i < 100; ++i)
        {
          EXPECT_EQ (result.getSample (ch, i), expected.getSample (ch, i));
        }
    }
}

TEST_F (FileAudioSourceTest, SerializationRejectsNonContiguousSpans)
{
  nlohmann::json j;
  {
    FileAudioSource src (
      make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
      project_sample_rate, current_bpm, u8"gap_test", nullptr);
    src.set_store (store);
    src.flush_tail ();
    j = src;
  }
  // Introduce a gap in the manifest.
  j["manifest"]["blocks"][0]["destStart"] = 10;

  FileAudioSource deserialized (nullptr);
  EXPECT_THROW (j.get_to (deserialized), std::exception);
}

TEST_F (FileAudioSourceTest, SerializationRejectsZeroChannels)
{
  nlohmann::json j;
  {
    FileAudioSource src (
      make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
      project_sample_rate, current_bpm, u8"zero_channels_test", nullptr);
    src.set_store (store);
    src.flush_tail ();
    j = src;
  }
  j["manifest"]["channels"] = 0;

  FileAudioSource deserialized (nullptr);
  EXPECT_THROW (j.get_to (deserialized), std::exception);
}

TEST_F (FileAudioSourceTest, SerializationRejectsInvalidBlockId)
{
  nlohmann::json j;
  {
    FileAudioSource src (
      make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
      project_sample_rate, current_bpm, u8"invalid_id_test", nullptr);
    src.set_store (store);
    src.flush_tail ();
    j = src;
  }
  j["manifest"]["blocks"][0]["id"] = "not-a-block-id";

  FileAudioSource deserialized (nullptr);
  EXPECT_THROW (j.get_to (deserialized), std::exception);
}

TEST_F (FileAudioSourceTest, SerializationRejectsZeroFrameSpan)
{
  nlohmann::json j;
  {
    FileAudioSource src (
      make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
      project_sample_rate, current_bpm, u8"zero_frames_test", nullptr);
    src.set_store (store);
    src.flush_tail ();
    j = src;
  }
  j["manifest"]["blocks"][0]["numFrames"] = 0;

  FileAudioSource deserialized (nullptr);
  EXPECT_THROW (j.get_to (deserialized), std::exception);
}

TEST_F (FileAudioSourceTest, SerializationRejectsNegativeSrcStart)
{
  nlohmann::json j;
  {
    FileAudioSource src (
      make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
      project_sample_rate, current_bpm, u8"negative_src_start_test", nullptr);
    src.set_store (store);
    src.flush_tail ();
    j = src;
  }
  j["manifest"]["blocks"][0]["srcStart"] = -10;

  FileAudioSource deserialized (nullptr);
  EXPECT_THROW (j.get_to (deserialized), std::exception);
}

TEST_F (FileAudioSourceTest, SerializationRejectsZeroSampleRate)
{
  nlohmann::json j;
  {
    FileAudioSource src (
      make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
      project_sample_rate, current_bpm, u8"zero_rate_test", nullptr);
    src.set_store (store);
    src.flush_tail ();
    j = src;
  }
  j["manifest"]["sampleRate"] = 0;

  FileAudioSource deserialized (nullptr);
  EXPECT_THROW (j.get_to (deserialized), std::exception);
}

TEST_F (FileAudioSourceTest, SerializationRejectsTooManyChannels)
{
  nlohmann::json j;
  {
    FileAudioSource src (
      make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
      project_sample_rate, current_bpm, u8"too_many_channels_test", nullptr);
    src.set_store (store);
    src.flush_tail ();
    j = src;
  }
  j["manifest"]["channels"] = 65;

  FileAudioSource deserialized (nullptr);
  EXPECT_THROW (j.get_to (deserialized), std::exception);
}

// A source with no channels refuses content writes.
TEST_F (FileAudioSourceTest, ZeroChannelSourceRefusesSetContent)
{
  FileAudioSource src (nullptr);
  src.set_store (store);

  utils::audio::AudioBuffer frames (0, 100);
  src.set_content (frames, project_sample_rate);

  EXPECT_EQ (src.get_num_frames (), units::samples (0));
}

// Replacing content with empty frames clears the source.
TEST_F (FileAudioSourceTest, SetContentWithEmptyFramesClearsSource)
{
  FileAudioSource src (
    make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"empty_replacement_test", nullptr);
  src.set_store (store);
  src.flush_tail ();
  ASSERT_EQ (src.get_num_frames (), units::samples (100));

  src.set_content (utils::audio::AudioBuffer (2, 0), units::sample_rate (48000));

  EXPECT_EQ (src.get_num_frames (), units::samples (0));
  EXPECT_TRUE (src.get_spans ().empty ());
  EXPECT_FALSE (src.has_pending_tail ());
  EXPECT_EQ (src.get_samplerate (), units::sample_rate (48000));
}

// A write failure part-way through a flush leaves the manifest and the
// pending tail untouched.
TEST_F (FileAudioSourceTest, WriteFailureDuringFlushLeavesSourceUnchanged)
{
  const auto block_frames = static_cast<int64_t> (
    AudioBlockStore::kTargetBlockPayloadBytes / (2 * sizeof (float)));
  const auto total_frames = block_frames * 2 + 1000;
  const auto buf = make_ramp_buffer (static_cast<int> (total_frames), 0.f);

  // Discover the second block's id so a corrupt file can occupy its name.
  AudioBlockStore scratch (temp_dir / "scratch-blocks");
  const auto      second_id = scratch.write_block (
    buf, project_sample_rate, units::samples (block_frames),
    units::samples (block_frames));
  std::filesystem::create_directories (store->get_blocks_directory ());
  std::ofstream corrupt (
    store->get_blocks_directory () / second_id.hex, std::ios::binary);
  corrupt << "corrupt";
  corrupt.close ();

  FileAudioSource src (
    buf, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"flush_failure_test", nullptr);
  src.set_store (store);

  EXPECT_THROW (src.flush_tail (), ZrythmException);
  EXPECT_TRUE (src.get_spans ().empty ());
  EXPECT_TRUE (src.has_pending_tail ());
  EXPECT_EQ (src.get_num_frames (), units::samples (total_frames));

  // Removing the corrupt file lets the flush complete and recover.
  std::filesystem::remove (store->get_blocks_directory () / second_id.hex);
  src.flush_tail ();
  EXPECT_EQ (src.get_num_frames (), units::samples (total_frames));
  EXPECT_EQ (src.get_spans ().size (), 3);

  utils::audio::AudioBuffer result (2, 500);
  src.read_span (
    units::samples (block_frames + 123), units::samples (500), result);
  for (int ch = 0; ch < 2; ++ch)
    {
      for (int i = 0; i < 500; ++i)
        {
          EXPECT_EQ (
            result.getSample (ch, i),
            buf.getSample (ch, static_cast<int> (block_frames) + 123 + i));
        }
    }
}

// Replacing a frame range shares the blocks outside it and references
// boundary blocks partially.
TEST_F (FileAudioSourceTest, ReplaceSpanSharesUntouchedBlocks)
{
  const auto block = static_cast<int64_t> (
    AudioBlockStore::kTargetBlockPayloadBytes / (2 * sizeof (float)));
  const auto total = block * 4 + block / 2;
  const auto original = make_ramp_buffer (static_cast<int> (total), 0.f);

  FileAudioSource src (
    original, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"replace_test", nullptr);
  src.set_store (store);
  src.flush_tail ();
  const auto old_spans = src.get_spans ();
  ASSERT_EQ (old_spans.size (), 5);

  const auto start = units::samples (block * 3 / 2);
  const auto old_len = units::samples (block * 5 / 4);
  const auto replacement = [&] {
    utils::audio::AudioBuffer buf (
      2, static_cast<int> (old_len.in (units::samples)));
    for (const auto ch : std::views::iota (0, buf.getNumChannels ()))
      {
        for (const auto i : std::views::iota (0, buf.getNumSamples ()))
          {
            buf.setSample (ch, i, 999.f);
          }
      }
    return buf;
  }();

  src.replace_span (start, old_len, replacement);

  EXPECT_EQ (src.get_num_frames (), units::samples (total));
  const auto new_spans = src.get_spans ();
  ASSERT_EQ (new_spans.size (), 7);
  // The first and last blocks are shared untouched; the blocks straddling
  // the range boundaries are referenced partially.
  EXPECT_EQ (new_spans.front ().block, old_spans.front ().block);
  EXPECT_EQ (new_spans.front ().num_frames, units::samples (block));
  EXPECT_EQ (new_spans.back ().block, old_spans.back ().block);
  EXPECT_TRUE (std::ranges::any_of (new_spans, [] (const auto &span) {
    return span.src_start_frame > units::samples (0);
  }));

  utils::audio::AudioBuffer result (2, static_cast<int> (total));
  src.read_span (units::samples (0), units::samples (total), result);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (int64_t{ 0 }, total))
        {
          const auto in_replacement =
            i >= start.in (units::samples)
            && i < (start + old_len).in (units::samples);
          const auto expected =
            in_replacement ? 999.f : original.getSample (ch, static_cast<int> (i));
          EXPECT_EQ (result.getSample (ch, static_cast<int> (i)), expected);
        }
    }
}

// A replacement with a different length re-tiles the following spans.
TEST_F (FileAudioSourceTest, ReplaceSpanWithDifferentLengthRetiles)
{
  const auto block = static_cast<int64_t> (
    AudioBlockStore::kTargetBlockPayloadBytes / (2 * sizeof (float)));
  const auto total = block * 4 + block / 2;
  const auto original = make_ramp_buffer (static_cast<int> (total), 0.f);

  FileAudioSource src (
    original, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"replace_len_test", nullptr);
  src.set_store (store);
  src.flush_tail ();

  const auto                start = units::samples (block * 3 / 2);
  const auto                old_len = units::samples (block * 5 / 4);
  constexpr auto            new_len = 50000;
  utils::audio::AudioBuffer replacement (2, new_len);
  for (const auto ch : std::views::iota (0, replacement.getNumChannels ()))
    {
      for (const auto i : std::views::iota (0, replacement.getNumSamples ()))
        {
          replacement.setSample (ch, i, 888.f);
        }
    }

  src.replace_span (start, old_len, replacement);

  const auto new_total_raw = total - old_len.in (units::samples) + new_len;
  EXPECT_EQ (src.get_num_frames (), units::samples (new_total_raw));

  const auto                new_total = src.get_num_frames ();
  utils::audio::AudioBuffer result (
    2, static_cast<int> (new_total.in (units::samples)));
  src.read_span (units::samples (0), new_total, result);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (
        const auto i :
        std::views::iota (int64_t{ 0 }, new_total.in (units::samples)))
        {
          const auto in_replacement =
            i >= start.in (units::samples)
            && i < start.in (units::samples) + new_len;
          const auto original_index =
            (i < start.in (units::samples))
              ? i
              : i - new_len + old_len.in (units::samples);
          const auto expected =
            in_replacement
              ? 888.f
              : original.getSample (ch, static_cast<int> (original_index));
          EXPECT_EQ (result.getSample (ch, static_cast<int> (i)), expected);
        }
    }
}

// Empty replacement frames delete the range.
TEST_F (FileAudioSourceTest, ReplaceSpanWithEmptyFramesDeletesRange)
{
  FileAudioSource src (
    make_ramp_buffer (1000, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"replace_del_test", nullptr);
  src.set_store (store);
  src.flush_tail ();
  const auto original = make_ramp_buffer (1000, 0.f);

  src.replace_span (
    units::samples (200), units::samples (500),
    utils::audio::AudioBuffer (2, 0));

  EXPECT_EQ (src.get_num_frames (), units::samples (500));
  utils::audio::AudioBuffer result (2, 500);
  src.read_span (units::samples (0), units::samples (500), result);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, 200))
        {
          EXPECT_EQ (result.getSample (ch, i), original.getSample (ch, i));
        }
      for (const auto i : std::views::iota (200, 500))
        {
          EXPECT_EQ (result.getSample (ch, i), original.getSample (ch, i + 500));
        }
    }
}

// A write failure part-way through a replacement leaves the manifest
// untouched.
TEST_F (FileAudioSourceTest, ReplaceSpanWriteFailureLeavesSourceUnchanged)
{
  const auto block = static_cast<int64_t> (
    AudioBlockStore::kTargetBlockPayloadBytes / (2 * sizeof (float)));
  const auto total = block * 4 + block / 2;
  const auto original = make_ramp_buffer (static_cast<int> (total), 0.f);

  FileAudioSource src (
    original, FileAudioSource::BitDepth::BIT_DEPTH_32, project_sample_rate,
    current_bpm, u8"replace_fail_test", nullptr);
  src.set_store (store);
  src.flush_tail ();
  const auto spans_before = src.get_spans ();

  const auto                start = units::samples (block * 3 / 2);
  const auto                old_len = units::samples (block * 5 / 4);
  utils::audio::AudioBuffer replacement (
    2, static_cast<int> (old_len.in (units::samples)));
  for (const auto ch : std::views::iota (0, replacement.getNumChannels ()))
    {
      for (const auto i : std::views::iota (0, replacement.getNumSamples ()))
        {
          replacement.setSample (ch, i, 999.f);
        }
    }

  // Discover the replacement's first block id so a corrupt file can
  // occupy its name.
  AudioBlockStore scratch (temp_dir / "scratch-blocks");
  const auto      new_id = scratch.write_block (
    replacement, project_sample_rate, units::samples (0),
    units::samples (block));
  std::ofstream corrupt (
    store->get_blocks_directory () / new_id.hex, std::ios::binary);
  corrupt << "corrupt";
  corrupt.close ();

  EXPECT_THROW (src.replace_span (start, old_len, replacement), ZrythmException);
  EXPECT_EQ (src.get_num_frames (), units::samples (total));
  const auto spans_after = src.get_spans ();
  ASSERT_EQ (spans_after.size (), spans_before.size ());
  for (size_t i = 0; i < spans_before.size (); ++i)
    {
      EXPECT_EQ (spans_after[i].block, spans_before[i].block);
      EXPECT_EQ (spans_after[i].num_frames, spans_before[i].num_frames);
    }
}

// Appending frames modifies the source's samples and must emit samplesChanged
TEST_F (FileAudioSourceTest, AppendFramesEmitsSamplesChanged)
{
  test_helpers::ScopedQCoreApplication app;

  FileAudioSource src (
    make_ramp_buffer (100, 0.f), FileAudioSource::BitDepth::BIT_DEPTH_32,
    project_sample_rate, current_bpm, u8"signal_test", nullptr);
  src.set_store (store);

  QSignalSpy spy (&src, &FileAudioSource::samplesChanged);
  ASSERT_TRUE (spy.isValid ());

  src.append_frames (make_ramp_buffer (10, 100.f));

  EXPECT_GT (spy.count (), 0);
}

} // namespace zrythm::dsp
