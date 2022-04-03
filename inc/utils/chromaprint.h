/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Chromaprint utils.
 */

#ifndef __UTILS_CHROMAPRINT_H__
#define __UTILS_CHROMAPRINT_H__

#include "zrythm-config.h"

#ifdef HAVE_CHROMAPRINT

#  include <stdarg.h>
#  include <stdbool.h>

#  include "utils/types.h"
#  include "utils/yaml.h"

#  include <chromaprint.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Chroma fingerprint info for a specific file.
 */
typedef struct ChromaprintFingerprint
{
  uint32_t * fp;
  int        size;
  char *     compressed_str;
} ChromaprintFingerprint;

void
z_chromaprint_fingerprint_free (
  ChromaprintFingerprint * self);

ChromaprintFingerprint *
z_chromaprint_get_fingerprint (
  const char *     file1,
  unsigned_frame_t max_frames);

/**
 * @param perc Minimum percentage of equal
 *   fingerprints required.
 */
void
z_chromaprint_check_fingerprint_similarity (
  const char * file1,
  const char * file2,
  int          perc,
  int          expected_size);

/**
 * @}
 */

#endif /* HAVE_CHROMAPRINT */

#endif /* header guard */
