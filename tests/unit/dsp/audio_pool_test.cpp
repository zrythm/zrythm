// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>

#include "dsp/audio_pool.h"
#include "dsp/file_audio_source.h"
#include "utils/io_utils.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class AudioPoolTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    temp_dir_obj = zrythm::utils::io::make_tmp_dir ();
    temp_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();
    pool_dir = temp_dir / "pool";
    backup_dir = temp_dir / "backup-pool";

    path_getter = [this] () { return pool_dir; };
    sample_rate_getter = [] { return units::sample_rate (44100); };
    audio_pool =
      std::make_unique<AudioPool> (registry, path_getter, sample_rate_getter);
  }

  void TearDown () override
  {
    audio_pool.reset ();
    // QTemporaryDir auto-removes when destroyed, no manual cleanup needed
  }

  FileAudioSourceUuidReference
  make_source (const utils::Utf8String &name, int num_frames)
  {
    auto ref = utils::create_object<FileAudioSource> (
      registry, utils::audio::AudioBuffer (2, num_frames),
      FileAudioSource::BitDepth::BIT_DEPTH_32, units::sample_rate (44100),
      units::bpm (120.0), name, nullptr);
    ref.get_object_as<FileAudioSource> ()->set_store (
      audio_pool->get_block_store ());
    return ref;
  }

  std::unique_ptr<QTemporaryDir>   temp_dir_obj;
  std::filesystem::path            temp_dir;
  std::filesystem::path            pool_dir;
  std::filesystem::path            backup_dir;
  utils::ObjectRegistry            registry;
  AudioPool::ProjectPoolPathGetter path_getter;
  AudioPool::SampleRateGetter      sample_rate_getter;
  std::unique_ptr<AudioPool>       audio_pool;
};

TEST_F (AudioPoolTest, Construction)
{
  EXPECT_NE (audio_pool, nullptr);
}

// A cloned pool resolves its block store through its own
// constructor-injected getters.
TEST_F (AudioPoolTest, ClonedPoolUsesItsOwnGetters)
{
  const auto other_dir = temp_dir / "other-pool";
  const auto clone = utils::clone_unique (
    *audio_pool, utils::ObjectCloneType::Snapshot, registry,
    [other_dir] () { return other_dir; }, sample_rate_getter);

  EXPECT_EQ (
    clone->get_block_store ()->get_blocks_directory (), other_dir / "blocks");
}

TEST_F (AudioPoolTest, FlushPendingTailsWritesBlocks)
{
  auto   ref = make_source (u8"test_clip", 100);
  auto * source = ref.get_object_as<FileAudioSource> ();
  ASSERT_TRUE (source->has_pending_tail ());

  audio_pool->flush_pending_tails ();

  EXPECT_FALSE (source->has_pending_tail ());
  EXPECT_EQ (source->get_num_frames (), units::samples (100));
  EXPECT_EQ (source->get_spans ().size (), 1);
  EXPECT_EQ (audio_pool->get_block_store ()->get_all_block_ids ().size (), 1);
}

TEST_F (AudioPoolTest, InitLoadedWiresStoreToSources)
{
  auto ref = make_source (u8"test_clip", 100);
  audio_pool->flush_pending_tails ();
  auto * source = ref.get_object_as<FileAudioSource> ();

  // Simulate loading with a fresh pool.
  AudioPool new_pool (registry, path_getter, sample_rate_getter);
  EXPECT_NO_THROW (new_pool.init_loaded ());

  // The source reads back through the new pool's store.
  utils::audio::AudioBuffer frames (2, 100);
  EXPECT_NO_THROW (
    source->read_span (units::samples (0), units::samples (100), frames));
}

TEST_F (AudioPoolTest, InitLoadedRematerializesOnRateChange)
{
  const auto ref = utils::create_object<FileAudioSource> (
    registry, utils::audio::AudioBuffer (2, 100),
    FileAudioSource::BitDepth::BIT_DEPTH_32, units::sample_rate (48000),
    units::bpm (120.0), u8"rate_test", nullptr);
  auto * source = ref.get_object_as<FileAudioSource> ();
  source->set_store (audio_pool->get_block_store ());
  audio_pool->flush_pending_tails ();
  const auto old_blocks = audio_pool->get_block_store ()->get_all_block_ids ();

  // Load with the pool running at 44.1 kHz.
  AudioPool new_pool (registry, path_getter, sample_rate_getter);
  ASSERT_NO_THROW (new_pool.init_loaded ());

  EXPECT_EQ (source->get_samplerate (), units::sample_rate (44100));
  // 100 frames at 48 kHz become ~92 frames at 44.1 kHz.
  EXPECT_NEAR (source->get_num_frames ().in (units::samples), 92, 2);

  // The old-rate blocks become orphans and are swept; only the rematerialized
  // block remains.
  new_pool.sweep_unused_blocks ();
  const auto new_blocks = audio_pool->get_block_store ()->get_all_block_ids ();
  EXPECT_EQ (new_blocks.size (), 1);
  EXPECT_FALSE (new_blocks.front () == old_blocks.front ());
}

TEST_F (AudioPoolTest, RemoveUnusedSweepsOrphanBlocks)
{
  auto used_ref = make_source (u8"used_clip", 100);
  std::optional<FileAudioSourceUuidReference> unused_ref (
    make_source (u8"unused_clip", 50));
  audio_pool->flush_pending_tails ();
  ASSERT_EQ (audio_pool->get_block_store ()->get_all_block_ids ().size (), 2);

  // Dropping the reference removes the source from the registry.
  const auto unused_block_id =
    unused_ref->get_object_as<FileAudioSource> ()->get_spans ().front ().block;
  unused_ref.reset ();

  audio_pool->sweep_unused_blocks ();

  const auto remaining = audio_pool->get_block_store ()->get_all_block_ids ();
  ASSERT_EQ (remaining.size (), 1);
  EXPECT_EQ (
    remaining.front (),
    used_ref.get_object_as<FileAudioSource> ()->get_spans ().front ().block);
  EXPECT_NE (remaining.front (), unused_block_id);
}

TEST_F (AudioPoolTest, CopyBlocksToDirectoryAndPrune)
{
  auto ref = make_source (u8"test_clip", 100);
  audio_pool->flush_pending_tails ();

  ASSERT_NO_THROW (audio_pool->copy_blocks_to_directory (backup_dir / "blocks"));

  const auto backup_store_dir = backup_dir / "blocks";
  ASSERT_EQ (std::filesystem::exists (backup_store_dir), true);
  const auto * source = ref.get_object_as<FileAudioSource> ();
  const auto   block_id = source->get_spans ().front ().block;
  EXPECT_TRUE (std::filesystem::exists (backup_store_dir / block_id.hex));

  // A stale file in the backup blocks directory gets pruned.
  std::ofstream stale (backup_store_dir / std::string (64, 'a'));
  stale.close ();
  audio_pool->remove_unused_in_directory (backup_store_dir);
  EXPECT_FALSE (
    std::filesystem::exists (backup_store_dir / std::string (64, 'a')));
}

TEST_F (AudioPoolTest, CopyBlocksToDirectoryRefusesPendingData)
{
  auto ref = make_source (u8"test_clip", 100);
  // Deliberately no flush before the copy.
  EXPECT_THROW (
    audio_pool->copy_blocks_to_directory (backup_dir / "blocks"),
    ZrythmException);
}

// The Save As sequence: blocks are copied to the new project's pool, then
// the pool moves to the new directory
TEST_F (AudioPoolTest, MoveToAnotherDirectoryCopiesAndRewires)
{
  auto ref = make_source (u8"test_clip", 100);
  audio_pool->flush_pending_tails ();
  auto *     source = ref.get_object_as<FileAudioSource> ();
  const auto block_hex = source->get_spans ().front ().block.hex;

  const auto new_pool_dir = temp_dir / "moved-pool";
  audio_pool->copy_blocks_to_directory (new_pool_dir / "blocks");
  ASSERT_TRUE (std::filesystem::exists (new_pool_dir / "blocks" / block_hex));

  pool_dir = new_pool_dir;
  audio_pool->rebuild_block_store ();
  EXPECT_EQ (
    audio_pool->get_block_store ()->get_blocks_directory (),
    new_pool_dir / "blocks");

  // The source reads through the new store, where the block was copied.
  utils::audio::AudioBuffer frames (2, 100);
  EXPECT_NO_THROW (
    source->read_span (units::samples (0), units::samples (100), frames));

  // New writes land in the new pool directory.
  auto ref2 = make_source (u8"test_clip2", 50);
  audio_pool->flush_pending_tails ();
  const auto block_hex_2 =
    ref2.get_object_as<FileAudioSource> ()->get_spans ().front ().block.hex;
  EXPECT_TRUE (std::filesystem::exists (new_pool_dir / "blocks" / block_hex_2));
}

// Blocks written to the old pool after a save copied it (recording
// continuing during Save As) are migrated when the pool moves.
TEST_F (AudioPoolTest, RebuildMigratesBlocksWrittenAfterPoolCopy)
{
  auto ref = make_source (u8"test_clip", 100);
  audio_pool->flush_pending_tails ();

  const auto new_pool_dir = temp_dir / "moved-pool";
  audio_pool->copy_blocks_to_directory (new_pool_dir / "blocks");

  // Recording continues into the old store after the copy.
  utils::audio::AudioBuffer more (2, 50);
  ref.get_object_as<FileAudioSource> ()->append_frames (more);

  pool_dir = new_pool_dir;
  audio_pool->rebuild_block_store ();

  // Everything written before the move is readable through the new store.
  auto *                    source = ref.get_object_as<FileAudioSource> ();
  const auto                total = source->get_num_frames ();
  utils::audio::AudioBuffer frames (
    2, static_cast<int> (total.in (units::samples)));
  EXPECT_NO_THROW (source->read_span (units::samples (0), total, frames));
  EXPECT_EQ (source->get_spans ().size (), 2);
  for (const auto &span : source->get_spans ())
    {
      EXPECT_TRUE (
        std::filesystem::exists (new_pool_dir / "blocks" / span.block.hex));
    }

  // The next save's pool copy finds every referenced block in the pool's
  // own directory.
  EXPECT_NO_THROW (
    audio_pool->copy_blocks_to_directory (temp_dir / "other-pool" / "blocks"));
}

} // namespace zrythm::dsp
