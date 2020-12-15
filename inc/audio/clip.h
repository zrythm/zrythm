/*
 * Copyright (C) 2019-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Audio clip.
 */

#ifndef __AUDIO_CLIP_H__
#define __AUDIO_CLIP_H__

#include <stdbool.h>

#include "utils/types.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

/**
 * Audio clips for the pool.
 *
 * These should be loaded in the project's sample
 * rate.
 */
typedef struct AudioClip
{
  /** Name of the clip. */
  char *        name;

  /** The audio frames, interleaved. */
  sample_t *    frames;

  /** Number of frames per channel. */
  long          num_frames;

  /** Number of channels. */
  channels_t    channels;

  /**
   * BPM of the clip, or BPM of the project when
   * the clip was first loaded.
   */
  bpm_t         bpm;

  /**
   * Samplerate of the clip, or samplerate when
   * the clip was imported into the project.
   */
  int           samplerate;

  /** ID in the audio pool. */
  int           pool_id;

  /**
   * Frames already written to the file.
   *
   * Used when writing in chunks/parts.
   */
  long          frames_written;

  /**
   * Time the last write took place.
   *
   * This is used so that we can write every x
   * ms instead of all the time.
   *
   * @see AudioClip.frames_written.
   */
  gint64        last_write;
} AudioClip;

static const cyaml_schema_field_t
audio_clip_fields_schema[] =
{
  CYAML_FIELD_STRING_PTR (
    "name", CYAML_FLAG_POINTER,
    AudioClip, name,
    0, CYAML_UNLIMITED),
  YAML_FIELD_FLOAT (
    AudioClip, bpm),
  YAML_FIELD_INT (
    AudioClip, samplerate),
  YAML_FIELD_INT (
    AudioClip, pool_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t
audio_clip_schema = {
  CYAML_VALUE_MAPPING (CYAML_FLAG_POINTER,
    AudioClip, audio_clip_fields_schema),
};

/**
 * Inits after loading a Project.
 */
void
audio_clip_init_loaded (
  AudioClip * self);

/**
 * Creates an audio clip from a file.
 *
 * The name used is the basename of the file.
 */
AudioClip *
audio_clip_new_from_file (
  const char * full_path);

/**
 * Creates an audio clip by copying the given float
 * array.
 *
 * @param arr Interleaved array.
 * @param name A name for this clip.
 */
AudioClip *
audio_clip_new_from_float_array (
  const float *    arr,
  const long       nframes,
  const channels_t channels,
  const char *     name);

/**
 * Create an audio clip while recording.
 *
 * The frames will keep getting reallocated until
 * the recording is finished.
 *
 * @param nframes Number of frames to allocate. This
 *   should be the current cycle's frames when
 *   called during recording.
 */
AudioClip *
audio_clip_new_recording (
  const channels_t channels,
  const long       nframes,
  const char *     name);

/**
 * Writes the given audio clip data to a file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 *
 * @return Non-zero if fail.
 */
int
audio_clip_write_to_file (
  AudioClip *  self,
  const char * filepath,
  bool         parts);

/**
 * Writes the clip to the pool as a wav file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 */
void
audio_clip_write_to_pool (
  AudioClip * self,
  bool        parts);

/**
 * To be called by audio_pool_remove_clip().
 *
 * Removes the file associated with the clip and
 * frees the instance.
 */
void
audio_clip_remove_and_free (
  AudioClip * self);

/**
 * Frees the audio clip.
 */
void
audio_clip_free (
  AudioClip * self);

/**
 * @}
 */

#endif
