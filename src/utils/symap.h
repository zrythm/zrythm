// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
  Copyright 2011-2014 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

 SPDX-License-Identifier: ISC
*/

/**
   @file symap.h API for Symap, a basic symbol map (string interner).

   Particularly useful for implementing LV2 URI mapping.

   @see <a href="http://lv2plug.in/ns/ext/urid">LV2 URID</a>
   @see <a href="http://lv2plug.in/ns/ext/uri-map">LV2 URI Map</a>
*/

#ifndef SYMAP_H
#define SYMAP_H

#include <cstdint>
#include <utility>

#define ZSYMAP (ZRYTHM->symap)

/**
 * @brief A string interner (Symbol Map).
 *
 */
class Symap
{
public:
  Symap () = default;
  Symap (const Symap &other) { *this = other; }
  Symap &operator= (const Symap &other)
  {
    Symap tmp (other);
    swap (*this, tmp);
    return *this;
  }
  Symap (Symap &&other) { *this = std::move (other); }
  Symap &operator= (Symap &&other)
  {
    Symap tmp (std::move (other));
    swap (*this, tmp);
    return *this;
  }
  ~Symap ();

  friend void swap (Symap &first, Symap &second) // nothrow
  {
    using std::swap;

    swap (first.index, second.index);
    swap (first.size, second.size);
    swap (first.symbols, second.symbols);
  }

  /**
     Map a string to a symbol ID if it is already mapped, otherwise return 0.
  */
  uint32_t try_map (const char * sym);

  /**
     Map a string to a symbol ID.

     Note that 0 is never a valid symbol ID.
  */
  uint32_t map (const char * sym);

  /**
     Unmap a symbol ID back to a symbol, or NULL if no such ID exists.

     Note that 0 is never a valid symbol ID.
  */
  const char * unmap (uint32_t id);

private:
  uint32_t search (const char * sym, bool * exact) const;

  /**
     Unsorted array of strings, such that the symbol for ID i is found
     at symbols[i - 1].
  */
  char ** symbols = nullptr;

  /**
     Array of IDs, sorted by corresponding string in `symbols`.
  */
  uint32_t * index = nullptr;

  /**
     Number of symbols (number of items in `symbols` and `index`).
  */
  uint32_t size = 0;
};

#endif /* SYMAP_H */
