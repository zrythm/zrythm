// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio clip.
 */

#ifndef __AUDIO_CLIP_H__
#define __AUDIO_CLIP_H__

#include <stdbool.h>

#include "utils/audio.h"
#include "utils/types.h"
#include "utils/yaml.h"

/**
 * @addtogroup audio
 *
 * @{
 */

#define AUDIO_CLIP_SCHEMA_VERSION 1

/**
 * Audio clips for the pool.
 *
 * These should be loaded in the project's sample
 * rate.
 */
typedef struct AudioClip
{
  int schema_version;

  /** Name of the clip. */
  char * name;

  /** The audio frames, interleaved. */
  sample_t * frames;

  /** Number of frames per channel. */
  unsigned_frame_t num_frames;

  /**
   * Per-channel frames for convenience.
   */
  sample_t * ch_frames[16];

  /** Number of channels. */
  channels_t channels;

  /**
   * BPM of the clip, or BPM of the project when
   * the clip was first loaded.
   */
  bpm_t bpm;

  /**
   * Samplerate of the clip, or samplerate when
   * the clip was imported into the project.
   */
  int samplerate;

  /**
   * Bit depth of the clip when the clip was
   * imported into the project.
   */
  BitDepth bit_depth;

  /** Whether the clip should use FLAC when being
   * serialized. */
  bool use_flac;

  /** ID in the audio pool. */
  int pool_id;

  /** File hash, used for checking if a clip is
   * already written to the pool. */
  char * file_hash;

  /**
   * Frames already written to the file, per channel.
   *
   * Used when writing in chunks/parts.
   */
  unsigned_frame_t frames_written;

  /**
   * Time the last write took place.
   *
   * This is used so that we can write every x
   * ms instead of all the time.
   *
   * @see AudioClip.frames_written.
   */
  gint64 last_write;
} AudioClip;

static const cyaml_schema_field_t audio_clip_fields_schema[] = {
  YAML_FIELD_INT (AudioClip, schema_version),
  YAML_FIELD_STRING_PTR (AudioClip, name),
  YAML_FIELD_STRING_PTR_OPTIONAL (AudioClip, file_hash),
  YAML_FIELD_FLOAT (AudioClip, bpm),
  YAML_FIELD_ENUM (AudioClip, bit_depth, bit_depth_strings),
  YAML_FIELD_INT (AudioClip, use_flac),
  YAML_FIELD_INT (AudioClip, samplerate),
  YAML_FIELD_INT (AudioClip, pool_id),

  CYAML_FIELD_END
};

static const cyaml_schema_value_t audio_clip_schema = {
  YAML_VALUE_PTR_NULLABLE (AudioClip, audio_clip_fields_schema),
};

static inline bool
audio_clip_use_flac (BitDepth bd)
{
  return false;
  /* FLAC seems to fail writing sometimes so disable for now */
#if 0
  return bd < BIT_DEPTH_32;
#endif
}

/**
 * Inits after loading a Project.
 */
COLD NONNULL void
audio_clip_init_loaded (AudioClip * self);

/**
 * Creates an audio clip from a file.
 *
 * The name used is the basename of the file.
 */
NONNULL AudioClip *
audio_clip_new_from_file (const char * full_path);

/**
 * Creates an audio clip by copying the given float
 * array.
 *
 * @param arr Interleaved array.
 * @param name A name for this clip.
 */
AudioClip *
audio_clip_new_from_float_array (
  const float *          arr,
  const unsigned_frame_t nframes,
  const channels_t       channels,
  BitDepth               bit_depth,
  const char *           name);

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
  const channels_t       channels,
  const unsigned_frame_t nframes,
  const char *           name);

/**
 * Updates the channel caches.
 *
 * See @ref AudioClip.ch_frames.
 *
 * @param start_from Frames to start from (per
 *   channel. The previous frames will be kept.
 */
NONNULL void
audio_clip_update_channel_caches (
  AudioClip * self,
  size_t      start_from);

/**
 * Shows a dialog with info on how to edit a file,
 * with an option to open an app launcher.
 *
 * When the user closes the dialog, the clip is
 * assumed to have been edited.
 *
 * The given audio clip will be free'd.
 *
 * @note This must not be used on pool clips.
 *
 * @return A new instance of AudioClip if successful,
 *   NULL, if not.
 */
NONNULL AudioClip *
audio_clip_edit_in_ext_program (
  AudioClip * self,
  GError **   error);

/**
 * Writes the given audio clip data to a file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 *
 * @return Whether successful.
 */
WARN_UNUSED_RESULT NONNULL bool
audio_clip_write_to_file (
  AudioClip *  self,
  const char * filepath,
  bool         parts,
  GError **    error);

/**
 * Writes the clip to the pool as a wav file.
 *
 * @param parts If true, only write new data. @see
 *   AudioClip.frames_written.
 * @param is_backup Whether writing to a backup
 *   project.
 *
 * @return Whether successful.
 */
WARN_UNUSED_RESULT NONNULL bool
audio_clip_write_to_pool (
  AudioClip * self,
  bool        parts,
  bool        is_backup,
  GError **   error);

/**
 * Gets the path of a clip matching \ref name from
 * the pool.
 *
 * @param use_flac Whether to look for a FLAC file
 *   instead of a wav file.
 * @param is_backup Whether writing to a backup
 *   project.
 */
NONNULL char *
audio_clip_get_path_in_pool_from_name (
  const char * name,
  bool         use_flac,
  bool         is_backup);

/**
 * Gets the path of the given clip from the pool.
 *
 * @param is_backup Whether writing to a backup
 *   project.
 */
NONNULL char *
audio_clip_get_path_in_pool (AudioClip * self, bool is_backup);

/**
 * Returns whether the clip is used inside the
 * project.
 *
 * @param check_undo_stack If true, this checks both
 *   project regions and the undo stack. If false,
 *   this only checks actual project regions only.
 */
NONNULL bool
audio_clip_is_in_use (AudioClip * self, bool check_undo_stack);

/**
 * To be called by audio_pool_remove_clip().
 *
 * Removes the file associated with the clip and
 * frees the instance.
 *
 * @param backup Whether to remove from backup
 *   directory.
 */
NONNULL void
audio_clip_remove_and_free (AudioClip * self, bool backup);

NONNULL AudioClip *
audio_clip_clone (AudioClip * src);

/**
 * Frees the audio clip.
 */
NONNULL void
audio_clip_free (AudioClip * self);

/**
 * @}
 */

#endif
