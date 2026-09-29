// SPDX-FileCopyrightText: © 2023-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/base64.h"
#include "utils/compression.h"
#include "utils/exceptions.h"
#include "utils/mem.h"

#include <zstd.h>

using zrythm::utils::ZrythmException;

namespace zrythm::utils::compression
{

QByteArray
compress_to_base64_str (const QByteArray &src)
{
  size_t compress_bound = ZSTD_compressBound (static_cast<size_t> (src.size ()));
  char * dest = static_cast<char *> (malloc (compress_bound));
  if (dest == nullptr)
    {
      throw ZrythmException (
        fmt::format (
          "Failed to allocate {} bytes for compression", compress_bound));
    }
  size_t dest_size = ZSTD_compress (
    dest, compress_bound, src.constData (), static_cast<size_t> (src.size ()),
    1);
  if (ZSTD_isError (dest_size))
    {
      free (dest);

      throw ZrythmException (
        fmt::format ("Failed to compress: {}", ZSTD_getErrorName (dest_size)));
    }

  // The buffer is freed on any throw from here on (e.g. bad_alloc below)
  CStringRAII dest_guard{ dest };

  return utils::base64::encode (
    QByteArray (dest_guard.c_str (), static_cast<qsizetype> (dest_size)));
}

CStringRAII
decompress_string_from_base64 (const QByteArray &b64, size_t max_output_size)
{
  auto src = utils::base64::decode (b64);
#if (ZSTD_VERSION_MAJOR == 1 && ZSTD_VERSION_MINOR < 3)
  unsigned long long const frame_content_size =
    ZSTD_getDecompressedSize (src.constData (), src.size ());
  if (frame_content_size == 0)
#else
  unsigned long long const frame_content_size = ZSTD_getFrameContentSize (
    src.constData (), static_cast<size_t> (src.size ()));
  if (frame_content_size == ZSTD_CONTENTSIZE_ERROR)
#endif
    {
      throw ZrythmException ("String not compressed by zstd");
    }
  if (frame_content_size > max_output_size)
    {
      throw ZrythmException (
        fmt::format (
          "Decompressed size {} exceeds the maximum allowed {}",
          frame_content_size, max_output_size));
    }
  // +1 so the result can be null-terminated without a realloc
  auto dest = static_cast<char *> (malloc ((size_t) frame_content_size + 1));
  if (dest == nullptr)
    {
      throw ZrythmException (
        fmt::format (
          "Failed to allocate {} bytes for decompression", frame_content_size));
    }
  // the buffer is freed on any throw from here on
  CStringRAII dest_guard{ dest };
  size_t      dest_size = ZSTD_decompress (
    dest, frame_content_size, src.constData (),
    static_cast<size_t> (src.size ()));
  if (ZSTD_isError (dest_size))
    {
      throw ZrythmException (
        fmt::format (
          "Failed to decompress string: {}", ZSTD_getErrorName (dest_size)));
    }
  if (dest_size != frame_content_size)
    {
      /* impossible because zstd will check this condition */
      throw ZrythmException ("uncompressed_size != frame_content_size");
    }

  /* make string null-terminated */
  dest[dest_size] = '\0';

  return dest_guard;
}

} // namespace zrythm::utils::compression
