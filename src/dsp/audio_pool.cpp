// SPDX-FileCopyrightText: © 2019-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <limits>
#include <utility>

#include <fmt/std.h>

#include "dsp/audio_pool.h"
#include "utils/audio.h"
#include "utils/exceptions.h"
#include "utils/io_utils.h"
#include "utils/logger.h"
#include "utils/registry_utils.h"
#include "utils/resampler.h"
#include "utils/serialization.h"
#include "utils/utf8_string.h"
#include "utils/uuid_identifiable_object.h"

using zrythm::utils::ZrythmException;

namespace zrythm::dsp
{

AudioPool::AudioPool (
  utils::IObjectRegistry &registry,
  ProjectPoolPathGetter   path_getter,
  SampleRateGetter        sr_getter)
    : sample_rate_getter_ (std::move (sr_getter)),
      project_pool_path_getter_ (std::move (path_getter)), registry_ (registry)
{
}

void
AudioPool::for_each_clip (
  std::function<void (dsp::FileAudioSource &)> visitor) const
{
  registry_.for_each_matching<dsp::FileAudioSource> (std::move (visitor));
}

void
AudioPool::init_loaded ()
{
  const auto store = get_block_store ();
  for_each_clip ([&] (dsp::FileAudioSource &src) {
    src.set_store (store);

    const auto current_rate = sample_rate_getter_ ();
    if (src.get_samplerate () == current_rate)
      {
        return;
      }

    // The source's blocks were written at a different rate: read them back,
    // resample and write new blocks at the current rate.
    // TODO: stream the rematerialization through windowed reads and
    // incremental resampling instead of materializing the whole source.
    const auto num_frames = src.get_num_frames ();
    if (num_frames > units::samples (std::numeric_limits<int>::max ()))
      {
        throw ZrythmException (
          fmt::format (
            "Source '{}' is too large to resample ({} frames)",
            src.get_name ().str (), num_frames.in (units::samples)));
      }
    utils::audio::AudioBuffer frames (
      src.get_num_channels (),
      static_cast<int> (num_frames.in (units::samples)));
    src.read_span (units::samples (0), num_frames, frames);

    Resampler resampler (
      frames,
      static_cast<double> (src.get_samplerate ().in (units::sample_rate)),
      static_cast<double> (current_rate.in (units::sample_rate)),
      Resampler::Quality::High);
    while (!resampler.is_done ())
      {
        resampler.process ();
      }
    const auto &resampled = resampler.get_out_frames ();
    if (resampled.getNumSamples () == 0)
      {
        z_warning (
          "resampling source '{}' discarded all of its frames - the source "
          "becomes empty",
          src.get_name ().str ());
      }
    src.set_content (resampled, current_rate);
  });
}

void
init_from (
  AudioPool             &obj,
  const AudioPool       &other,
  utils::ObjectCloneType clone_type)
{
  // The pool's getters and registry are constructor-injected per
  // instance: nothing is copied from the source pool.
}

std::shared_ptr<AudioBlockStore>
AudioPool::get_block_store ()
{
  const std::lock_guard lock (store_init_mutex_);
  // The store is created lazily because the pool path is only resolvable
  // once the owning project knows its directory.
  if (!store_)
    {
      store_ = std::make_shared<AudioBlockStore> (
        project_pool_path_getter_ () / "blocks");
    }
  return store_;
}

void
AudioPool::rebuild_block_store ()
{
  const std::lock_guard lock (store_init_mutex_);
  const auto            replacement =
    std::make_shared<AudioBlockStore> (project_pool_path_getter_ () / "blocks");

  // Pending data is flushed to the previous store and each source is
  // rewired, so the migration below sees every block the sources
  // reference.
  for_each_clip ([&replacement] (dsp::FileAudioSource &src) {
    if (src.has_pending_tail ())
      {
        src.flush_tail ();
      }
    src.set_store (replacement);
  });

  // Blocks written to the previous directory after a save copied the pool
  // (recording continuing during Save As) are migrated here.
  if (
    store_
    && store_->get_blocks_directory () != replacement->get_blocks_directory ())
    {
      copy_referenced_blocks (
        store_->get_blocks_directory (), replacement->get_blocks_directory ());
    }

  store_ = std::move (replacement);
}

void
AudioPool::flush_pending_tails ()
{
  const auto store = get_block_store ();
  for_each_clip ([&] (dsp::FileAudioSource &src) {
    src.set_store (store);
    if (src.has_pending_tail ())
      {
        src.flush_tail ();
      }
  });
}

std::unordered_set<BlockId, BlockIdHash>
AudioPool::collect_referenced_blocks () const
{
  std::unordered_set<BlockId, BlockIdHash> referenced;
  for_each_clip ([&] (const dsp::FileAudioSource &src) {
    for (const auto &span : src.get_spans ())
      {
        referenced.insert (span.block);
      }
  });
  return referenced;
}

void
AudioPool::copy_blocks_to_directory (
  const std::filesystem::path &dest_blocks_dir) const
{
  const auto main_dir = project_pool_path_getter_ () / "blocks";
  if (dest_blocks_dir == main_dir)
    {
      return;
    }

  for_each_clip ([&] (const dsp::FileAudioSource &src) {
    if (src.has_pending_tail ())
      {
        throw ZrythmException (
          fmt::format (
            "Source '{}' has pending data - flush it before copying blocks",
            src.get_name ().str ()));
      }
  });

  copy_referenced_blocks (main_dir, dest_blocks_dir);
}

void
AudioPool::copy_referenced_blocks (
  const std::filesystem::path &src_blocks_dir,
  const std::filesystem::path &dest_blocks_dir) const
{
  std::filesystem::create_directories (dest_blocks_dir);
  for (const auto &id : collect_referenced_blocks ())
    {
      const auto dest_path = dest_blocks_dir / id.hex;
      if (std::filesystem::exists (dest_path))
        {
          continue;
        }
      const auto src_path = src_blocks_dir / id.hex;
      if (!utils::io::reflink_file (dest_path, src_path))
        {
          utils::io::copy_file (dest_path, src_path);
        }
    }
}

void
AudioPool::sweep_unused_blocks ()
{
  const auto store = get_block_store ();
  if (!std::filesystem::exists (store->get_blocks_directory ()))
    {
      return;
    }

  const auto referenced = collect_referenced_blocks ();
  size_t     removed = 0;
  for (const auto &id : store->get_all_block_ids ())
    {
      if (!referenced.contains (id))
        {
          store->delete_block (id);
          ++removed;
        }
    }
  if (removed > 0)
    {
      z_info ("removed {} unused blocks from the pool", removed);
    }
}

void
AudioPool::remove_unused_in_directory (
  const std::filesystem::path &blocks_dir) const
{
  if (!std::filesystem::exists (blocks_dir))
    {
      return;
    }

  // The sweep runs through a store limited to the given directory; the
  // project's own store only knows the main pool.
  AudioBlockStore dir_store (blocks_dir);
  const auto      referenced = collect_referenced_blocks ();
  for (const auto &id : dir_store.get_all_block_ids ())
    {
      if (!referenced.contains (id))
        {
          dir_store.delete_block (id);
        }
    }
}

} // namespace zrythm::dsp
