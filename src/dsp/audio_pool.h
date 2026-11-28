// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <filesystem>
#include <mutex>
#include <unordered_set>

#include "dsp/audio_block_store.h"
#include "dsp/file_audio_source.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief The audio block store of a project and the policy around it.
 *
 * The pool owns the project's AudioBlockStore and coordinates its contents
 * with the FileAudioSource objects in the registry: loading wires sources to
 * the store, saving flushes pending data, backups copy referenced blocks
 * and the garbage collector deletes unreferenced block files.
 */
struct AudioPool final
{
public:
  /**
   * @brief Returns a path that will be used to manage audio files in.
   */
  using ProjectPoolPathGetter = std::function<std::filesystem::path ()>;
  using SampleRateGetter = std::function<units::sample_rate_t ()>;

  AudioPool (
    utils::IObjectRegistry &registry,
    ProjectPoolPathGetter   path_getter,
    SampleRateGetter        sr_getter);

public:
  /** Returns the directory that holds the blocks of the pool at @p pool_dir. */
  [[nodiscard]] static std::filesystem::path
  blocks_directory_of (const std::filesystem::path &pool_dir);

  /**
   * @brief The pool's block store, created lazily at the pool's current
   * path.
   *
   * The returned pointer shares ownership with the pool: sources hold the
   * store alive even when the pool rebuilds its own instance, so the
   * pointer stays valid regardless of rebuilds.
   */
  std::shared_ptr<AudioBlockStore> get_block_store ();

  /**
   * @brief Wires the block store to every source and rematerializes sources
   * whose sample rate differs from the current one.
   *
   * @throw ZrythmException if a source could not be read or rewritten.
   */
  void init_loaded ();

  /**
   * @brief Writes the pending tail data of every source as blocks.
   *
   * Sources created without a block store are wired to the pool's store
   * here.
   *
   * @throw ZrythmException on I/O error.
   */
  void flush_pending_tails ();

  /**
   * @brief Copies the blocks referenced by the registry's sources into the
   * given blocks directory.
   *
   * A directory equal to the pool's own blocks directory is a no-op. The
   * directory is created if needed. Sources with pending tail data are
   * refused: flush them first so the copied snapshot is complete.
   *
   * @throw ZrythmException on I/O error.
   */
  void
  copy_blocks_to_directory (const std::filesystem::path &dest_blocks_dir) const;

  /**
   * @brief Copies the blocks referenced by the registry's sources from
   * the given blocks directory into the pool's own store.
   *
   * A directory equal to the pool's own blocks directory is a no-op, and
   * so is a registry without referenced blocks. The given directory is
   * only read; the pool's directory is created once every referenced
   * block was found in the given directory. The pending tail data of a
   * source is not part of the import: it is written to the pool's own
   * store when the source is flushed.
   *
   * @throw ZrythmException when a referenced block is missing from the
   * given directory, and on I/O error.
   */
  void import_referenced_blocks_from (
    const std::filesystem::path &source_blocks_dir) const;

  /**
   * @brief Deletes block files not referenced by any source from the
   * pool's own store.
   *
   * @throw ZrythmException on I/O error.
   */
  void sweep_unused_blocks ();

  /**
   * @brief Deletes block files not referenced by any source in the given
   * blocks directory.
   *
   * @throw ZrythmException on I/O error.
   */
  void
  remove_unused_in_directory (const std::filesystem::path &blocks_dir) const;

  /**
   * @brief Rebuilds the block store at the pool's current path and rewires
   * every source to it.
   *
   * Pending data is flushed to the previous store and every referenced
   * block missing from the new directory is copied from the previous one,
   * so blocks written before the move (recording continuing during Save
   * As) are migrated as well. Used when the pool moves to another
   * directory (Save As).
   */
  void rebuild_block_store ();

  void
  for_each_clip (std::function<void (dsp::FileAudioSource &)> visitor) const;

private:
  friend void init_from (
    AudioPool             &obj,
    const AudioPool       &other,
    utils::ObjectCloneType clone_type);

  /**
   * Copies every block of @p referenced that is missing from
   * @p dest_blocks_dir from @p src_blocks_dir. The destination
   * directory is created if needed.
   */
  void copy_referenced_blocks (
    const std::unordered_set<BlockId, BlockIdHash> &referenced,
    const std::filesystem::path                    &src_blocks_dir,
    const std::filesystem::path                    &dest_blocks_dir) const;

  /**
   * Copies every referenced block missing from @p dest_blocks_dir from
   * @p src_blocks_dir. The destination directory is created if needed.
   */
  void copy_referenced_blocks (
    const std::filesystem::path &src_blocks_dir,
    const std::filesystem::path &dest_blocks_dir) const;

  /** The block ids referenced by the registry's sources. */
  [[nodiscard]] std::unordered_set<BlockId, BlockIdHash>
  collect_referenced_blocks () const;

private:
  SampleRateGetter      sample_rate_getter_;
  ProjectPoolPathGetter project_pool_path_getter_;

  /**
   * Audio clips.
   */
  utils::IObjectRegistry &registry_;

  std::shared_ptr<AudioBlockStore> store_;

  /**
   * Serializes the store's construction: the pool is reached from the main
   * thread and the save task, which would otherwise each build their own
   * store over the same directory.
   */
  mutable std::mutex store_init_mutex_;
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
    pool.for_each_clip ([&] (const auto &clip) {
      ss << fmt::format (
        "[Clip {}] {}: {} blocks\n", clip.get_uuid (), clip.get_name (),
        clip.get_spans ().size ());
    });

    return fmt::formatter<std::string_view>::format (
      fmt::format ("{}", ss.str ()), ctx);
  }
};
