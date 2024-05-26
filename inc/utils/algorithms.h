// SPDX-FileCopyrightText: © 2019, 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
HOT NONNULL void *
algorithms_binary_search_nearby (
  const void *   key,
  const void *   base,
  size_t         nmemb,
  size_t         size,
  GenericCmpFunc cmp_func,
  bool           return_prev,
  bool           include_equal);

#endif
