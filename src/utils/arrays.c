/*
 * Copyright (C) 2018-2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Array helpers.
 */

#include <stdlib.h>
#include <string.h>

#include "utils/arrays.h"

/**
 * Returns 1 if element exists in array, 0 if not.
 *
 * TODO rename arrays to array
 */
int
_array_contains (void ** array,
                 int size,
                 void * element)
{
  for (int i = 0; i < size; i++)
    {
      if (array[i] == element)
        return 1;
    }
  return 0;
}

/**
 * Returns the index ofthe element exists in array,
 * -1 if not.
 */
int
_array_index_of (void ** array, int size, void * element)
{
  for (int i = 0; i < size; i++)
    {
      if (array[i] == element)
        return i;
    }
  return -1;
}

static int
alphaBetize (const void *a, const void *b)
{
  const char * aa = (const char *) a;
  const char * bb = (const char *) b;
  int r = strcasecmp(aa, bb);
  if (r) return r;
  /* if equal ignoring case, use opposite of strcmp()
   * result to get lower before upper */
  return -strcmp(aa, bb); /* aka: return strcmp(b, a); */
}

void
array_sort_alphabetically (char ** array,
                           int     size,
                           int     case_sensitive)
{
  if (!case_sensitive)
    qsort (array, size, sizeof (char *), alphaBetize);
}
