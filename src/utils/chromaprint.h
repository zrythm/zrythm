/*
 * SPDX-FileCopyrightText: © 2020-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Chromaprint utils.
 */

#ifndef __UTILS_CHROMAPRINT_H__
#define __UTILS_CHROMAPRINT_H__

#include "zrythm-config.h"

#if HAVE_CHROMAPRINT

#  include "utils/types.h"

#  include <chromaprint.h>

/**
 * @addtogroup utils
 *
 * @{
 */

/**
 * Chroma fingerprint info for a specific file.
 */
struct ChromaprintFingerprint
{
  ChromaprintFingerprint () noexcept = default;
  ~ChromaprintFingerprint ()
  {
    if (fp)
      chromaprint_dealloc (fp);
    if (compressed_str)
      chromaprint_dealloc (compressed_str);
  }
  uint32_t * fp;
  int        size;
  char *     compressed_str;
};

std::unique_ptr<ChromaprintFingerprint>
z_chromaprint_get_fingerprint (const char * file1, unsigned_frame_t max_frames);

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
