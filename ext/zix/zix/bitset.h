/*
  Copyright 2014-2016 David Robillard <http://drobilla.net>

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
*/

#ifndef ZIX_BITSET_H
#define ZIX_BITSET_H

#include <stddef.h>
#include <stdint.h>
#include <limits.h>

#include "zix/common.h"

/**
   @addtogroup zix
   @{
   @name Bitset
   @{
*/

/**
   A bitset (always referred to by pointer, actually an array).
*/
typedef unsigned long ZixBitset;

/**
   Tally of the number of bits in one ZixBitset element.
*/
typedef uint8_t ZixBitsetTally;

/**
   The number of bits per ZixBitset array element.
*/
#define ZIX_BITSET_BITS_PER_ELEM (CHAR_BIT * sizeof(ZixBitset))

/**
   The number of bitset elements needed for the given number of bits.
*/
#define ZIX_BITSET_ELEMS(n_bits) \
	((n_bits / ZIX_BITSET_BITS_PER_ELEM) + \
	 (n_bits % ZIX_BITSET_BITS_PER_ELEM ? 1 : 0))

/**
   Clear a Bitset.
*/
ZIX_API void
zix_bitset_clear(ZixBitset* b, ZixBitsetTally* t, size_t n_bits);

/**
   Set bit `i` in `t` to 1.
*/
ZIX_API void
zix_bitset_set(ZixBitset* b, ZixBitsetTally* t, size_t i);

/**
   Clear bit `i` in `t` (set to 0).
*/
ZIX_API void
zix_bitset_reset(ZixBitset* b, ZixBitsetTally* t, size_t i);

/**
   Return the `i`th bit in `t`.
*/
ZIX_API bool
zix_bitset_get(const ZixBitset* b, size_t i);

/**
   Return the number of set bits in `b` up to bit `i` (non-inclusive).
*/
ZIX_API size_t
zix_bitset_count_up_to(const ZixBitset* b, const ZixBitsetTally* t, size_t i);

/**
   Return the number of set bits in `b` up to bit `i` (non-inclusive) if bit
   `i` is set, or (size_t)-1 otherwise.
*/
ZIX_API size_t
zix_bitset_count_up_to_if(const ZixBitset* b, const ZixBitsetTally* t, size_t i);

/**
   @}
   @}
*/

#endif  /* ZIX_BITSET_H */
