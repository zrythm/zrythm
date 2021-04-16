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
 * Audio utils.
 */

#ifndef __UTILS_AUDIO_H__
#define __UTILS_AUDIO_H__

#include <stdarg.h>
#include <stdbool.h>

#include "utils/types.h"

#include <audec/audec.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Number of plugin slots per channel.
 */
#define STRIP_SIZE 9

void
audio_audec_log_func (
  AudecLogLevel level,
  const char *  fmt,
  va_list       args);

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
  long         frames_already_written,
  long         nframes,
  uint32_t     samplerate,
  unsigned int channels,
  const char * filename);

/**
 * Returns whether the frame buffers are equal.
 */
bool
audio_frames_equal (
  float * src1,
  float * src2,
  size_t  num_frames);

/**
 * Returns whether the frame buffer is empty (zero).
 */
bool
audio_frames_empty (
  float * src,
  size_t  num_frames);

bool
audio_file_is_silent (
  const char * filepath);

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores (void);

/**
 * @}
 */

#endif
