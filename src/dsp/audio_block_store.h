// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <list>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include "utils/audio.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Content hash identifying an audio block file.
 *
 * The hash covers the block's full file contents (header and payload), so
 * identical audio at different sample rates or channel counts gets distinct
 * ids.
 */
struct BlockId
{
  /** 64 lowercase hex characters (SHA-256). */
  std::string hex;

  bool operator== (const BlockId &other) const = default;

  /** Returns whether @ref hex is a valid block file name. */
  [[nodiscard]] bool is_valid () const;
};

struct BlockIdHash
{
  size_t operator() (const BlockId &id) const noexcept
  {
    return std::hash<std::string>{}(id.hex);
  }
};

/**
 * @brief Storage for immutable, content-addressed audio blocks.
 *
 * A block is a small binary file — a fixed header followed by one
 * little-endian float32 plane per channel — named after the SHA-256 of its
 * full contents, under the blocks directory. Blocks are written once and
 * never modified; identical contents deduplicate to a single file regardless
 * of how many sources reference them.
 *
 * Reads are served through a bounded least-recently-used cache. Blocks
 * returned to callers are kept alive through shared ownership, so eviction
 * never invalidates data in use.
 *
 * Realtime audio threads never access the store; playback reads the rendered
 * playback caches. The directory-enumeration methods (has_block,
 * get_all_block_ids) take no lock and are meant for quiescent points such as
 * the post-save garbage collection.
 */
class AudioBlockStore final
{
public:
  /**
   * Payload size writers aim for when splitting audio into blocks; smaller
   * final blocks are allowed.
   */
  static constexpr size_t kTargetBlockPayloadBytes = 1024 * 1024;

  /**
   * @param blocks_directory Directory the block files live in. Created on
   * first write. Temporary files left behind by interrupted writes are
   * removed.
   */
  explicit AudioBlockStore (std::filesystem::path blocks_directory);

  AudioBlockStore (const AudioBlockStore &) = delete;
  AudioBlockStore &operator= (const AudioBlockStore &) = delete;

  /**
   * @brief Writes a block and returns its id.
   *
   * If a block with the same contents already exists, no file is written
   * and the existing id is returned.
   *
   * @param frames Planar frames to store the range
   * [start_frame, start_frame + num_frames) of, one channel per buffer
   * channel.
   *
   * @throw ZrythmException on invalid input or I/O failure.
   */
  BlockId write_block (
    const utils::audio::AudioBuffer &frames,
    units::sample_rate_t             sample_rate,
    units::sample_t                  start_frame,
    units::sample_t                  num_frames);

  /**
   * @brief Returns the block's frames.
   *
   * The block file is read and cached if not cached already.
   *
   * @throw ZrythmException if the block file is missing or corrupt.
   */
  std::shared_ptr<const utils::audio::AudioBuffer>
  read_block (const BlockId &id);

  /** Returns whether a block file with this id exists on disk. */
  [[nodiscard]] bool has_block (const BlockId &id) const;

  /**
   * @brief Deletes the block file and drops it from the cache.
   *
   * @return Whether the file existed.
   */
  bool delete_block (const BlockId &id);

  /**
   * @brief Returns the ids of all block files in the blocks directory.
   *
   * Files whose names are not valid block ids are ignored.
   */
  [[nodiscard]] std::vector<BlockId> get_all_block_ids () const;

  [[nodiscard]] const std::filesystem::path &get_blocks_directory () const;

  /** Sets the maximum number of payload bytes the read cache holds. */
  void set_cache_budget_bytes (size_t bytes);

  /** Number of blocks currently held in the read cache. */
  [[nodiscard]] size_t cached_block_count () const;

  /** Sum of payload bytes currently held in the read cache. */
  [[nodiscard]] size_t cached_bytes () const;

private:
  struct CacheEntry
  {
    std::shared_ptr<const utils::audio::AudioBuffer> data;
    std::list<BlockId>::iterator                     lru_position;
  };

  [[nodiscard]] std::filesystem::path block_path (const BlockId &id) const;
  void                                ensure_blocks_directory () const;
  void                                evict_over_budget ();

  std::filesystem::path blocks_directory_;

  mutable std::mutex mutex_;

  std::unordered_map<BlockId, CacheEntry, BlockIdHash> cache_;

  /** Cache ids in use order; front = most recently used. */
  std::list<BlockId> lru_order_;

  size_t cache_bytes_ = 0;
  size_t cache_budget_bytes_ = 256 * 1024 * 1024;

  uint64_t temp_file_counter_ = 0;
};

} // namespace zrythm::dsp
