// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __UTILS_COMPRESSION_H__
#define __UTILS_COMPRESSION_H__

#include "utils/types.h"
#include "utils/utf8_string.h"

/**
 * @brief Compression utilities.
 */
namespace zrythm::utils::compression
{

/**
 * Compresses a NULL-terminated string.
 *
 * @throw ZrythmException on error.
 */
QByteArray
compress_to_base64_str (const QByteArray &src);

/**
 * Decompresses a NULL-terminated string.
 *
 * @throw ZrythmException on error.
 */
CStringRAII
decompress_string_from_base64 (const QByteArray &b64);

}; // namespace zrythm::utils::compression

#endif // __UTILS_COMPRESSION_H__
