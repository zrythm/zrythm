// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * Copyright 2007-2021 David Robillard <d@drobilla.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * SPDX-License-Identifier: ISC
 *
 * ---
 */

#include "zrythm-config.h"

#include <cstdlib>
#include <cstring>

#include <errno.h>

#include "utils/exceptions.h"
#ifdef __linux__
#  include <sys/ioctl.h>
#  include <sys/stat.h>
#  include <sys/types.h>

#  include <fcntl.h>
#  include <linux/fs.h>
#endif

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#endif

#include "utils/file.h"

#include <glib.h>
#include <glib/gstdio.h>

#include <glibmm.h>

bool
file_path_exists (const std::string &path)
{
  return Glib::file_test (path, Glib::FileTest::EXISTS);
}

bool
file_dir_exists (const std::string &dir_path)
{
  return Glib::file_test (dir_path, Glib::FileTest::IS_DIR);
}

char *
file_path_relative_to (const char * path, const char * base)
{
  const size_t path_len = strlen (path);
  const size_t base_len = strlen (base);
  const size_t min_len = (path_len < base_len) ? path_len : base_len;

  // Find the last separator common to both paths
  size_t last_shared_sep = 0;
  for (size_t i = 0; i < min_len && path[i] == base[i]; ++i)
    {
      if (path[i] == G_DIR_SEPARATOR)
        {
          last_shared_sep = i;
        }
    }

  if (last_shared_sep == 0)
    {
      // No common components, return path
      return g_strdup (path);
    }

  // Find the number of up references ("..") required
  size_t up = 0;
  for (size_t i = last_shared_sep + 1; i < base_len; ++i)
    {
      if (base[i] == G_DIR_SEPARATOR)
        {
          ++up;
        }
    }

#ifdef _WIN32
  const bool use_slash = strchr (path, '/');
#else
  static const bool use_slash = true;
#endif

  // Write up references
  const size_t suffix_len = path_len - last_shared_sep;
  char * rel = static_cast<char *> (g_malloc0_n (1, suffix_len + (up * 3) + 1));
  for (size_t i = 0; i < up; ++i)
    {
      if (use_slash)
        {
          memcpy (rel + (i * 3), "../", 3);
        }
      else
        {
          memcpy (rel + (i * 3), "..\\", 3);
        }
    }

  // Write suffix
  memcpy (rel + (up * 3), path + last_shared_sep + 1, suffix_len);
  return rel;
}

int
file_symlink (const char * old_path, const char * new_path)
{
  int ret = 0;
#ifdef _WIN32
  ret = !CreateHardLink ((LPCSTR) new_path, (LPCSTR) old_path, 0);
#else
  char * target = file_path_relative_to (old_path, new_path);
  ret = symlink (target, new_path);
  g_free (target);
#endif

  return ret;
}

void
file_reflink (const std::string &dest, const std::string &src)
{
#ifdef __linux__
  int src_fd = g_open (src.c_str (), O_RDONLY);
  if (src_fd == -1)
    throw ZrythmException ("Failed to open source file");
  int dest_fd = g_open (dest.c_str (), O_RDWR | O_CREAT, 0644);
  if (dest_fd == -1)
    throw ZrythmException ("Failed to open destination file");
  if (ioctl (dest_fd, FICLONE, src_fd) != 0)
    throw ZrythmException ("Failed to reflink");
#else
  throw ZrythmException ("Reflink not supported on this platform");
#endif
}
