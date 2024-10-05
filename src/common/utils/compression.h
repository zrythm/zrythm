// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Compression utilities.
 */

#ifndef __UTILS_COMPRESSION_H__
#define __UTILS_COMPRESSION_H__

#include <glib.h>

#include "common/utils/types.h"

/**
 * Compresses a NULL-terminated string.
 */
char *
compression_compress_to_base64_str (const char * src, GError ** error);

/**
 * Decompresses a NULL-terminated string.
 */
char *
compression_decompress_from_base64_str (const char * b64, GError ** error);

#endif // __UTILS_COMPRESSION_H__
