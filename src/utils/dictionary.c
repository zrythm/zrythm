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

#include "utils/dictionary.h"

Dictionary *
dictionary_new (void)
{
  Dictionary proto = {
    0, 10, malloc (10 * sizeof (DictionaryEntry))
  };
  Dictionary * d = malloc(sizeof(Dictionary));
  *d = proto;
  return d;
}

static int
dictionary_find_index (
  Dictionary * dict,
  const char *key)
{
  for (int i = 0; i < dict->len; i++)
    {
      if (!strcmp(dict->entry[i].key, key))
        {
          return i;
        }
    }
  return -1;
}

void *
dictionary_find (
  Dictionary * dict,
  const char * key,
  void *       def)
{
  int idx = dictionary_find_index(dict, key);
  return idx == -1 ? def : dict->entry[idx].val;
}

void
_dictionary_add (
  Dictionary * dict,
  const char * key,
  void *       value)
{
  int idx = dictionary_find_index(dict, key);
  if (idx != -1)
    {
      dict->entry[idx].val= value;
      return;
    }
  if (dict->len == (int) dict->size)
    {
      dict->size *= 2;
      dict->entry =
        realloc (
          dict->entry,
          dict->size * sizeof( DictionaryEntry));
    }
  dict->entry[dict->len].key = strdup (key);
  dict->entry[dict->len].val = value;
  dict->len++;
}

void
dictionary_free (
  Dictionary * dict)
{
  for (int i = 0; i < dict->len; i++)
    {
      free (dict->entry[i].key);
    }
  free(dict->entry);
  free(dict);
}
