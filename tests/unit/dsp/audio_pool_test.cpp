// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <filesystem>

#include "dsp/audio_pool.h"
#include "dsp/file_audio_source.h"
#include "utils/io_utils.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{
class AudioPoolTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create temporary test directory using thread-safe temporary directory
    temp_dir_obj = zrythm::utils::io::make_tmp_dir ();
    temp_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();

    // Create sample audio clip
    clip_id_ref = registry.create_object<FileAudioSource> (
      utils::audio::AudioBuffer (2, 100),
      FileAudioSource::BitDepth::BIT_DEPTH_32, units::sample_rate (44100),
      120.0, u8"test_clip", nullptr);
    clip_id = clip_id_ref->id ();

    // Setup AudioPool
    path_getter = [this] (bool /*backup*/) { return temp_dir; };
    sample_rate_getter = [] { return units::sample_rate (44100); };
    audio_pool =
      std::make_unique<AudioPool> (registry, path_getter, sample_rate_getter);
  }

  void TearDown () override
  {
    audio_pool.reset ();
    // QTemporaryDir auto-removes when destroyed, no manual cleanup needed
  }

  std::unique_ptr<QTemporaryDir>              temp_dir_obj;
  std::filesystem::path                       temp_dir;
  FileAudioSourceRegistry                     registry;
  std::optional<FileAudioSourceUuidReference> clip_id_ref;
  FileAudioSource::Uuid                       clip_id;
  AudioPool::ProjectPoolPathGetter            path_getter;
  AudioPool::SampleRateGetter                 sample_rate_getter;
  std::unique_ptr<AudioPool>                  audio_pool;
};

// Test construction and initialization
TEST_F (AudioPoolTest, Construction)
{
  EXPECT_NE (audio_pool, nullptr);
}

// Test loading existing clips from disk
TEST_F (AudioPoolTest, LoadExistingClips)
{
  // Write clip to disk first
  auto * clip =
    std::get<FileAudioSource *> (registry.find_by_id_or_throw (clip_id));
  ASSERT_NO_THROW (audio_pool->write_clip (clip, false, false));

  // Create a new pool to simulate loading
  AudioPool new_pool (registry, path_getter, sample_rate_getter);

  // Initialize from loaded state
  EXPECT_NO_THROW (new_pool.init_loaded ());

  // Verify clip was loaded
  EXPECT_EQ (new_pool.get_clip_ptrs ().size (), 1);
}

// Test clip duplication
TEST_F (AudioPoolTest, DuplicateClip)
{
  auto new_clip_id = audio_pool->duplicate_clip (clip_id, false);
  EXPECT_FALSE (new_clip_id.id ().is_null ());
  EXPECT_NE (new_clip_id.id (), clip_id);

  // Verify clip exists in registry
  EXPECT_TRUE (registry.contains (new_clip_id.id ()));
}

// Test clip path retrieval
TEST_F (AudioPoolTest, GetClipPath)
{
  auto path = audio_pool->get_clip_path (clip_id, false);
  EXPECT_FALSE (path.empty ());
  EXPECT_TRUE (path.extension () == ".wav");
}

// Test clip writing
TEST_F (AudioPoolTest, WriteClip)
{
  auto * clip =
    std::get<FileAudioSource *> (registry.find_by_id_or_throw (clip_id));
  ASSERT_NO_THROW (audio_pool->write_clip (clip, false, false));

  // Verify file was created
  auto path = audio_pool->get_clip_path (clip_id, false);
  EXPECT_TRUE (utils::io::path_exists (path));
}

// Test removing unused clips
TEST_F (AudioPoolTest, RemoveUnused)
{
  // Create a clip that won't be used
  std::optional<FileAudioSourceUuidReference> unused_clip =
    registry.create_object<FileAudioSource> (
      utils::audio::AudioBuffer (2, 50), FileAudioSource::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), 120.0, u8"unused_clip", nullptr);
  auto unused_path = audio_pool->get_clip_path (unused_clip->id (), false);

  // Write both clips
  auto * clip =
    std::get<FileAudioSource *> (registry.find_by_id_or_throw (clip_id));
  audio_pool->write_clip (clip, false, false);
  audio_pool->write_clip (
    unused_clip->get_object_as<dsp::FileAudioSource> (), false, false);

  // No longer use the clip
  unused_clip.reset ();
  EXPECT_TRUE (utils::io::path_exists (unused_path)); // file still exists

  // Remove unused - should remove only the unused clip
  ASSERT_NO_THROW (audio_pool->remove_unused (false));

  EXPECT_TRUE (
    utils::io::path_exists (audio_pool->get_clip_path (clip_id, false)));
  EXPECT_FALSE (utils::io::path_exists (unused_path));
}

// Test reloading clip frame buffers
TEST_F (AudioPoolTest, ReloadClipFrameBufs)
{
  // Write the clip
  auto * clip =
    std::get<FileAudioSource *> (registry.find_by_id_or_throw (clip_id));
  audio_pool->write_clip (clip, false, false);

  // Initially clear frames
  clip->clear_frames ();
  EXPECT_EQ (clip->get_num_frames (), 0);

  // Reload should restore frames
  ASSERT_NO_THROW (audio_pool->reload_clip_frame_bufs ());
  EXPECT_GT (clip->get_num_frames (), 0);
}

// Test writing all clips to disk
TEST_F (AudioPoolTest, WriteToDisk)
{
  // Add multiple clips
  std::vector<FileAudioSourceUuidReference> clip_ids;
  for (int i = 0; i < 3; i++)
    {
      auto clip = registry.create_object<FileAudioSource> (
        utils::audio::AudioBuffer (2, 100 + i * 10),
        FileAudioSource::BitDepth::BIT_DEPTH_32, units::sample_rate (44100),
        120.0,
        utils::Utf8String::from_utf8_encoded_string (fmt::format ("clip_{}", i)),
        nullptr);
      clip_ids.emplace_back (std::move (clip));
    }

  // Write all to disk
  ASSERT_NO_THROW (audio_pool->write_to_disk (false));

  // Verify files exist
  for (const auto &id : clip_ids)
    {
      EXPECT_TRUE (
        utils::io::path_exists (audio_pool->get_clip_path (id.id (), false)));
    }
}

// Test serialization/deserialization
TEST_F (AudioPoolTest, Serialization)
{
  nlohmann::json j;

  // Serialize
  j = *audio_pool;

  // Deserialize
  AudioPool deserialized (registry, path_getter, sample_rate_getter);
  j.get_to (deserialized);

  // Verify properties match
  // (Add more specific checks based on serialized properties)
  EXPECT_EQ (j.dump (), nlohmann::json (deserialized).dump ());
}

} // namespace zrythm::dsp
