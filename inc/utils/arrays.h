/*
 * SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __UTILS_ARRAYS_H__
#define __UTILS_ARRAYS_H__

#include <cstddef>

#define array_index_of(_array, size, element) \
  _array_index_of ((void **) _array, size, (void *) element)

/**
 * Appends element to the end of array array and increases the size.
 */
#define array_append(_array, size, element) (_array)[(size)++] = element;

#define array_double_append(arr1, arr2, size, el1, el2) \
  arr1[size] = el1; \
  arr2[size] = el2; \
  size++;

/**
 * Inserts element in array at pos and increases the
 * size.
 */
#define array_insert(_array, size, pos, element) \
  for (int ii = size; ii > pos; ii--) \
    { \
      _array[ii] = _array[ii - 1]; \
    } \
  _array[pos] = element; \
  size++;

/**
 * Inserts elements in 2 arrays at the same time.
 *
 * Assumes the arrays are the same size.
 */
#define array_double_insert(arr1, arr2, size, pos, el1, el2) \
  for (int ii = size; ii > pos; ii--) \
    { \
      arr1[ii] = arr1[ii - 1]; \
      arr2[ii] = arr2[ii - 1]; \
    } \
  arr1[pos] = el1; \
  arr2[pos] = el2; \
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

/**
 * Deletes element from array and rearranges other
 * elements accordingly.
 */
#define array_delete(_array, size, element) \
  for (size_t ii = 0; ii < (size_t) size; ii++) \
    { \
      if ((void *) _array[ii] == (void *) element) \
        { \
          --size; \
          for (size_t jj = ii; jj < (size_t) size; jj++) \
            { \
              _array[jj] = _array[jj + 1]; \
            } \
          break; \
        } \
    }

/**
 * Deletes element from array and rearranges other
 * elements accordingly.
 */
#define array_delete_confirm(_array, size, element, confirm) \
  confirm = false; \
  for (size_t ii = 0; ii < (size_t) size; ii++) \
    { \
      if ((void *) _array[ii] == (void *) element) \
        { \
          confirm = true; \
          --size; \
          for (size_t jj = ii; jj < (size_t) size; jj++) \
            { \
              _array[jj] = _array[jj + 1]; \
            } \
          break; \
        } \
    }

/**
 * Deletes element from array and rearranges other
 * elements accordingly.
 */
#define array_delete_primitive(_array, size, element) \
  for (int ii = 0; ii < size; ii++) \
    { \
      if (_array[ii] == element) \
        { \
          --size; \
          for (int jj = ii; jj < size; jj++) \
            { \
              _array[jj] = _array[jj + 1]; \
            } \
          break; \
        } \
    }

/**
 * Same as array_delete but sets the deleted element
 * index to pos.
 */
#define array_delete_return_pos(_array, size, element, pos) \
  for (int ii = 0; ii < size; ii++) \
    { \
      if (_array[ii] == element) \
        { \
          pos = ii; \
          --size; \
          for (int jj = ii; jj < size; jj++) \
            { \
              _array[jj] = _array[jj + 1]; \
            } \
          break; \
        } \
    }

/**
 * Deletes from two arrays concurrently.
 *
 * Assumes that the element should be deleted at the
 * same index in both arrays.
 */
#define array_double_delete(array1, array2, size, element1, element2) \
  for (int ii = 0; ii < size; ii++) \
    { \
      if (array1[ii] == element1) \
        { \
          --size; \
          for (int jj = ii; jj < size; jj++) \
            { \
              array1[jj] = array1[jj + 1]; \
              array2[jj] = array2[jj + 1]; \
            } \
          break; \
        } \
    }

#define array_contains_cmp(_array, size, element, cmp, equal_val, pointers) \
  _array_contains_cmp ( \
    (void **) _array, size, (void *) element, (int (*) (void *, void *)) cmp, \
    equal_val, pointers)

/**
 * Macro so that no casting to void ** and void * is
 * necessary.
 */
#define array_contains(_array, size, element) \
  _array_contains ((void **) _array, size, (void *) element)

#define array_dynamic_swap(arr1, sz1, arr2, sz2) \
  _array_dynamic_swap ( \
    (void ***) arr1, (size_t *) sz1, (void ***) arr2, (size_t *) sz2)

/**
 * Swaps the elements of the 2 arrays.
 *
 * The arrays must be of pointers.
 *
 * @param arr1 Dest array.
 * @param arr2 Source array.
 */
void
_array_dynamic_swap (void *** arr1, size_t * sz1, void *** arr2, size_t * sz2);

static inline int
array_contains_int (int * _array, int size, int element)
{
  for (int i = 0; i < size; i++)
    {
      if (_array[i] == element)
        return 1;
    }
  return 0;
}

/**
 * Shuffle array elements.
 *
 * @param n Count.
 * @param size Size of each element
 *   (@code sizeof (arr[0]) @endcode).
 */
void
array_shuffle (void * _array, size_t n, size_t size);

/**
 * Returns 1 if element exists in array, 0 if not.
 *
 * The element exists if the pointers are equal.
 */
NONNULL WARN_UNUSED_RESULT int
_array_contains (void ** _array, int size, void * element);

/**
 * Returns if the array contains an element by
 * comparing the values using the comparator
 * function.
 *
 * @param equal_val The return value of the
 *   comparator function that should be interpreted
 *   as equal. Normally this will be 0.
 * @param pointers Whether the elements are
 *   pointers or not.
 */
int
_array_contains_cmp (
  void ** _array,
  int     size,
  void *  element,
  int (*cmp) (void *, void *),
  int equal_val,
  int pointers);

/**
 * Returns the index of the element exists in array,
 * -1 if not.
 */
int
_array_index_of (void ** _array, int size, void * element);

#endif /* __UTILS_ARRAYS_H__ */
