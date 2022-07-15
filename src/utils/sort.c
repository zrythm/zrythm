/*
 * Copyright (C) 2021-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include <string.h>

#include "utils/sort.h"

#include <strings.h>

/**
 * Alphabetical sort func.
 *
 * The arguments must be strings (char *).
 */
int
sort_alphabetical_func (const void * a, const void * b)
{
  char * pa = *(char * const *) a;
  char * pb = *(char * const *) b;
  int    r = strcasecmp (pa, pb);
  if (r)
    return r;

  /* if equal ignoring case, use opposite of strcmp() result to get
   * lower before upper */
  return -strcmp (pa, pb); /* aka: return strcmp(b, a); */
}
