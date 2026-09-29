// SPDX-FileCopyrightText: © 2023-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/utf8_string.h"

/**
 * @brief Compression utilities.
 */
namespace zrythm::utils::compression
{

/** Default maximum decompressed size accepted (1 GiB, enough for project
 * files); larger frames are rejected before any allocation. */
inline constexpr size_t kDefaultMaxDecompressedSize = 1024ULL * 1024 * 1024;

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
 * @param max_output_size Rejects frames declaring a larger decompressed
 *   size than this, before any allocation (guards against decompression
 *   bombs from untrusted input).
 *
 * @throw ZrythmException on error.
 */
CStringRAII
decompress_string_from_base64 (
  const QByteArray &b64,
  size_t            max_output_size = kDefaultMaxDecompressedSize);

}; // namespace zrythm::utils::compression
