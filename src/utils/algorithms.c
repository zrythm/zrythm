/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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

#include "utils/algorithms.h"

/**
 * Binary search with the option to find the closest
 * member in a sorted array.
 *
 * All of the parameters except the following are
 * the same as the C std bsearch().
 *
 * @param return_prev True to return previous
 *  closest element, false for next.
 * @param include_equal Include equal elements (if an
 *   exact match is found, return it).
 */
void *
algorithms_binary_search_nearby (
  const void *   key,
  const void *   base,
  size_t         nmemb,
  size_t         size,
  GenericCmpFunc cmp_func,
  bool           return_prev,
  bool           include_equal)
{
  /* init values */
  size_t       first = 0;
  size_t       last = nmemb;
  size_t       middle = (first + last) / 2;
  bool         pivot_is_before, pivot_succ_is_before;
  const void * pivot;
  const void * pivot_succ;

  /* return if no objects */
  if (first == last)
    {
      return NULL;
    }

  /* search loops, exit if pos is not in array */
  void * ret = NULL;
  while (first <= last)
    {
      pivot =
        (void *) (((const char *) base) + (middle * size));
      pivot_succ = NULL;
      pivot_succ_is_before = 0;

      if (middle == 0 && return_prev)
        {
          return (void *) base;
        }

      /* Return next/previous item if pivot is th
       * searched position */
      if (cmp_func (pivot, key) == 0)
        {
          if (include_equal)
            return (void *) pivot;
          else if (return_prev)
            ret =
              (void *) (((const char *) base) + ((middle - 1) * size));
          else
            ret =
              (void *) (((const char *) base) + ((middle + 1) * size));
          return ret;
        }

      /* Select pivot successor if possible */
      if (middle < last)
        {
          pivot_succ =
            (void *) (((const char *) base) + ((middle + 1) * size));
          pivot_succ_is_before =
            cmp_func (pivot_succ, key) <= 0;
        }
      pivot_is_before = cmp_func (pivot, key) <= 0;

      /* if pivot and pivot_succ are before pos,
       * search in the second half on next iteration */
      if (pivot_is_before && pivot_succ_is_before)
        {
          first = middle + 1;
        }
      /* else if key is between pivot and pivot_succ */
      else if (pivot_is_before)
        {
          if (return_prev)
            {
              return (void *) pivot;
            }
          else
            {
              return (void *) pivot_succ;
            }
        }
      /* else if pivot_succ and pivot are behind pos,
       * search in the first half on next iteration */
      else
        {
          last = middle;
        }

      /* recalculate middle position */
      middle = (first + last) / 2;
    }

  return NULL;
}
