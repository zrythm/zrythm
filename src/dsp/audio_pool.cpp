// SPDX-FileCopyrightText: © 2019-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

#include "dsp/audio_pool.h"
#include "utils/io_utils.h"
#include "utils/utf8_string.h"

namespace zrythm::dsp
{

AudioPool::AudioPool (
  dsp::FileAudioSourceRegistry &file_audio_source_registry,
  ProjectPoolPathGetter         path_getter,
  SampleRateGetter              sr_getter)
    : sample_rate_getter_ (std::move (sr_getter)),
      project_pool_path_getter_ (std::move (path_getter)),
      clip_registry_ (file_audio_source_registry)
{
}

void
AudioPool::init_loaded ()
{
  for (auto * clip : get_clip_ptrs ())
    {
      clip->init_from_file (
        get_clip_path (clip->get_uuid (), false), clip->get_samplerate (),
        clip->get_bpm ());
    }
}

void
init_from (
  AudioPool             &obj,
  const AudioPool       &other,
  utils::ObjectCloneType clone_type)
{
  // nothing needed
}

fs::path
AudioPool::get_clip_path (const dsp::FileAudioSource::Uuid &id, bool is_backup)
  const
{
  auto prj_pool_dir = project_pool_path_getter_ (is_backup);
  if (!utils::io::path_exists (prj_pool_dir))
    {
      z_error ("{} does not exist", prj_pool_dir);
      return {};
    }
  const auto basename =
    utils::Utf8String::from_qstring (id.value_.toString (QUuid::WithoutBraces))
    + u8".wav";
  return prj_pool_dir / basename;
}

void
AudioPool::write_clip (const FileAudioSource * clip, bool parts, bool backup)
{
  z_debug (
    "attempting to write clip {} ({}) to pool...", clip->get_name (),
    clip->get_uuid ());
  const auto clip_id = clip->get_uuid ();

  /* generate a copy of the given filename in the project dir */
  auto path_in_main_project = get_clip_path (clip_id, false);
  auto new_path = get_clip_path (clip_id, backup);
  z_return_if_fail (!path_in_main_project.empty ());
  z_return_if_fail (!new_path.empty ());

  // get last known hash
  std::optional<utils::hash::HashT> last_known_hash_for_clip;
  {
    last_known_file_hashes_.cvisit (
      clip_id, [&last_known_hash_for_clip] (const auto &hash) {
        last_known_hash_for_clip = hash.second;
      });
  }

  /* skip if file with same hash already exists */
  if (
    last_known_hash_for_clip.has_value () && !parts
    && utils::io::path_exists (new_path)
    && last_known_hash_for_clip.value () == utils::hash::get_file_hash (new_path))
    {
      z_debug ("skipping writing to existing clip {} in pool", new_path);
      return;
    }

  /* if writing to backup and same file exists in main project dir, copy (first
   * try reflink) */
  if (last_known_hash_for_clip.has_value () && backup)
    {
      bool exists_in_main_project = false;
      if (utils::io::path_exists (path_in_main_project))
        {
          exists_in_main_project =
            last_known_hash_for_clip.value ()
            == utils::hash::get_file_hash (path_in_main_project);
        }

      if (exists_in_main_project)
        {
          /* try reflink and fall back to normal copying */
          z_debug (
            "reflinking clip from main project ('{}' to '{}')",
            path_in_main_project, new_path);

          if (!utils::io::reflink_file (path_in_main_project, new_path))
            {
              z_debug ("failed to reflink, copying instead");
              z_debug (
                "copying clip from main project ('{}' to '{}')",
                path_in_main_project, new_path);
              utils::io::copy_file (new_path, path_in_main_project);
            }
        }
    }

  z_debug (
    "writing clip {} to pool (parts {}, is backup  {}): '{}'",
    clip->get_name (), parts, backup, new_path);
  dsp::FileAudioSourceWriter writer{ *clip, new_path, parts };
  writer.write_to_file ();
  if (!parts)
    {
      /* store file hash */
      last_known_file_hashes_.emplace (
        clip_id, utils::hash::get_file_hash (new_path));
    }
}

auto
AudioPool::duplicate_clip (const FileAudioSource::Uuid &clip_id, bool write_file)
  -> FileAudioSourceUuidReference
{
  auto * const clip = std::get<dsp::FileAudioSource *> (
    clip_registry_.find_by_id_or_throw (clip_id));

  auto new_clip_ref = clip_registry_.create_object<FileAudioSource> (
    clip->get_samples (), clip->get_bit_depth (), sample_rate_getter_ (), 140.f,
    clip->get_name ());

  z_debug (
    "duplicating clip {} to {}...", clip->get_name (), new_clip_ref.id ());

  if (write_file)
    {
      write_clip (
        new_clip_ref.get_object_as<dsp::FileAudioSource> (), false, false);
    }

  return new_clip_ref;
}

void
AudioPool::remove_unused (bool backup)
{
  z_debug ("--- removing unused files from pool ---");

  // remove untracked files from pool directory
  // TODO: check the registry
  auto prj_pool_dir = project_pool_path_getter_ (backup);
  auto files =
    utils::io::get_files_in_dir_ending_in (prj_pool_dir, true, std::nullopt);
  auto removed_clips = 0;
  for (const auto &path : files)
    {
      bool found = false;
      for (const auto &clip_id : clip_registry_.get_uuids ())
        {
          if (get_clip_path (clip_id, backup) == path)
            {
              found = true;
              break;
            }
        }

      /* if file not found in pool clips, delete */
      if (!found)
        {
          utils::io::remove (path);
          ++removed_clips;
        }
    }

  z_info ("removed {} clips", removed_clips);
}

void
AudioPool::reload_clip_frame_bufs ()
{
  for (auto * clip : get_clip_ptrs ())
    {
      if (clip->get_num_frames () == 0)
        {
          /* load from the file */
          clip->init_from_file (
            get_clip_path (clip->get_uuid (), false), sample_rate_getter_ (),
            std::nullopt);
        }
    }
}

struct WriteClipData
{
  FileAudioSource * clip;
  bool              is_backup;

  /** To be set after writing the file. */
  bool        successful = false;
  std::string error;
};

void
AudioPool::write_to_disk (bool is_backup)
{
  /* ensure pool dir exists */
  auto prj_pool_dir = project_pool_path_getter_ (is_backup);
  if (!utils::io::path_exists (prj_pool_dir))
    {
      try
        {
          utils::io::mkdir (prj_pool_dir);
        }
      catch (const ZrythmException &e)
        {
          std::throw_with_nested (
            ZrythmException ("Failed to create pool directory"));
        }
    }

  // write clips in parallel
  const auto clips = get_clip_ptrs () | std::ranges::to<std::vector> ();
  std::vector<std::exception_ptr> exceptions;
  std::mutex                      error_mutex;
  const unsigned                  num_workers =
    std::max (1u, std::thread::hardware_concurrency ());
  std::queue<FileAudioSource *> clip_queue;
  for (auto * clip : clips)
    {
      clip_queue.push (clip);
    }

  std::mutex                queue_mutex;
  std::condition_variable   cv;
  std::vector<std::jthread> workers;

  workers.reserve (num_workers);
  for (unsigned i = 0; i < num_workers; ++i)
    {
      workers.emplace_back ([&] {
        while (true)
          {
            FileAudioSource * clip = nullptr;
            {
              std::unique_lock lock (queue_mutex);
              if (clip_queue.empty ())
                {
                  return;
                }
              clip = clip_queue.front ();
              clip_queue.pop ();
            }

            try
              {
                write_clip (clip, false, is_backup);
              }
            catch (const ZrythmException &e)
              {
                std::lock_guard lock (error_mutex);
                exceptions.push_back (std::current_exception ());
              }
          }
      });
    }

  // Wait for all workers to finish
  for (auto &worker : workers)
    {
      worker.join ();
    }

  // Check for errors
  for (const auto &eptr : exceptions)
    {
      if (eptr)
        {
          try
            {
              std::rethrow_exception (eptr);
            }
          catch (const std::exception &e)
            {
              throw ZrythmException (
                fmt::format ("Failed to write clips: {}", e.what ()));
            }
        }
    }

  z_debug ("done writing clips, {}", *this);
}

} // namespace zrythm::dsp
