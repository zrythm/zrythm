/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * @addtogroup audio
 *
 * @{
 */

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
  /** The clips. */
  AudioClip **   clips;

  /** Clip counter. */
  int            num_clips;

  /** Array sizes. */
  size_t         clips_size;
} AudioPool;

static const cyaml_schema_field_t
audio_pool_fields_schema[] =
{
  CYAML_FIELD_SEQUENCE_COUNT (
    "clips",
    CYAML_FLAG_POINTER | CYAML_FLAG_OPTIONAL,
    AudioPool, clips, num_clips,
    &audio_clip_schema, 0, CYAML_UNLIMITED),

	CYAML_FIELD_END
};

static const cyaml_schema_value_t
audio_pool_schema =
{
	CYAML_VALUE_MAPPING (
    CYAML_FLAG_POINTER,
    AudioPool, audio_pool_fields_schema),
};

/**
 * Inits after loading a project.
 */
void
audio_pool_init_loaded (
  AudioPool * self);

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
audio_pool_add_clip (
  AudioPool * self,
  AudioClip * clip);

void
audio_pool_free (
  AudioPool * self);

/**
 * @}
 */

#endif
