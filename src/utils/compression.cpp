// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/base64.h"
#include "utils/compression.h"
#include "utils/exceptions.h"
#include "utils/mem.h"
#include <zstd.h>

namespace zrythm::utils::compression
{

QByteArray
compress_to_base64_str (const QByteArray &src)
{
  size_t compress_bound = ZSTD_compressBound (src.size ());
  char * dest = static_cast<char *> (malloc (compress_bound));
  size_t dest_size =
    ZSTD_compress (dest, compress_bound, src.constData (), src.size (), 1);
  if (ZSTD_isError (dest_size))
    {
      free (dest);

      throw ZrythmException (
        fmt::format ("Failed to compress: {}", ZSTD_getErrorName (dest_size)));
    }

  auto ret = utils::base64::encode (QByteArray (dest, dest_size));

  free (dest);

  return ret;
}

string::CStringRAII
decompress_string_from_base64 (const QByteArray &b64)
{
  auto src = utils::base64::decode (b64);
#if (ZSTD_VERSION_MAJOR == 1 && ZSTD_VERSION_MINOR < 3)
  unsigned long long const frame_content_size =
    ZSTD_getDecompressedSize (src.constData (), src.size ());
  if (frame_content_size == 0)
#else
  unsigned long long const frame_content_size =
    ZSTD_getFrameContentSize (src.constData (), src.size ());
  if (frame_content_size == ZSTD_CONTENTSIZE_ERROR)
#endif
    {
      throw ZrythmException ("String not compressed by zstd");
    }
  auto   dest = static_cast<char *> (malloc ((size_t) frame_content_size));
  size_t dest_size =
    ZSTD_decompress (dest, frame_content_size, src.constData (), src.size ());
  if (ZSTD_isError (dest_size))
    {
      free (dest);

      throw ZrythmException (fmt::format (
        "Failed to decompress string: {}", ZSTD_getErrorName (dest_size)));
    }
  if (dest_size != frame_content_size)
    {
      free (dest);

      /* impossible because zstd will check this condition */
      throw ZrythmException ("uncompressed_size != frame_content_size");
    }

  /* make string null-terminated */
  dest = static_cast<char *> (z_realloc (dest, dest_size + sizeof (char)));
  dest[dest_size] = '\0';

  return { dest };
}

};
