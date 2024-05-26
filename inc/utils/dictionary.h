/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * \file
 *
 * Dictionary implementation.
 */

#ifndef __UTILS_DICTIONARY_H__
#define __UTILS_DICTIONARY_H__

#include <cstdlib>

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
