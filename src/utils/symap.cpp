// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * Copyright 2011-2014 David Robillard <http://drobilla.net>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <cassert>
#include <cstdlib>
#include <cstring>

#include "utils/mem.h"
#include "utils/symap.h"

/**
  @file symap.c Implementation of Symap, a basic symbol map (string interner).

  This implementation is primitive, but has some desirable qualities: good
  (O(lg(n)) lookup performance for already-mapped symbols, minimal space
  overhead, extremely fast (O(1)) reverse mapping (ID to string), simple code,
  no dependencies.

  The tradeoff is that mapping new symbols may be quite slow.  In other words,
  this implementation is ideal for use cases with a relatively limited set of
  symbols, or where most symbols are mapped early.  It will not fare so well
  with very dynamic sets of symbols.  For that, you're better off with a
  tree-based implementation (and the associated space cost, especially if you
  need reverse mapping).
*/

Symap::~Symap ()
{
  z_free_strv (symbols);
  free (index);
}

/**
   Return the index into this->index (not the ID) corresponding to `sym`,
   or the index where a new entry for `sym` should be inserted.
*/
uint32_t
Symap::search (const char * sym, bool * exact) const
{
  *exact = false;
  if (this->size == 0) [[unlikely]]
    {
      return 0; // Empty map, insert at 0
    }
  else if (strcmp (this->symbols[this->index[this->size - 1] - 1], sym) < 0)
    {
      return this->size; // Greater than last element, append
    }

  uint32_t lower = 0;
  uint32_t upper = this->size - 1;
  uint32_t i = upper;
  int      cmp;

  while (upper >= lower)
    {
      i = lower + ((upper - lower) / 2);
      cmp = strcmp (this->symbols[this->index[i] - 1], sym);

      if (cmp == 0)
        {
          *exact = true;
          return i;
        }
      else if (cmp > 0)
        {
          if (i == 0)
            {
              break; // Avoid underflow
            }
          upper = i - 1;
        }
      else
        {
          lower = ++i;
        }
    }

  assert (!*exact || strcmp (this->symbols[this->index[i] - 1], sym) > 0);
  return i;
}

uint32_t
Symap::try_map (const char * sym)
{
  bool           exact;
  const uint32_t _index = search (sym, &exact);
  if (exact)
    {
      assert (!strcmp (this->symbols[this->index[_index]], sym));
      return this->index[_index];
    }

  return 0;
}

uint32_t
Symap::map (const char * sym)
{
  bool           exact;
  const uint32_t _index = search (sym, &exact);
  if (exact)
    {
      assert (!strcmp (this->symbols[this->index[_index] - 1], sym));
      return this->index[_index];
    }

  const uint32_t id = ++this->size;
  char * const   str = strdup (sym);

  /* Append new symbol to symbols array */
  this->symbols = (char **) realloc (this->symbols, this->size * sizeof (str));
  this->symbols[id - 1] = str;

  /* Insert new index element into sorted index */
  this->index =
    (uint32_t *) realloc (this->index, this->size * sizeof (uint32_t));
  if (_index < this->size - 1)
    {
      memmove (
        this->index + _index + 1, this->index + _index,
        (this->size - _index - 1) * sizeof (uint32_t));
    }

  this->index[_index] = id;

  return id;
}

const char *
Symap::unmap (uint32_t id)
{
  if (id == 0)
    {
      return NULL;
    }
  else if (id <= this->size)
    {
      return this->symbols[id - 1];
    }
  return NULL;
}
