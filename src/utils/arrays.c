/*
 * utils/arrays.c - Array helpers
 *
 * Copyright (C) 2018 Alexandros Theodotou
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

#include "utils/arrays.h"

void
array_delete (void ** array, int * size, void * element)
{
  for (int i = 0; i < (* size); i++)
    {
      if (array[i] == element)
        {
          --(* size);
          for (int j = i; j < (* size); j++)
            {
              array[j] = array[j + 1];
            }
          break;
        }
    }
}

/**
 * Appends element to the end of array array and increases the size.
 */
void
array_append (void ** array, int * size, void * element)
{
  array[(* size)] = element;
  (* size)++;
}

/**
 * Inserts element in array at pos and increases the size.
 */
void
array_insert (void ** array, int * size, int pos, void * element)
{
  for (int i = pos; i < (* size); i++)
    {
      array[i + 1] = array[i];
    }
  array[pos] = element;
  (* size)++;
}

/**
 * Returns 1 if element exists in array, 0 if not.
 *
 * TODO rename arrays to array
 */
int
array_contains (void ** array, int size, void * element)
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
array_index_of (void ** array, int size, void * element)
{
  for (int i = 0; i < size; i++)
    {
      if (array[i] == element)
        return i;
    }
  return -1;
}
