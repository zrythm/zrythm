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

/**
 * \file
 *
 * Various algorithms.
 */

#ifndef __UTILS_ALGORITHMS_H__
#define __UTILS_ALGORITHMS_H__

#include <stdbool.h>
#include <stddef.h>

#include "utils/types.h"

static inline int
algorithm_sort_int_cmpfunc (const void * a, const void * b)
{
  return (*(int *) a - *(int *) b);
}

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
HOT PURE NONNULL void *
algorithms_binary_search_nearby (
  const void *   key,
  const void *   base,
  size_t         nmemb,
  size_t         size,
  GenericCmpFunc cmp_func,
  bool           return_prev,
  bool           include_equal);

#endif
