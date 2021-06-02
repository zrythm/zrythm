/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
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
 */

#include "zrythm-config.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WOE32
#include <windows.h>
#else
#include <unistd.h>
#endif

#include "utils/file.h"

#include <glib.h>

char *
file_path_relative_to (
  const char * path,
  const char * base)
{
  const size_t path_len = strlen(path);
  const size_t base_len = strlen(base);
  const size_t min_len  = (path_len < base_len) ? path_len : base_len;

  // Find the last separator common to both paths
  size_t last_shared_sep = 0;
  for (size_t i = 0; i < min_len && path[i] == base[i]; ++i) {
    if (path[i] == G_DIR_SEPARATOR) {
      last_shared_sep = i;
    }
  }

  if (last_shared_sep == 0) {
    // No common components, return path
    return g_strdup(path);
  }

  // Find the number of up references ("..") required
  size_t up = 0;
  for (size_t i = last_shared_sep + 1; i < base_len; ++i) {
    if (base[i] == G_DIR_SEPARATOR) {
      ++up;
    }
  }

#ifdef _WOE32
  const bool use_slash = strchr(path, '/');
#else
  static const bool use_slash = true;
#endif

  // Write up references
  const size_t suffix_len = path_len - last_shared_sep;
  char * rel =
    g_malloc0_n (1, suffix_len + (up * 3) + 1);
  for (size_t i = 0; i < up; ++i) {
    if (use_slash) {
      memcpy(rel + (i * 3), "../", 3);
    } else {
      memcpy(rel + (i * 3), "..\\", 3);
    }
  }

  // Write suffix
  memcpy(rel + (up * 3), path + last_shared_sep + 1, suffix_len);
  return rel;
}

int
file_symlink (
  const char * old_path,
  const char * new_path)
{
  int ret = 0;
#ifdef _WOE32
  ret =
    !CreateHardLink (
      new_path, old_path, 0);
#else
  char * target =
    file_path_relative_to (
      old_path, new_path);
  ret = symlink (target, new_path);
#endif

  return ret;
}
