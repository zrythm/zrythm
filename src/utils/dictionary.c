/*
 * SPDX-FileCopyrightText: © 2019,2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "utils/dictionary.h"
#include "utils/mem.h"
#include "utils/objects.h"

Dictionary *
dictionary_new (void)
{
  Dictionary   proto = { 0, 10, object_new_n (10, DictionaryEntry) };
  Dictionary * d = object_new (Dictionary);
  *d = proto;
  return d;
}

static int
dictionary_find_index (Dictionary * dict, const char * key)
{
  for (int i = 0; i < dict->len; i++)
    {
      if (!strcmp (dict->entry[i].key, key))
        {
          return i;
        }
    }
  return -1;
}

void *
dictionary_find (Dictionary * dict, const char * key, void * def)
{
  int idx = dictionary_find_index (dict, key);
  return idx == -1 ? def : dict->entry[idx].val;
}

void
_dictionary_add (Dictionary * dict, const char * key, void * value)
{
  int idx = dictionary_find_index (dict, key);
  if (idx != -1)
    {
      dict->entry[idx].val = value;
      return;
    }
  if (dict->len == (int) dict->size)
    {
      dict->entry = object_realloc_n (
        dict->entry, dict->size, dict->size * 2, DictionaryEntry);
      dict->size *= 2;
    }
  dict->entry[dict->len].key = g_strdup (key);
  dict->entry[dict->len].val = value;
  dict->len++;
}

void
dictionary_free (Dictionary * dict)
{
  for (int i = 0; i < dict->len; i++)
    {
      g_free_and_null (dict->entry[i].key);
    }
  free (dict->entry);
  free (dict);
}
