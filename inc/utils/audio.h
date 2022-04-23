// SPDX-FileCopyrightText: © 2019-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Audio utils.
 */

#ifndef __UTILS_AUDIO_H__
#define __UTILS_AUDIO_H__

#include <stdarg.h>
#include <stdbool.h>

#include "utils/types.h"
#include "utils/yaml.h"

#include <audec/audec.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Bit depth.
 */
typedef enum BitDepth
{
  BIT_DEPTH_16,
  BIT_DEPTH_24,
  BIT_DEPTH_32
} BitDepth;

static const cyaml_strval_t bit_depth_strings[] = {
  {"16",  BIT_DEPTH_16},
  { "24", BIT_DEPTH_24},
  { "32", BIT_DEPTH_32},
};

static inline int
audio_bit_depth_enum_to_int (BitDepth depth)
{
  switch (depth)
    {
    case BIT_DEPTH_16:
      return 16;
    case BIT_DEPTH_24:
      return 24;
    case BIT_DEPTH_32:
      return 32;
    default:
      g_return_val_if_reached (-1);
    }
}

/**
 * Number of plugin slots per channel.
 */
#define STRIP_SIZE 9

void
audio_audec_log_func (
  AudecLogLevel level,
  const char *  fmt,
  va_list       args) G_GNUC_PRINTF (2, 0);

/**
 * Writes the buffer as a raw file to the given
 * path.
 *
 * @param size The number of frames per channel.
 * @param samplerate The samplerate of \ref buff.
 * @param frames_already_written Frames (per
 *   channel)already written. If this is non-zero
 *   and the file exists, it will append to the
 *   existing file.
 *
 * @return Non-zero if fail.
 */
int
audio_write_raw_file (
  float *      buff,
  size_t       frames_already_written,
  size_t       nframes,
  uint32_t     samplerate,
  bool         flac,
  BitDepth     bit_depth,
  channels_t   channels,
  const char * filename);

/**
 * Returns the number of frames in the given audio
 * file.
 */
unsigned_frame_t
audio_get_num_frames (const char * filepath);

/**
 * Returns whether the frame buffers are equal.
 */
bool
audio_frames_equal (
  float * src1,
  float * src2,
  size_t  num_frames,
  float   epsilon);

/**
 * Returns whether the frame buffer is empty (zero).
 */
bool
audio_frames_empty (float * src, size_t num_frames);

/**
 * Detect BPM.
 *
 * @return The BPM, or 0 if not found.
 */
float
audio_detect_bpm (
  float *      src,
  size_t       num_frames,
  unsigned int samplerate,
  GArray *     candidates);

bool
audio_file_is_silent (const char * filepath);

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores (void);

/**
 * @}
 */

#endif
