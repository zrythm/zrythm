// SPDX-FileCopyrightText: © 2019-2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>

#include "actions/undo_manager.h"
#include "dsp/clip.h"
#include "dsp/pool.h"
#include "dsp/track.h"
#include "dsp/tracklist.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/error.h"
#include "utils/file.h"
#include "utils/flags.h"
#include "utils/io.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/string.h"
#include "zrythm.h"

#include <glib/gi18n.h>

#include "gtk_wrapper.h"

void
AudioPool::init_loaded ()
{
  for (auto &clip : clips_)
    {
      if (clip)
        clip->init_loaded ();
    }
}

bool
AudioPool::name_exists (const std::string &name) const
{
  return std::any_of (clips_.begin (), clips_.end (), [&name] (const auto &clip) {
    return clip && clip->name_ == name;
  });
}

void
AudioPool::ensure_unique_clip_name (AudioClip &clip)
{
  constexpr bool is_backup = false;
  auto        orig_name_without_ext = io_file_strip_ext (clip.name_.c_str ());
  auto        orig_path_in_pool = clip.get_path_in_pool (is_backup);
  std::string new_name = orig_name_without_ext;
  z_return_if_fail (!new_name.empty ());

  bool changed = false;
  while (name_exists (new_name))
    {
      const auto     prev_new_name = new_name;
      constexpr auto regex = R"(^.*\((\d+)\)$)";
      char * cur_val_str = string_get_regex_group (new_name.c_str (), regex, 1);
      int    cur_val =
        string_get_regex_group_as_int (new_name.c_str (), regex, 1, 0);
      if (cur_val == 0)
        {
          new_name = fmt::format ("{} (1)", new_name);
        }
      else
        {
          size_t len =
            strlen (new_name.c_str ()) -
            /* + 2 for the parens */
            (strlen (cur_val_str) + 2);
          /* + 1 for the terminating NULL */
          size_t tmp_len = len + 1;
          char   tmp[tmp_len];
          memset (tmp, 0, tmp_len * sizeof (char));
          memcpy (tmp, new_name.c_str (), len == 0 ? 0 : len - 1);
          new_name = fmt::format ("{} ({})", std::string (tmp), cur_val + 1);
        }
      g_free (cur_val_str);
      changed = true;
    }

  auto new_path_in_pool =
    AudioClip::get_path_in_pool_from_name (new_name, clip.use_flac_, is_backup);
  if (changed)
    {
      z_return_if_fail (new_path_in_pool != orig_path_in_pool);
    }

  clip.name_ = new_name;
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
          next_id = std::max (clip->pool_id_, next_id);
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
  z_return_val_if_fail (!clip->name_.empty (), -1);

  z_debug ("adding clip <%s> to pool...", clip->name_);

  ensure_unique_clip_name (*clip);

  int next_id = get_next_id ();
  z_return_val_if_fail (next_id >= 0 && next_id <= (int) clips_.size (), -1);

  clip->pool_id_ = next_id;
  if (next_id == (int) clips_.size ())
    {
      clips_.emplace_back (std::move (clip));
    }
  else
    {
      z_return_val_if_fail (clips_[next_id] == nullptr, -1);
      clips_[next_id] = std::move (clip);
    }

  z_debug ("added clip <{}> to pool", clips_[next_id]->name_);
  print ();

  return next_id;
}

AudioClip *
AudioPool::get_clip (int clip_id)
{
  z_return_val_if_fail (clip_id >= 0 && clip_id < (int) clips_.size (), nullptr);

  auto it =
    std::find_if (clips_.begin (), clips_.end (), [clip_id] (const auto &clip) {
      return clip && clip->pool_id_ == clip_id;
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
    clip->frames_.getReadPointer (0), clip->num_frames_, clip->channels_,
    clip->bit_depth_, clip->name_));
  auto new_clip = get_clip (new_id);

  z_debug ("duplicating clip %s to %s...", clip->name_, new_clip->name_);

  /* assert clip names are not the same */
  z_return_val_if_fail (clip->name_ != new_clip->name_, -1);

  if (write_file)
    {
      new_clip->write_to_pool (false, false);
    }

  return new_clip->pool_id_;
}

std::string
AudioPool::gen_name_for_recording_clip (const Track &track, int lane)
{
  return fmt::format (
    "%s - lane %d - recording", track.name_,
    /* add 1 to get human friendly index */
    lane + 1);
}

void
AudioPool::remove_clip (int clip_id, bool free_and_remove_file, bool backup)
{
  z_debug ("removing clip with ID %d", clip_id);

  auto clip = get_clip (clip_id);
  z_return_if_fail (clip);

  if (free_and_remove_file)
    {
      clip->remove (backup);
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
      if (clip && !clip->is_in_use (true))
        {
          z_info ("unused clip [{}]: {}", i, clip->name_);
          remove_clip (i, true, backup);
          removed_clips++;
        }
    }

  /* remove untracked files from pool directory */
  auto prj_pool_dir = PROJECT->get_path (ProjectPath::POOL, backup);
  auto files = io_get_files_in_dir_ending_in (prj_pool_dir, true, std::nullopt);
  for (const auto &path : files)
    {
      bool found = false;
      for (const auto &clip : clips_)
        {
          if (!clip)
            continue;

          if (clip->get_path_in_pool (backup) == path)
            {
              found = true;
              break;
            }
        }

      /* if file not found in pool clips, delete */
      if (!found)
        {
          io_remove (path.toStdString ());
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

      bool in_use = clip->is_in_use (false);

      if (in_use && clip->get_num_frames () == 0)
        {
          /* load from the file */
          clip->init_loaded ();
        }
      else if (!in_use && clip->get_num_frames () > 0)
        {
          /* unload frames */
          clip->num_frames_ = 0;
          clip->frames_.clear ();
          clip->ch_frames_.clear ();
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

  static void free (void * data, GClosure * closure)
  {
    auto * self = (WriteClipData *) data;
    delete self;
  }
};

/**
 * Thread for writing an audio clip to disk.
 *
 * To be used as a GThreadFunc.
 */
static void
write_clip_thread (void * data, void * user_data)
{
  auto * write_clip_data = (WriteClipData *) data;
  try
    {
      write_clip_data->clip->write_to_pool (false, write_clip_data->is_backup);
      write_clip_data->successful = true;
    }
  catch (const ZrythmException &e)
    {
      write_clip_data->successful = false;
      write_clip_data->error = e.what ();
    }
}

void
AudioPool::write_to_disk (bool is_backup)
{
  /* ensure pool dir exists */
  auto prj_pool_dir = PROJECT->get_path (ProjectPath::POOL, is_backup);
  if (!file_path_exists (prj_pool_dir))
    {
      try
        {
          io_mkdir (prj_pool_dir);
        }
      catch (const ZrythmException &e)
        {
          std::throw_with_nested (
            ZrythmException ("Failed to create pool directory"));
        }
    }

  GError *      err = NULL;
  GThreadPool * thread_pool = g_thread_pool_new (
    write_clip_thread, this, (int) g_get_num_processors (), F_NOT_EXCLUSIVE,
    &err);
  if (err)
    {
      throw ZrythmException (
        fmt::format ("Failed to create thread pool: {}", err->message));
    }

  std::vector<WriteClipData> clip_data_vec;
  for (auto &clip : clips_)
    {
      if (clip)
        {
          clip_data_vec.emplace_back (WriteClipData{ clip.get (), is_backup });

          /* start writing in a new thread */
          g_thread_pool_push (thread_pool, &clip_data_vec.back (), nullptr);
        }
    }

  z_debug ("waiting for thread pool to finish...");
  g_thread_pool_free (thread_pool, false, true);
  z_debug ("done");

  for (auto &clip_data : clip_data_vec)
    {
      if (!clip_data.successful)
        {
          throw ZrythmException (fmt::format (
            "Failed to write clip {}: {}", clip_data.clip->name_,
            clip_data.error));
        }
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
          auto pool_path = clip->get_path_in_pool (false);
          ss << fmt::format (
            "[Clip #{}] {} ({}): {}\n", i, clip->name_, clip->file_hash_,
            pool_path);
        }
      else
        {
          ss << fmt::format ("[Clip #{}] <empty>\n", i);
        }
    }
  z_info ("%s", ss.str ());
}