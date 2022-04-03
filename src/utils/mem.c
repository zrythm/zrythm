/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm.org>
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
 */

#include <stdlib.h>

#include "utils/mem.h"

#include <gtk/gtk.h>

#include <string.h>

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
