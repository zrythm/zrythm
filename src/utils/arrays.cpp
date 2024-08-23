// SPDX-FileCopyrightText: © 2018-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Array helpers.
 */

#include <cstdlib>
#include <cstring>

#include "utils/arrays.h"
#include "utils/mem.h"
#include "utils/objects.h"
#include "utils/pcg_rand.h"
#include "zrythm.h"

#include <glib.h>

/**
 * Returns 1 if element exists in array, 0 if not.
 *
 * The element exists if the pointers are equal.
 */
int
_array_contains (void ** _array, int size, void * element)
{
  for (int i = 0; i < size; i++)
    {
      if (_array[i] == element)
        return 1;
    }
  return 0;
}

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
  int pointers)
{
  for (int i = 0; i < size; i++)
    {
      if (pointers)
        {
          if (cmp (_array[i], element) == equal_val)
            return 1;
        }
      else
        {
          if (cmp (&_array[i], element) == equal_val)
            return 1;
        }
    }
  return 0;
}

/**
 * Returns the index of the element exists in array,
 * -1 if not.
 */
int
_array_index_of (void ** _array, int size, void * element)
{
  for (int i = 0; i < size; i++)
    {
      if (_array[i] == element)
        return i;
    }
  return -1;
}

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
void
_array_double_size_if_full (
  void **  arr_ptr,
  size_t   count,
  size_t * max_sz,
  size_t   el_size)
{
  if (G_LIKELY (count < *max_sz))
    {
      /* nothing to do, array is not full */
      return;
    }

  if (G_UNLIKELY (count > *max_sz && *max_sz != 0))
    {
      z_error ("invalid count ({}) and max sz ({})", count, *max_sz);
      return;
    }

  size_t new_sz = count * 2;
  if (count == 0)
    {
      new_sz = 1;
      *arr_ptr = object_new_n_sizeof (new_sz, el_size);
    }
  if (*max_sz == 0)
    {
      /*z_info ("FIXME: current size of array given is 0.");*/
      *arr_ptr = g_realloc_n (*arr_ptr, new_sz, el_size);
    }
  else
    {
      *arr_ptr =
        object_realloc_n_sizeof (*arr_ptr, *max_sz * el_size, new_sz * el_size);
    }
  *max_sz = new_sz;
}

/**
 * Shuffle array elements.
 *
 * @param n Count.
 * @param size Size of each element
 *   (@code sizeof (arr[0]) @endcode).
 */
void
array_shuffle (void * _array, size_t n, size_t size)
{
  char   tmp[size];
  char * arr = static_cast<char *> (_array);
  size_t stride = size * sizeof (char);

  auto rand = PCGRand::getInstance ();
  if (n > 1)
    {
      size_t i;
      for (i = 0; i < n - 1; ++i)
        {
          size_t rnd = (size_t) ((double) rand->uf () * (double) RAND_MAX);
          size_t j = i + rnd / (RAND_MAX / (n - i) + 1);

          memcpy (tmp, arr + j * stride, size);
          memcpy (arr + j * stride, arr + i * stride, size);
          memcpy (arr + i * stride, tmp, size);
        }
    }
}