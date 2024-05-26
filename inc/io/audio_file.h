// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio file manipulation.
 */

#ifndef __IO_AUDIO_FILE_H__
#define __IO_AUDIO_FILE_H__

#include "zrythm-config.h"

#include "utils/types.h"

#include <glib.h>

/**
 * @addtogroup io
 *
 * @{
 */

typedef struct AudioFileMetadata
{
  int samplerate;
  int channels;

  /** Milliseconds. */
  int64_t length;

  /** Total number of frames (eg. a frame for 16bit stereo is
   * 4 bytes. */
  int64_t num_frames;

  int bit_rate;
  int bit_depth;

  /** BPM if detected, or 0. */
  float bpm;

  /** Whether metadata are already filled (valid). */
  bool filled;
} AudioFileMetadata;

/**
 * Audio file struct.
 *
 * Must only be created with audio_file_new().
 */
typedef struct AudioFile
{
  /** Absolute path. */
  char * filepath;

  AudioFileMetadata metadata;

  /** Implemented in the source file. */
  void * internal_data;
} AudioFile;

/**
 * Creates a new instance of an AudioFile for the given path.
 *
 * This may be a file to read from or a file to write to.
 */
NONNULL AudioFile *
audio_file_new (const char * filepath);

/**
 * Reads the metadata for the given file.
 */
NONNULL_ARGS (1)
bool audio_file_read_metadata (AudioFile * self, GError ** error);

/**
 * Reads the file into an internal float array (interleaved).
 *
 * @param samples Samples to fill in.
 * @param in_parts Whether to read the file in parts. If true,
 *   @p start_from and @p num_frames_to_read must be specified.
 * @param samples[out] Pre-allocated frame array. Caller must
 *   ensure there is enough space (ie, number of frames *
 *   number of channels).
 */
NONNULL_ARGS (1)
bool audio_file_read_samples (
  AudioFile * self,
  bool        in_parts,
  float *     samples,
  size_t      start_from,
  size_t      num_frames_to_read,
  GError **   error);

/**
 * Must be called when done reading or writing files (or when
 * the operation was cancelled).
 *
 * This is not needed when only reading metadata.
 */
NONNULL_ARGS (1) bool audio_file_finish (AudioFile * self, GError ** error);

/**
 * Simple blocking API for reading and optionally resampling
 * audio files.
 *
 * Only to be used on small files.
 *
 * @param[out] frames Pointer to store newly allocated
 *   interlaved frames to.
 * @param[out] metadata File metadata will be pasted here if
 *   non-NULL.
 * @param samplerate If specified, the audio will be resampled
 *   to the given samplerate. Pass 0 to avoid resampling.
 */
bool
audio_file_read_simple (
  const char *        filepath,
  float **            frames,
  size_t *            num_frames,
  AudioFileMetadata * metadata,
  size_t              samplerate,
  GError **           error);

NONNULL void
audio_file_free (AudioFile * self);

/**
 * @}
 */

#endif
