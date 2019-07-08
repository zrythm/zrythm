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

#ifndef __UTILS_AUDIO_H__
#define __UTILS_AUDIO_H__

#include <samplerate.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Number of plugin slots per channel.
 */
#define STRIP_SIZE 9


struct adinfo;

/**
 * Decodes the given filename (absolute path).
 */
void
audio_decode (
  struct adinfo * nfo,
  SRC_DATA *      src_data,
  float **        out_buff,
  long *          out_buff_size,
  const char *    filename);

/**
 * Writes the buffer as a raw file to the given
 * path.
 */
void
audio_write_raw_file (
  float * buff,
  long    size,
  int     samplerate,
  int     channels,
  const char * filename);

/**
 * Returns the number of CPU cores.
 */
int
audio_get_num_cores ();

/**
 * @}
 */

#endif
