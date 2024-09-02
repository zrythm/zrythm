// SPDX-FileCopyrightText: © 2019-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Audio utils.
 */

#ifndef __UTILS_AUDIO_H__
#define __UTILS_AUDIO_H__

#include <cstdarg>

#include "utils/format.h"
#include "utils/logger.h"
#include "utils/types.h"

#include <glib/gi18n.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Bit depth.
 */
enum class BitDepth
{
  BIT_DEPTH_16,
  BIT_DEPTH_24,
  BIT_DEPTH_32
};

static inline int
audio_bit_depth_enum_to_int (BitDepth depth)
{
  switch (depth)
    {
    case BitDepth::BIT_DEPTH_16:
      return 16;
    case BitDepth::BIT_DEPTH_24:
      return 24;
    case BitDepth::BIT_DEPTH_32:
      return 32;
    default:
      z_return_val_if_reached (-1);
    }
}

static inline BitDepth
audio_bit_depth_int_to_enum (int depth)
{
  switch (depth)
    {
    case 16:
      return BitDepth::BIT_DEPTH_16;
    case 24:
      return BitDepth::BIT_DEPTH_24;
    case 32:
      return BitDepth::BIT_DEPTH_32;
    default:
      z_return_val_if_reached (BitDepth::BIT_DEPTH_16);
    }
}

DEFINE_ENUM_FORMATTER (
  BitDepth,
  BitDepth,
  N_ ("16 bit"),
  N_ ("24 bit"),
  N_ ("32 bit"));

/**
 * Number of plugin slots per channel.
 */
constexpr size_t STRIP_SIZE = 9;

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
  const float * src1,
  const float * src2,
  size_t        num_frames,
  float         epsilon);

/**
 * Returns whether the file contents are equal.
 *
 * @param num_frames Maximum number of frames to check. Passing
 *   0 will check all frames.
 */
bool
audio_files_equal (
  const char * f1,
  const char * f2,
  size_t       num_frames,
  float        epsilon);

/**
 * Returns whether the frame buffer is empty (zero).
 */
bool
audio_frames_empty (const float * src, size_t num_frames);

/**
 * Detect BPM.
 *
 * @return The BPM, or 0 if not found.
 */
float
audio_detect_bpm (
  const float *       src,
  size_t              num_frames,
  unsigned int        samplerate,
  std::vector<float> &candidates);

bool
audio_file_is_silent (const char * filepath);

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores ();

/**
 * @}
 */

#endif
