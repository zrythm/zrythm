// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <stdlib.h>
#include <string.h>

#include "utils/mem.h"

#include <gtk/gtk.h>

/**
 * Reallocate and zero out newly added memory.
 */
void *
realloc_zero (void * buf, size_t old_sz, size_t new_sz)
{
  /*g_message ("realloc %zu to %zu", old_sz, new_sz);*/
  void * new_buf = g_realloc (buf, new_sz);
  if (new_sz > old_sz && new_buf)
    {
      size_t diff = new_sz - old_sz;
      void * pStart = ((char *) new_buf) + old_sz;
      memset (pStart, 0, diff);
    }
  return new_buf;
}
