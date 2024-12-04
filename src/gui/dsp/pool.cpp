// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "gui/backend/backend/actions/undo_manager.h"
#include "gui/backend/backend/project.h"
#include "gui/backend/backend/zrythm.h"
#include "gui/dsp/clip.h"
#include "gui/dsp/pool.h"
#include "gui/dsp/track.h"
#include "gui/dsp/tracklist.h"

#include "utils/io.h"
#include "utils/string.h"

using namespace zrythm;

AudioPool::AudioPool (AudioEngine * engine) : engine_ (engine) { }

void
AudioPool::init_loaded (AudioEngine * engine)
{
  engine_ = engine;
  for (auto &clip : clips_)
    {
      if (clip)
        {
          clip->init_loaded (get_clip_path_from_name (
            clip->get_name (), clip->get_use_flac (), false));
        }
    }
}

void
AudioPool::init_after_cloning (const AudioPool &other)
{
  clone_ptr_vector (clips_, other.clips_);
}

bool
AudioPool::name_exists (const std::string &name) const
{
  return std::any_of (clips_.begin (), clips_.end (), [&name] (const auto &clip) {
    return clip && clip->get_name () == name;
  });
}

fs::path
AudioPool::get_clip_path_from_name (
  const std::string &name,
  bool               use_flac,
  bool               is_backup)
{
  auto prj_pool_dir = PROJECT->get_path (ProjectPath::POOL, is_backup);
  if (!utils::io::path_exists (prj_pool_dir))
    {
      z_error ("{} does not exist", prj_pool_dir);
      return "";
    }
  std::string basename =
    utils::io::file_strip_ext (name) + (use_flac ? ".FLAC" : ".wav");
  return prj_pool_dir / basename;
}

fs::path
AudioPool::get_clip_path (const AudioClip &clip, bool is_backup)
{
  return get_clip_path_from_name (
    clip.get_name (), clip.get_use_flac (), is_backup);
}

void
AudioPool::write_clip (AudioClip &clip, bool parts, bool backup)
{
  AudioClip * pool_clip = get_clip (clip.get_pool_id ());
  z_return_if_fail (pool_clip == &clip);

  print ();
  z_debug (
    "attempting to write clip {} ({}) to pool...", clip.get_name (),
    clip.get_pool_id ());

  /* generate a copy of the given filename in the project dir */
  auto path_in_main_project = get_clip_path (clip, false);
  auto new_path = get_clip_path (clip, backup);
  z_return_if_fail (!path_in_main_project.empty ());
  z_return_if_fail (!new_path.empty ());

  /* whether a new write is needed */
  bool need_new_write = true;

  /* skip if file with same hash already exists */
  if (utils::io::path_exists (new_path) && !parts)
    {
      bool same_hash =
        clip.get_file_hash () != 0
        && clip.get_file_hash () == utils::hash::get_file_hash (new_path);

      if (same_hash)
        {
          z_debug ("skipping writing to existing clip {} in pool", new_path);
          need_new_write = false;
        }
    }

  /* if writing to backup and same file exists in main project dir, copy (first
   * try reflink) */
  if (need_new_write && clip.get_file_hash () != 0 && backup)
    {
      bool exists_in_main_project = false;
      if (utils::io::path_exists (path_in_main_project))
        {
          exists_in_main_project =
            clip.get_file_hash ()
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

  if (need_new_write)
    {
      z_debug (
        "writing clip {} to pool (parts {}, is backup  {}): '{}'",
        clip.get_name (), parts, backup, new_path);
      clip.write_to_file (new_path, parts);
      if (!parts)
        {
          /* store file hash */
          clip.set_file_hash (utils::hash::get_file_hash (new_path));
        }
    }

  print ();
}

void
AudioPool::ensure_unique_clip_name (AudioClip &clip)
{
  constexpr bool is_backup = false;
  auto orig_name_without_ext = utils::io::file_strip_ext (clip.get_name ());
  auto orig_path_in_pool = get_clip_path (clip, is_backup);
  std::string    new_name = orig_name_without_ext;
  z_return_if_fail (!new_name.empty ());

  bool changed = false;
  while (name_exists (new_name))
    {
      const auto     prev_new_name = new_name;
      constexpr auto regex = R"(^.*\((\d+)\)$)";
      auto cur_val_str = utils::string::get_regex_group (new_name, regex, 1);
      int  cur_val =
        utils::string::get_regex_group_as_int (new_name, regex, 1, 0);
      if (cur_val == 0)
        {
          new_name = fmt::format ("{} (1)", new_name);
        }
      else
        {
          size_t len =
            strlen (new_name.c_str ()) -
            /* + 2 for the parens */
            (strlen (cur_val_str.c_str ()) + 2);
          /* + 1 for the terminating NULL */
          size_t            tmp_len = len + 1;
          std::vector<char> tmp (tmp_len);
          memset (tmp.data (), 0, tmp_len * sizeof (char));
          memcpy (tmp.data (), new_name.c_str (), len == 0 ? 0 : len - 1);
          new_name =
            fmt::format ("{} ({})", std::string (tmp.data ()), cur_val + 1);
        }
      changed = true;
    }

  auto new_path_in_pool =
    get_clip_path_from_name (new_name, clip.get_use_flac (), is_backup);
  if (changed)
    {
      z_return_if_fail (new_path_in_pool != orig_path_in_pool);
    }

  clip.set_name (new_name);
}

int
AudioPool::get_next_id () const
{
  int next_id = -1;
  for (size_t i = 0; i < clips_.size (); ++i)
    {
      const auto &clip = clips_[i];
      if (clip)
        {
          next_id = std::max (clip->get_pool_id (), next_id);
        }
      else
        {
          return i;
        }
    }

  return next_id + 1;
}

int
AudioPool::add_clip (std::unique_ptr<AudioClip> &&clip)
{
  z_return_val_if_fail (!clip->get_name ().empty (), -1);

  z_debug ("adding clip <{}> to pool...", clip->get_name ());

  ensure_unique_clip_name (*clip);

  int next_id = get_next_id ();
  z_return_val_if_fail (next_id >= 0 && next_id <= (int) clips_.size (), -1);

  clip->set_pool_id (next_id);
  if (next_id == (int) clips_.size ())
    {
      clips_.emplace_back (std::move (clip));
    }
  else
    {
      z_return_val_if_fail (clips_[next_id] == nullptr, -1);
      clips_[next_id] = std::move (clip);
    }

  z_debug ("added clip <{}> to pool", clips_[next_id]->get_name ());
  print ();

  return next_id;
}

AudioClip *
AudioPool::get_clip (int clip_id)
{
  z_return_val_if_fail (clip_id >= 0 && clip_id < (int) clips_.size (), nullptr);

  auto it =
    std::find_if (clips_.begin (), clips_.end (), [clip_id] (const auto &clip) {
      return clip && clip->get_pool_id () == clip_id;
    });
  z_return_val_if_fail (it != clips_.end (), nullptr);
  return (*it).get ();
}

int
AudioPool::duplicate_clip (int clip_id, bool write_file)
{
  const auto clip = get_clip (clip_id);
  z_return_val_if_fail (clip, -1);

  auto new_id = add_clip (std::make_unique<AudioClip> (
    clip->get_samples (), clip->get_bit_depth (), engine_->sample_rate_,
    P_TEMPO_TRACK->get_current_bpm (), clip->get_name ()));
  auto new_clip = get_clip (new_id);

  z_debug (
    "duplicating clip {} to {}...", clip->get_name (), new_clip->get_name ());

  /* assert clip names are not the same */
  z_return_val_if_fail (clip->get_name () != new_clip->get_name (), -1);

  if (write_file)
    {
      write_clip (*new_clip, false, false);
    }

  return new_clip->get_pool_id ();
}

std::string
AudioPool::gen_name_for_recording_clip (const Track &track, int lane)
{
  return fmt::format (
    "{} - lane {} - recording", track.name_,
    /* add 1 to get human friendly index */
    lane + 1);
}

void
AudioPool::remove_clip (int clip_id, bool free_and_remove_file, bool backup)
{
  z_debug ("removing clip with ID {}", clip_id);

  auto clip = get_clip (clip_id);
  z_return_if_fail (clip);

  if (free_and_remove_file)
    {
      std::string path = get_clip_path (*clip, backup);
      z_debug ("removing clip at {}", path);
      z_return_if_fail (path.length () > 0);
      utils::io::remove (path);
    }

  auto removed_clip = std::move (clips_[clip_id]);
}

void
AudioPool::remove_unused (bool backup)
{
  z_debug ("--- removing unused files from pool ---");

  /* remove clips from the pool that are not in use */
  int removed_clips = 0;
  for (size_t i = 0; i < clips_.size (); ++i)
    {
      auto &clip = clips_[i];
      if (clip && !PROJECT->is_audio_clip_in_use (*clip, true))
        {
          z_info ("unused clip [{}]: {}", i, clip->get_name ());
          remove_clip (i, true, backup);
          removed_clips++;
        }
    }

  /* remove untracked files from pool directory */
  auto prj_pool_dir = PROJECT->get_path (ProjectPath::POOL, backup);
  auto files =
    utils::io::get_files_in_dir_ending_in (prj_pool_dir, true, std::nullopt);
  for (const auto &path : files)
    {
      bool found = false;
      for (const auto &clip : clips_)
        {
          if (!clip)
            continue;

          if (get_clip_path (*clip, backup) == fs::path (path.toStdString ()))
            {
              found = true;
              break;
            }
        }

      /* if file not found in pool clips, delete */
      if (!found)
        {
          utils::io::remove (path.toStdString ());
        }
    }

  z_info ("removed {} clips", removed_clips);
}

void
AudioPool::reload_clip_frame_bufs ()
{
  for (auto &clip : clips_)
    {
      if (!clip)
        continue;

      const auto in_use = PROJECT->is_audio_clip_in_use (*clip, false);

      if (in_use && clip->get_num_frames () == 0)
        {
          /* load from the file */
          clip->init_loaded (get_clip_path_from_name (
            clip->get_name (), clip->get_use_flac (), false));
        }
      else if (!in_use && clip->get_num_frames () > 0)
        {
          /* unload frames */
          clip->clear_frames ();
        }
    }
}

struct WriteClipData
{
  AudioClip * clip;
  bool        is_backup;

  /** To be set after writing the file. */
  bool        successful = false;
  std::string error;
};

void
AudioPool::write_to_disk (bool is_backup)
{
  z_return_if_fail (engine_);

  /* ensure pool dir exists */
  auto prj_pool_dir = engine_->project_->get_path (ProjectPath::POOL, is_backup);
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

  const int        num_threads = juce::SystemStats::getNumCpus ();
  juce::ThreadPool pool (num_threads);

  std::string           error_message;
  juce::CriticalSection error_mutex;

  for (auto &clip : clips_)
    {
      if (clip)
        {
          pool.addJob ([this, &clip, is_backup, &error_message, &error_mutex] () {
            try
              {
                write_clip (*clip, false, is_backup);
              }
            catch (const ZrythmException &e)
              {
                const juce::ScopedLock lock (error_mutex);
                if (error_message.empty ())
                  {
                    error_message = fmt::format (
                      "Failed to write clip {} to disk: {}", clip->get_name (),
                      e.what ());
                  }
              }
          });
        }
    }

  z_debug ("waiting for tasks to finish...");
  pool.removeAllJobs (false, -1);
  z_debug ("done");

  if (!error_message.empty ())
    {
      throw ZrythmException (error_message);
    }
}

void
AudioPool::print () const
{
  std::stringstream ss;
  ss << "[Audio Pool]\n";
  for (size_t i = 0; i < clips_.size (); ++i)
    {
      const auto &clip = clips_[i];
      if (clip)
        {
          auto pool_path = get_clip_path (*clip, false);
          ss << fmt::format (
            "[Clip #{}] {} ({}): {}\n", i, clip->get_name (),
            clip->get_file_hash (), pool_path);
        }
      else
        {
          ss << fmt::format ("[Clip #{}] <empty>\n", i);
        }
    }
  z_info ("{}", ss.str ());
}
