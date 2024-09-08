/*
 * SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __UTILS_ARRAYS_H__
#define __UTILS_ARRAYS_H__

#include <cstddef>

/**
 * Appends element to the end of array array and increases the size.
 */
#define array_append(_array, size, element) (_array)[(size)++] = element;

#define array_double_append(arr1, arr2, size, el1, el2) \
  arr1[size] = el1; \
  arr2[size] = el2; \
  size++;

/**
 * Doubles the size of the array, for dynamically allocated
 * arrays.
 *
 * If @ref max_sz is zero, this will reallocate the current
 * array to @ref count * 2.
 *
 * If @ref max_sz is equal to @ref count, this will reallocate
 * the current array to @ref count * 2 and also memset the new
 * memory to 0.
 *
 * Calling this function with other values is invalid.
 *
 * @param arr The array.
 * @param count The current number of elements.
 * @param max_sz The current max array size. The new size will
 *   be written to it.
 * @param el_sz The size of one element in the array.
 */
NONNULL void
_array_double_size_if_full (
  void **  arr_ptr,
  size_t   count,
  size_t * max_sz,
  size_t   el_size);

/**
 * Doubles the size of the array, for dynamically
 * allocated arrays.
 *
 * @param array The array.
 * @param count The current number of elements.
 * @param size The current max array size.
 * @param type The type of elements the array holds.
 */
#define array_double_size_if_full(_array, count, size, type) \
  ((size_t) (count) < (size_t) (size) \
     ? ({}) \
     : _array_double_size_if_full ( \
         (void **) &_array, (size_t) (count), &size, sizeof (type)))

#endif /* __UTILS_ARRAYS_H__ */
