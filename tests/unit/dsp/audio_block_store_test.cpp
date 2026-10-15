// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <filesystem>
#include <fstream>

#include "dsp/audio_block_store.h"
#include "utils/audio.h"
#include "utils/exceptions.h"
#include "utils/io_utils.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class AudioBlockStoreTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    temp_dir_obj = zrythm::utils::io::make_tmp_dir ();
    temp_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();
    blocks_dir = temp_dir / "blocks";
  }

  /** Deterministic stereo ramp: L rises from base, R mirrors it. */
  static utils::audio::AudioBuffer make_ramp_buffer (int frames, float base)
  {
    utils::audio::AudioBuffer buf (2, frames);
    for (int frame = 0; frame < frames; ++frame)
      {
        const auto value = base + static_cast<float> (frame);
        buf.setSample (0, frame, value);
        buf.setSample (1, frame, -value);
      }
    return buf;
  }

  static size_t payload_bytes (const utils::audio::AudioBuffer &buf)
  {
    return static_cast<size_t> (buf.getNumChannels ())
           * static_cast<size_t> (buf.getNumSamples ()) * sizeof (float);
  }

  std::unique_ptr<QTemporaryDir> temp_dir_obj;
  std::filesystem::path          temp_dir;
  std::filesystem::path          blocks_dir;
  const units::sample_rate_t     rate_48k = units::sample_rate (48000);
  const units::sample_rate_t     rate_44k1 = units::sample_rate (44100);
};

TEST_F (AudioBlockStoreTest, WriteReadRoundTripCreatesDirectory)
{
  AudioBlockStore store (blocks_dir);

  const auto frames = make_ramp_buffer (1000, 0.5f);
  const auto id = store.write_block (
    frames, rate_48k, units::samples (0), units::samples (1000));

  EXPECT_TRUE (store.has_block (id));
  EXPECT_TRUE (std::filesystem::exists (store.get_blocks_directory () / id.hex));

  const auto block = store.read_block (id);
  EXPECT_EQ (block->getNumChannels (), 2);
  EXPECT_EQ (block->getNumSamples (), 1000);
  for (int channel = 0; channel < 2; ++channel)
    {
      for (int frame = 0; frame < 1000; ++frame)
        {
          EXPECT_EQ (
            block->getSample (channel, frame), frames.getSample (channel, frame))
            << "at channel " << channel << " frame " << frame;
        }
    }
}

TEST_F (AudioBlockStoreTest, WriteSubRangeStoresOnlyThatRange)
{
  AudioBlockStore store (blocks_dir);

  const auto frames = make_ramp_buffer (100, 0.f);
  const auto id = store.write_block (
    frames, rate_48k, units::samples (10), units::samples (5));

  const auto block = store.read_block (id);
  EXPECT_EQ (block->getNumChannels (), 2);
  EXPECT_EQ (block->getNumSamples (), 5);
  for (int channel = 0; channel < 2; ++channel)
    {
      for (int frame = 0; frame < 5; ++frame)
        {
          EXPECT_EQ (
            block->getSample (channel, frame),
            frames.getSample (channel, 10 + frame));
        }
    }
}

TEST_F (AudioBlockStoreTest, IdenticalContentDeduplicates)
{
  AudioBlockStore store (blocks_dir);

  const auto first = store.write_block (
    make_ramp_buffer (500, 1.0f), rate_48k, units::samples (0),
    units::samples (500));
  const auto second = store.write_block (
    make_ramp_buffer (500, 1.0f), rate_48k, units::samples (0),
    units::samples (500));

  EXPECT_EQ (first, second);
  EXPECT_EQ (store.get_all_block_ids ().size (), 1);
}

// A file occupying a block's name without holding its bytes must not
// satisfy future writes of that block.
TEST_F (AudioBlockStoreTest, DedupHitOnWrongSizedFileThrows)
{
  AudioBlockStore store (blocks_dir);
  const auto      id = store.write_block (
    make_ramp_buffer (100, 0.f), rate_48k, units::samples (0),
    units::samples (100));

  std::error_code ec;
  std::filesystem::resize_file (store.get_blocks_directory () / id.hex, 10, ec);
  ASSERT_FALSE (ec);

  EXPECT_THROW (
    store.write_block (
      make_ramp_buffer (100, 0.f), rate_48k, units::samples (0),
      units::samples (100)),
    ZrythmException);
}

TEST_F (AudioBlockStoreTest, HeaderValuesAffectId)
{
  AudioBlockStore store (blocks_dir);

  const auto frames = make_ramp_buffer (500, 1.0f);
  const auto at_48k = store.write_block (
    frames, rate_48k, units::samples (0), units::samples (500));
  const auto at_44k1 = store.write_block (
    frames, rate_44k1, units::samples (0), units::samples (500));

  EXPECT_NE (at_48k, at_44k1);
  EXPECT_EQ (store.get_all_block_ids ().size (), 2);
}

TEST_F (AudioBlockStoreTest, DifferentContentProducesDifferentIds)
{
  AudioBlockStore store (blocks_dir);

  const auto id_a = store.write_block (
    make_ramp_buffer (500, 1.0f), rate_48k, units::samples (0),
    units::samples (500));
  const auto id_b = store.write_block (
    make_ramp_buffer (500, 1.5f), rate_48k, units::samples (0),
    units::samples (500));

  EXPECT_NE (id_a, id_b);
  EXPECT_EQ (store.get_all_block_ids ().size (), 2);
}

TEST_F (AudioBlockStoreTest, ReadMissingBlockThrows)
{
  AudioBlockStore store (blocks_dir);
  EXPECT_THROW (
    store.read_block (BlockId{ std::string (64, '0') }), ZrythmException);
}

TEST_F (AudioBlockStoreTest, CorruptMagicThrows)
{
  AudioBlockStore store (blocks_dir);
  const auto      id = store.write_block (
    make_ramp_buffer (100, 0.f), rate_48k, units::samples (0),
    units::samples (100));

  const auto    path = store.get_blocks_directory () / id.hex;
  std::ofstream out (path, std::ios::binary | std::ios::in);
  out.put ('X');
  out.close ();

  EXPECT_THROW (store.read_block (id), ZrythmException);
}

TEST_F (AudioBlockStoreTest, TruncatedBlockThrows)
{
  AudioBlockStore store (blocks_dir);
  const auto      id = store.write_block (
    make_ramp_buffer (100, 0.f), rate_48k, units::samples (0),
    units::samples (100));

  std::error_code ec;
  std::filesystem::resize_file (store.get_blocks_directory () / id.hex, 10, ec);
  ASSERT_FALSE (ec);

  EXPECT_THROW (store.read_block (id), ZrythmException);
}

TEST_F (AudioBlockStoreTest, CacheEvictsToBudget)
{
  AudioBlockStore store (blocks_dir);
  const auto      payload = make_ramp_buffer (1000, 0.f);
  store.set_cache_budget_bytes (payload_bytes (payload));

  const auto id_a = store.write_block (
    payload, rate_48k, units::samples (0), units::samples (1000));
  const auto id_b = store.write_block (
    make_ramp_buffer (1000, 1.0f), rate_48k, units::samples (0),
    units::samples (1000));

  (void) store.read_block (id_a);
  EXPECT_EQ (store.cached_block_count (), 1);

  // Reading the second block evicts the first.
  (void) store.read_block (id_b);
  EXPECT_LE (store.cached_block_count (), 1);
  EXPECT_LE (store.cached_bytes (), payload_bytes (payload));

  // A re-read of the evicted block reloads it from disk correctly.
  const auto reloaded = store.read_block (id_a);
  EXPECT_EQ (reloaded->getSample (0, 0), payload.getSample (0, 0));
}

TEST_F (AudioBlockStoreTest, EvictedBlockStaysAliveForHolders)
{
  AudioBlockStore store (blocks_dir);
  const auto      payload = make_ramp_buffer (1000, 0.f);
  store.set_cache_budget_bytes (payload_bytes (payload));

  const auto id_a = store.write_block (
    payload, rate_48k, units::samples (0), units::samples (1000));
  const auto id_b = store.write_block (
    make_ramp_buffer (1000, 1.0f), rate_48k, units::samples (0),
    units::samples (1000));

  const auto held = store.read_block (id_a);
  (void) store.read_block (id_b);
  EXPECT_EQ (store.cached_block_count (), 1);

  // The holder keeps the evicted block's data alive and intact.
  EXPECT_EQ (held->getNumSamples (), 1000);
  EXPECT_EQ (held->getSample (0, 0), payload.getSample (0, 0));
  EXPECT_EQ (held->getSample (1, 999), payload.getSample (1, 999));
}

TEST_F (AudioBlockStoreTest, DeleteRemovesFileAndCache)
{
  AudioBlockStore store (blocks_dir);
  const auto      id = store.write_block (
    make_ramp_buffer (100, 0.f), rate_48k, units::samples (0),
    units::samples (100));

  (void) store.read_block (id);
  ASSERT_EQ (store.cached_block_count (), 1);

  EXPECT_TRUE (store.delete_block (id));
  EXPECT_FALSE (store.has_block (id));
  EXPECT_EQ (store.cached_block_count (), 0);
  EXPECT_EQ (store.cached_bytes (), 0);
  EXPECT_THROW (store.read_block (id), ZrythmException);
  EXPECT_FALSE (store.delete_block (id));
}

TEST_F (AudioBlockStoreTest, RewriteAfterDeleteKeepsId)
{
  AudioBlockStore store (blocks_dir);
  const auto      payload = make_ramp_buffer (100, 0.f);
  const auto      id = store.write_block (
    payload, rate_48k, units::samples (0), units::samples (100));

  ASSERT_TRUE (store.delete_block (id));
  const auto rewritten = store.write_block (
    payload, rate_48k, units::samples (0), units::samples (100));

  EXPECT_EQ (id, rewritten);
  EXPECT_TRUE (store.has_block (id));
}

TEST_F (AudioBlockStoreTest, InvalidInputsRejected)
{
  AudioBlockStore store (blocks_dir);
  const auto      payload = make_ramp_buffer (10, 0.f);

  EXPECT_THROW (
    store.write_block (
      utils::audio::AudioBuffer (0, 10), rate_48k, units::samples (0),
      units::samples (10)),
    ZrythmException);
  EXPECT_THROW (
    store.write_block (payload, rate_48k, units::samples (0), units::samples (0)),
    ZrythmException);
  EXPECT_THROW (
    store.write_block (
      payload, rate_48k, units::samples (5), units::samples (100)),
    ZrythmException);
  EXPECT_THROW (
    store.write_block (
      payload, units::sample_rate (0), units::samples (0), units::samples (10)),
    ZrythmException);

  EXPECT_TRUE (store.get_all_block_ids ().empty ());
}

TEST_F (AudioBlockStoreTest, GetAllBlockIdsIgnoresForeignFiles)
{
  AudioBlockStore store (blocks_dir);
  (void) store.write_block (
    make_ramp_buffer (10, 0.f), rate_48k, units::samples (0),
    units::samples (10));
  (void) store.write_block (
    make_ramp_buffer (10, 1.0f), rate_48k, units::samples (0),
    units::samples (10));

  std::ofstream (store.get_blocks_directory () / "notes.txt") << "x";
  std::ofstream (store.get_blocks_directory () / ".abcdef.tmp") << "x";

  EXPECT_EQ (store.get_all_block_ids ().size (), 2);
}

TEST_F (AudioBlockStoreTest, ConstructorRemovesStaleTempFiles)
{
  std::filesystem::create_directories (blocks_dir);
  const auto stale_tmp = blocks_dir / ".0123456789abcdef.0.tmp";
  const auto kept = blocks_dir / "0123456789abcdef";
  {
    std::ofstream out (stale_tmp, std::ios::binary);
    out << "x";
  }
  {
    std::ofstream out (kept, std::ios::binary);
    out << "x";
  }

  AudioBlockStore store (blocks_dir);

  EXPECT_FALSE (std::filesystem::exists (stale_tmp));
  // Files that are not temporary files are left alone.
  EXPECT_TRUE (std::filesystem::exists (kept));
}

TEST_F (AudioBlockStoreTest, OversizedHeaderFrameCountThrows)
{
  std::filesystem::create_directories (blocks_dir);
  // A header claiming ~17 GiB of payload with only the header on disk.
  const auto                    id = BlockId{ std::string (64, '1') };
  std::array<unsigned char, 18> header{};
  header[0] = 'Z';
  header[1] = 'B';
  header[2] = 'L';
  header[3] = 'O';
  header[4] = 'C';
  header[5] = 'K';
  header[6] = 1; // version
  header[7] = 0;
  header[8] = 2; // channels
  header[9] = 0;
  const uint32_t rate = 48000;
  for (int byte = 0; byte < 4; ++byte)
    header[10 + byte] = static_cast<unsigned char> ((rate >> (8 * byte)) & 0xff);
  const uint32_t frame_count = 0xffffffff;
  for (int byte = 0; byte < 4; ++byte)
    header[14 + byte] =
      static_cast<unsigned char> ((frame_count >> (8 * byte)) & 0xff);
  {
    std::ofstream out (blocks_dir / id.hex, std::ios::binary);
    out.write (
      reinterpret_cast<const char *> (header.data ()),
      static_cast<std::streamsize> (header.size ()));
  }

  AudioBlockStore store (blocks_dir);
  EXPECT_THROW (store.read_block (id), ZrythmException);
}

} // namespace zrythm::dsp
