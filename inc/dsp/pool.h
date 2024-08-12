// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio file pool.
 */

#ifndef __AUDIO_POOL_H__
#define __AUDIO_POOL_H__

#include <memory>
#include <vector>

#include "dsp/clip.h"

class Track;

/**
 * @addtogroup dsp
 *
 * @{
 */

#define AUDIO_POOL (AUDIO_ENGINE->pool_)

/**
 * An audio pool is a pool of audio files and their corresponding float arrays
 * in memory that are referenced by regions.
 *
 * Instead of associating audio files with regions, all audio files (and their
 * edited counterparts after some hard editing like stretching) are saved in
 * the pool.
 */
struct AudioPool final
    : public ICloneable<AudioPool>,
      public ISerializable<AudioPool>
{
  /* Rule of 0 */
public:
  /**
   * Initializes the audio pool after deserialization.
   *
   * @throw ZrythmException if an error occurred.
   */
  void init_loaded ();

  /**
   * Adds an audio clip to the pool.
   *
   * Changes the name of the clip if another clip with the same name already
   * exists.
   *
   * @return The ID in the pool.
   */
  int add_clip (std::unique_ptr<AudioClip> &&clip);

  /**
   * Duplicates the clip with the given ID and returns the duplicate.
   *
   * @param write_file Whether to also write the file.
   * @return The ID in the pool.
   *
   * @throw ZrythmException If the file could not be written.
   */
  int duplicate_clip (int clip_id, bool write_file);

  /**
   * Returns the clip for the given ID.
   */
  AudioClip * get_clip (int clip_id);

  /**
   * Removes the clip with the given ID from the pool and optionally frees it
   * (and removes the file).
   *
   * @param backup Whether to remove from backup directory.
   * @throw ZrythmException If the file could not be removed.
   */
  void remove_clip (int clip_id, bool free_and_remove_file, bool backup);

  /**
   * Removes and frees (and removes the files for) all clips not used by the
   * project or undo stacks.
   *
   * @param backup Whether to remove from backup directory.
   * @throw ZrythmException If any file could not be removed.
   */
  void remove_unused (bool backup);

  /**
   * Ensures that the name of the clip is unique.
   *
   * The clip must not be part of the pool yet.
   *
   * If the clip name is not unique, it will be replaced by a unique name.
   */
  void ensure_unique_clip_name (AudioClip &clip);

  /**
   * Generates a name for a recording clip.
   */
  static std::string gen_name_for_recording_clip (const Track &track, int lane);

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

  void print () const;

  void init_after_cloning (const AudioPool &other) override
  {
    clone_ptr_vector (clips_, other.clips_);
  }

  DECLARE_DEFINE_FIELDS_METHOD ();

private:
  bool name_exists (const std::string &name) const;

  /**
   * Returns the next available ID.
   */
  int get_next_id () const;

public:
  /**
   * Audio clips.
   *
   * @warning May contain NULLs.
   */
  std::vector<std::unique_ptr<AudioClip>> clips_;
};

/**
 * @}
 */

#endif
