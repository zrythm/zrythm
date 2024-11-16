// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cstdlib>
#include <cstring>

#include "common/utils/mem.h"

/**
 * Reallocate and zero out newly added memory.
 */
void *
realloc_zero (void * buf, size_t old_sz, size_t new_sz)
{
  /*z_info ("realloc {} to {}", old_sz, new_sz);*/
  void * new_buf = z_realloc (buf, new_sz);
  if (new_sz > old_sz && new_buf)
    {
      size_t diff = new_sz - old_sz;
      void * pStart = ((char *) new_buf) + old_sz;
      memset (pStart, 0, diff);
    }
  return new_buf;
}

void *
z_realloc (void * ptr, size_t size)
{
  if (!ptr)
    {
      return calloc (1, size);
    }
  return realloc (ptr, size);
}

void
z_free_strv (char ** strv)
{
  if (!strv)
    return;

  for (char ** p = strv; *p != NULL; p++)
    {
      free (*p);
    }
  free (strv);
}