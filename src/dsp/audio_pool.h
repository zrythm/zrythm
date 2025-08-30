// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/file_audio_source.h"
#include "utils/hash.h"
#include "utils/types.h"

#include <boost/unordered/concurrent_flat_map.hpp>

namespace zrythm::dsp
{

/**
 * @brief A manager for a registry of FileAudioSource inside a project.
 *
 * An audio pool is a pool of audio files and their corresponding float arrays
 * in memory that are referenced by regions.
 *
 * Instead of associating audio files with regions, all audio files (and their
 * edited counterparts after some hard editing like stretching) are saved in
 * the pool.
 */
struct AudioPool final
{
public:
  /**
   * @brief Returns a path that will be used to manage audio files in.
   *
   * @param backup Whether this path is for a backup project (as opposed to the
   * main project).
   */
  using ProjectPoolPathGetter = std::function<fs::path (bool backup)>;

  AudioPool (
    dsp::FileAudioSourceRegistry &file_audio_source_registry,
    ProjectPoolPathGetter         path_getter,
    SampleRateGetter              sr_getter);

public:
  /**
   * Initializes the audio pool after deserialization.
   *
   * @throw ZrythmException if an error occurred.
   */
  void init_loaded ();

  /**
   * Duplicates the clip with the given ID and returns the duplicate.
   *
   * @param write_file Whether to also write the file.
   * @return The ID in the pool.
   *
   * @throw ZrythmException If the file could not be written.
   */
  auto duplicate_clip (const FileAudioSource::Uuid &clip_id, bool write_file)
    -> FileAudioSourceUuidReference;

  /**
   * Gets the path of a clip matching @ref name from the pool.
   *
   * @param is_backup Whether writing to a backup project.
   */
  [[nodiscard]] fs::path
  get_clip_path (const dsp::FileAudioSource::Uuid &id, bool is_backup) const;

  /**
   * Writes the clip to the pool as a wav file.
   *
   * @param parts If true, only write new data. @see
   * FileAudioSource.frames_written.
   * @param backup Whether writing to a backup project.
   *
   * @throw ZrythmException on error.
   */
  void write_clip (const FileAudioSource * clip, bool parts, bool backup);

  /**
   * Removes and frees (and removes the files for) all clips not used by the
   * project or undo stacks.
   *
   * @param backup Whether to remove from backup directory.
   * @throw ZrythmException If any file could not be removed.
   */
  void remove_unused (bool backup);

  /**
   * Loads the frame buffers of clips currently in use in the project from their
   * files and frees the buffers of clips not currently in use.
   *
   * This should be called whenever there is a relevant change in the project
   * (eg, object added/removed).
   *
   * @throw ZrythmException If any file could not be read.
   */
  void reload_clip_frame_bufs ();

  /**
   * Writes all the clips to disk.
   *
   * Used when saving a project elsewhere.
   *
   * @param is_backup Whether this is a backup project.
   *
   * @throw ZrythmException If any file could not be written.
   */
  void write_to_disk (bool is_backup);

#if 0
  /**
   * @brief Returns a new audio source that can be used by other classes so that
   * they don't need to depend on FileAudioSource.

   * Calling this multiple times is allowed - each source has a separate
   * buffer/file reader.
   *
   * @return A unique audio source.
   */
  std::unique_ptr<juce::PositionableAudioSource>
  generate_audio_source (const FileAudioSource::Uuid &id) const;
#endif

  auto get_clip_ptrs () const
  {
    return std::views::transform (
      clip_registry_.get_hash_map () | std::views::values,
      [] (const auto &clip) { return std::get<dsp::FileAudioSource *> (clip); });
  }

private:
  friend void init_from (
    AudioPool             &obj,
    const AudioPool       &other,
    utils::ObjectCloneType clone_type);

  static constexpr auto kClipsKey = "clips"sv;
  static constexpr auto kLastKnownFileHashesKey = "lastKnownFileHashes"sv;
  friend void           to_json (nlohmann::json &j, const AudioPool &pool)
  {
    j[AudioPool::kLastKnownFileHashesKey] = pool.last_known_file_hashes_;
  }
  friend void from_json (const nlohmann::json &j, AudioPool &pool)
  {
    j.at (AudioPool::kLastKnownFileHashesKey)
      .get_to (pool.last_known_file_hashes_);
  }

private:
  SampleRateGetter      sample_rate_getter_;
  ProjectPoolPathGetter project_pool_path_getter_;

  /**
   * Audio clips.
   */
  FileAudioSourceRegistry &clip_registry_;

  /** File hashes, used for checking if a clip is already written to the pool so
   * we can save time by skipping overwriting it. */
  boost::unordered::concurrent_flat_map<FileAudioSource::Uuid, utils::hash::HashT>
    last_known_file_hashes_;
};
} // namespace zrythm::dsp

// Formatter for AudioPool
template <>
struct fmt::formatter<zrythm::dsp::AudioPool> : fmt::formatter<std::string_view>
{
  template <typename FormatContext>
  auto format (const zrythm::dsp::AudioPool &pool, FormatContext &ctx) const
  {
    std::stringstream ss;
    ss << "\nAudio Pool:\n";
    for (const auto &clip : pool.get_clip_ptrs ())
      {
        auto pool_path = pool.get_clip_path (clip->get_uuid (), false);
        ss << fmt::format (
          "[Clip {}] {}: {}\n", clip->get_uuid (), clip->get_name (), pool_path);
      }

    return fmt::formatter<std::string_view>::format (
      fmt::format ("{}", ss.str ()), ctx);
  }
};
