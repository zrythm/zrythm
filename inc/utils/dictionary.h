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
 * Dictionary implementation.
 */

#ifndef __UTILS_DICTIONARY_H__
#define __UTILS_DICTIONARY_H__

#include <stdlib.h>

#include <gtk/gtk.h>

/**
 * \file
 *
 * Dictionary implementation.
 */

#define DICTIONARY_PUSH(s, element) dictionary_push (s, (void *) element)

typedef struct DictionaryEntry
{
  char * key;
  void * val;
} DictionaryEntry;

typedef struct Dictionary
{
  /** Number of valid elements in the array. */
  int len;
  /** Size of the array. */
  size_t            size;
  DictionaryEntry * entry;
} Dictionary;

Dictionary *
dictionary_new (void);

#define dictionary_find_simple(dict, key, type) \
  ((type *) dictionary_find (dict, key, NULL))

void *
dictionary_find (Dictionary * dict, const char * key, void * def);

#define dictionary_add(dict, key, val) _dictionary_add (dict, key, (void *) val)

void
_dictionary_add (Dictionary * dict, const char * key, void * value);

void
dictionary_free (Dictionary * self);

/**
 * @}
 */

#endif
