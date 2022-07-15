/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Audio file pool.
 */

#ifndef __AUDIO_POOL_H__
#define __AUDIO_POOL_H__

#include "audio/clip.h"
#include "utils/yaml.h"

typedef struct Track Track;

/**
 * @addtogroup audio
 *
 * @{
 */

#define AUDIO_POOL_SCHEMA_VERSION 1

#define AUDIO_POOL (AUDIO_ENGINE->pool)

/**
 * An audio pool is a pool of audio files and their
 * corresponding float arrays in memory that are
 * referenced by regions.
 *
 * Instead of associating audio files with regions,
 * all audio files (and their edited counterparts
 * after some hard editing like stretching) are saved
 * in the pool.
 */
typedef struct AudioPool
{
  int schema_version;

  /**
   * Audio clips.
   *
   * May contain NULLs.
   */
  AudioClip ** clips;

  /**
   * Clip counter.
   *
   * This is not the actual number of clips in the
   * pool - it may contain NULL clips. This is to
   * make identifying and managing clips easier
   * by their IDs.
   */
  int num_clips;

  /** Array sizes. */
  size_t clips_size;
} AudioPool;

static const cyaml_schema_field_t audio_pool_fields_schema[] = {
  YAML_FIELD_INT (AudioPool, schema_version),
  YAML_FIELD_DYN_ARRAY_VAR_COUNT (
    AudioPool,
    clips,
    audio_clip_schema),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_pool_schema = {
  YAML_VALUE_PTR (AudioPool, audio_pool_fields_schema),
};

/**
 * Inits after loading a project.
 */
void
audio_pool_init_loaded (AudioPool * self);

/**
 * Creates a new audio pool.
 */
AudioPool *
audio_pool_new (void);

/**
 * Adds an audio clip to the pool.
 *
 * Changes the name of the clip if another clip with
 * the same name already exists.
 *
 * @return The ID in the pool.
 */
int
audio_pool_add_clip (AudioPool * self, AudioClip * clip);

/**
 * Duplicates the clip with the given ID and returns
 * the duplicate.
 *
 * @param write_file Whether to also write the file.
 *
 * @return The ID in the pool.
 */
int
audio_pool_duplicate_clip (
  AudioPool * self,
  int         clip_id,
  bool        write_file);

/**
 * Returns the clip for the given ID.
 */
AudioClip *
audio_pool_get_clip (AudioPool * self, int clip_id);

/**
 * Removes the clip with the given ID from the pool
 * and optionally frees it (and removes the file).
 *
 * @param backup Whether to remove from backup
 *   directory.
 */
void
audio_pool_remove_clip (
  AudioPool * self,
  int         clip_id,
  bool        free_and_remove_file,
  bool        backup);

/**
 * Removes and frees (and removes the files for) all
 * clips not used by the project or undo stacks.
 *
 * @param backup Whether to remove from backup
 *   directory.
 */
void
audio_pool_remove_unused (AudioPool * self, bool backup);

/**
 * Ensures that the name of the clip is unique.
 *
 * The clip must not be part of the pool yet.
 *
 * If the clip name is not unique, it will be
 * replaced by a unique name.
 */
void
audio_pool_ensure_unique_clip_name (
  AudioPool * pool,
  AudioClip * clip);

/**
 * Generates a name for a recording clip.
 */
MALLOC
char *
audio_pool_gen_name_for_recording_clip (
  AudioPool * pool,
  Track *     track,
  int         lane);

/**
 * Loads the frame buffers of clips currently in
 * use in the project from their files and frees the
 * buffers of clips not currently in use.
 *
 * This should be called whenever there is a relevant
 * change in the project (eg, object added/removed).
 */
void
audio_pool_reload_clip_frame_bufs (AudioPool * self);

/**
 * Writes all the clips to disk.
 *
 * Used when saving a project elsewhere.
 *
 * @param is_backup Whether this is a backup project.
 */
void
audio_pool_write_to_disk (AudioPool * self, bool is_backup);

/**
 * To be used during serialization.
 */
AudioPool *
audio_pool_clone (const AudioPool * src);

void
audio_pool_print (const AudioPool * const self);

void
audio_pool_free (AudioPool * self);

/**
 * @}
 */

#endif
