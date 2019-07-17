/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * Conducts binary search on the object's member
 * array to find the previous or next member closest
 * to the given one.
 *
 * @note This requires that the search value and
 *   the elements in the array are of the same value.
 *
 * @param arr The array.
 * @param sval Value to search with.
 * @param rprev 1 to return previous element, 0 for
 *   next.
 * @param count Element count.
 * @param type Data type of each element. If the
 *   data type is MyStruct then "MyStruct *" should
 *   be passed with the "&" character passed to amp.
 * @param cmp Comparator function for two elements.
 * @param amp The & symbol, if the array does not
 *   contain primitives or pointers. In this case,
 *   type must also contain an asterisk.
 * @param ret Variable to store the return value in.
 * @param null Value to set to ret if nothing found.
 */
#define algorithms_binary_search_nearby( \
  arr,sval,rprev,count,type,cmp,amp,ret,null) \
{ \
  /* init values */ \
  int first = 0; \
  int last = count; \
  int middle = (first + last) / 2; \
  int pivot_is_before, pivot_succ_is_before; \
  type pivot; \
  type pivot_succ; \
 \
  /* return if SnapGrid has no entries */ \
  if (first == last) \
  { \
    ret = null; \
    goto end_binary_search; \
  } \
 \
  /* search loops, exit if pos is not in array */ \
  while (first <= last) \
  { \
    pivot = amp arr[middle]; \
    pivot_succ = null; \
    pivot_succ_is_before = 0; \
 \
    if (middle == 0 && rprev) { \
      ret = amp arr[0]; \
      goto end_binary_search; \
    } \
 \
    /* Return next/previous item if pivot is the
     * searched position */ \
    if (cmp (pivot, sval) == 0) { \
      if (rprev) \
        ret = amp arr[middle - 1]; \
      else \
        ret = amp arr[middle + 1]; \
      goto end_binary_search; \
    } \
 \
    /* Select pivot successor if possible */ \
    if (middle < last) \
    { \
      pivot_succ = amp arr[middle + 1]; \
      pivot_succ_is_before = \
        cmp (pivot_succ, sval) <= 0; \
    } \
    pivot_is_before = \
      cmp (pivot, sval) <= 0; \
 \
    /* if pivot and pivot_succ are before pos, search in the second half on next iteration */ \
    if (pivot_is_before && pivot_succ_is_before) \
    { \
      first = middle + 1; \
    } \
    /* sval is between pivot and pivot_succ */ \
    else if (pivot_is_before) \
    { \
      if (return_prev) { \
        ret = pivot; \
        goto end_binary_search; \
      } else { \
        ret = pivot_succ; \
        goto end_binary_search; \
      } \
    } \
    else { /* if pivot_succ and pivot are behind pos, search in the first half on next iteration */ \
      last = middle; \
    } \
 \
    /* recalculate middle position */ \
    middle = (first + last) / 2; \
  } \
  ret = null; \
end_binary_search: ; \
}

#endif
