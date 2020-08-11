/*
 * Copyright (C) 2018-2020 Alexandros Theodotou <alex at zrythm dot org>
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
 * Array helpers.
 */

#include <stdlib.h>
#include <string.h>

#include "utils/arrays.h"

#include <glib.h>

/**
 * Returns 1 if element exists in array, 0 if not.
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
  void ** array,
  int     size,
  void *  element,
  int (*cmp)(void *, void *),
  int     equal_val,
  int     pointers)
{
  for (int i = 0; i < size; i++)
    {
      if (pointers)
        {
          if (cmp (array[i], element) == equal_val)
            return 1;
        }
      else
        {
          if (cmp (&array[i], element) == equal_val)
            return 1;
        }
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

/**
 * Swaps the elements of the 2 arrays.
 *
 * The arrays must be of pointers.
 *
 * @param arr1 Dest array.
 * @param arr2 Source array.
 */
void
_array_dynamic_swap (
  void ***  arr1,
  size_t * sz1,
  void ***  arr2,
  size_t * sz2)
{
  g_return_if_fail (
    arr1 && arr2 && *arr1 && *arr2);
  int is_1_larger =
    *sz1 > *sz2;

  void *** large_arr;
  size_t * large_sz;
  void *** small_arr;
  size_t * small_sz;
  if (is_1_larger)
    {
      large_arr = arr1;
      large_sz = sz1;
      small_arr = arr2;
      small_sz = sz2;
    }
  else
    {
      large_arr = arr2;
      large_sz = sz2;
      small_arr = arr1;
      small_sz = sz1;
    }

  /* resize the small array */
  *small_arr =
    realloc (
      *small_arr, *large_sz * sizeof (void *));

  /* copy the elements of the small array in tmp */
  void * tmp[*small_sz > 0 ? *small_sz : 1];
  memcpy (
    tmp, *small_arr, sizeof (void *) * *small_sz);

  /* copy elements from large array to small */
  memcpy (
    *small_arr, *large_arr,
    sizeof (void *) * *large_sz);

  /* resize large array */
  *large_arr =
    realloc (
      *large_arr, *small_sz * sizeof (void *));

  /* copy the elements from temp to large array */
  memcpy (
    *large_arr, tmp, sizeof (void *) * *small_sz);

  /* update the sizes */
  size_t orig_large_sz = *large_sz;
  size_t orig_small_sz = *small_sz;
  *large_sz = orig_small_sz;
  *small_sz = orig_large_sz;
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
    qsort (
      array, (size_t) size,
      sizeof (char *), alphaBetize);
}

/**
 * Shuffle array elements.
 *
 * @param n Count.
 * @param size Size of each element
 *   (@code sizeof (arr[0]) @endcode).
 */
void array_shuffle (
  void *array, size_t n, size_t size)
{
  char tmp[size];
  char *arr = array;
  size_t stride = size * sizeof(char);

  if (n > 1)
    {
      size_t i;
      for (i = 0; i < n - 1; ++i)
        {
          size_t rnd = (size_t) rand();
          size_t j =
            i + rnd / (RAND_MAX / (n - i) + 1);

          memcpy (tmp, arr + j * stride, size);
          memcpy (
            arr + j * stride,
            arr + i * stride, size);
          memcpy (arr + i * stride, tmp, size);
        }
    }
}

static int
cmp_float_func (const void * a, const void * b)
{
  return ( *(float*)a - *(float*)b ) < 0.f ? -1 : 1;
}

void
array_sort_float (
  float * array,
  int     size)
{
  qsort (
    array, (size_t) size, sizeof (long),
    cmp_float_func);
}

static int
cmp_long_func (const void * a, const void * b)
{
  return ( *(long*)a - *(long*)b );
}

void
array_sort_long (
  long * array,
  int     size)
{
  qsort (
    array, (size_t) size, sizeof (long),
    cmp_long_func);
}
