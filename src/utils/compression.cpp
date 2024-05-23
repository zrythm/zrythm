// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/compression.h"

#include <zstd.h>

typedef enum
{
  Z_UTILS_COMPRESSION_ERROR_FAILED,
} ZUtilsCompressionError;

#define Z_UTILS_COMPRESSION_ERROR z_utils_compression_error_quark ()
GQuark
z_utils_compression_error_quark (void);
G_DEFINE_QUARK (z - utils - compression - error - quark, z_utils_compression_error)

char *
compression_compress_to_base64_str (const char * src, GError ** error)
{
  size_t src_size = strlen (src);
  size_t compress_bound = ZSTD_compressBound (src_size);
  char * dest = static_cast<char *> (malloc (compress_bound));
  size_t dest_size = ZSTD_compress (dest, compress_bound, src, src_size, 1);
  if (ZSTD_isError (dest_size))
    {
      free (dest);

      g_set_error (
        error, Z_UTILS_COMPRESSION_ERROR, Z_UTILS_COMPRESSION_ERROR_FAILED,
        "Failed to compress: %s", ZSTD_getErrorName (dest_size));
      return NULL;
    }

  char * b64 = g_base64_encode ((const unsigned char *) dest, dest_size);

  free (dest);

  return b64;
}

char *
compression_decompress_from_base64_str (const char * b64, GError ** error)
{
  size_t src_size;
  char * src = (char *) g_base64_decode (b64, &src_size);
#if (ZSTD_VERSION_MAJOR == 1 && ZSTD_VERSION_MINOR < 3)
  unsigned long long const frame_content_size =
    ZSTD_getDecompressedSize (src, src_size);
  if (frame_content_size == 0)
#else
  unsigned long long const frame_content_size =
    ZSTD_getFrameContentSize (src, src_size);
  if (frame_content_size == ZSTD_CONTENTSIZE_ERROR)
#endif
    {
      g_set_error_literal (
        error, Z_UTILS_COMPRESSION_ERROR, Z_UTILS_COMPRESSION_ERROR_FAILED,
        "String not compressed by zstd");
      return NULL;
    }
  char * dest = static_cast<char *> (malloc ((size_t) frame_content_size));
  size_t dest_size = ZSTD_decompress (dest, frame_content_size, src, src_size);
  if (ZSTD_isError (dest_size))
    {
      free (dest);

      g_set_error (
        error, Z_UTILS_COMPRESSION_ERROR, Z_UTILS_COMPRESSION_ERROR_FAILED,
        "Failed to decompress string: %s", ZSTD_getErrorName (dest_size));
      return NULL;
    }
  if (dest_size != frame_content_size)
    {
      free (dest);

      /* impossible because zstd will check this condition */
      g_set_error_literal (
        error, Z_UTILS_COMPRESSION_ERROR, Z_UTILS_COMPRESSION_ERROR_FAILED,
        "uncompressed_size != frame_content_size");
      return NULL;
    }

  /* make string null-terminated */
  dest = static_cast<char *> (g_realloc (dest, dest_size + sizeof (char)));
  dest[dest_size] = '\0';

  return dest;
}
