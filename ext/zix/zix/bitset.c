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

#include <string.h>

#include <stdio.h>

#include "zix/bitset.h"

/** Number of bits per ZixBitset element (ala CHAR_BIT). */
#define ZIX_BITSET_ELEM_BIT (CHAR_BIT * sizeof(ZixBitset))

ZIX_API void
zix_bitset_clear(ZixBitset* b, ZixBitsetTally* t, size_t n_bits)
{
	memset(b, 0, ZIX_BITSET_ELEMS(n_bits) * sizeof(ZixBitset));
	memset(t, 0, ZIX_BITSET_ELEMS(n_bits) * sizeof(ZixBitsetTally));
}

ZIX_API void
zix_bitset_set(ZixBitset* b, ZixBitsetTally* t, size_t i)
{
	const size_t    e    = i / ZIX_BITSET_ELEM_BIT;
	const size_t    ebit = i & (ZIX_BITSET_ELEM_BIT - 1); // i % ELEM_BIT
	const ZixBitset mask = (ZixBitset)1 << ebit;

	if (!(b[e] & mask)) {
		++t[e];
	}

	b[e] |= mask;
}

ZIX_API void
zix_bitset_reset(ZixBitset* b, ZixBitsetTally* t, size_t i)
{
	const size_t    e    = i / ZIX_BITSET_ELEM_BIT;
	const size_t    ebit = i & (ZIX_BITSET_ELEM_BIT - 1); // i % ELEM_BIT
	const ZixBitset mask = (ZixBitset)1 << ebit;

	if (b[e] & mask) {
		--t[e];
	}

	b[e] &= ~mask;
}

ZIX_API bool
zix_bitset_get(const ZixBitset* b, size_t i)
{
	const size_t    e    = i / ZIX_BITSET_ELEM_BIT;
	const size_t    ebit = i & (ZIX_BITSET_ELEM_BIT - 1); // i % ELEM_BIT
	const ZixBitset mask = (ZixBitset)1 << ebit;

	return b[e] & mask;
}

ZIX_API size_t
zix_bitset_count_up_to(const ZixBitset* b, const ZixBitsetTally* t, size_t i)
{
	const size_t full_elems = i / ZIX_BITSET_ELEM_BIT;
	const size_t extra      = i & (ZIX_BITSET_ELEM_BIT - 1); // i % ELEM_BIT

	size_t count = 0;
	for (size_t e = 0; e < full_elems; ++e) {
		count += t[e];
	}

	const ZixBitset mask = ~(~(ZixBitset)0 << extra);
	count += __builtin_popcountl(b[full_elems] & mask);

	return count;
}

ZIX_API size_t
zix_bitset_count_up_to_if(const ZixBitset* b, const ZixBitsetTally* t, size_t i)
{
	const size_t full_elems = i / ZIX_BITSET_ELEM_BIT;
	const size_t extra      = i & (ZIX_BITSET_ELEM_BIT - 1); // i % ELEM_BIT

	const ZixBitset get_mask = (ZixBitset)1 << extra;
	if (!(b[full_elems] & get_mask)) {
		return (size_t)-1;
	}

	size_t count = 0;
	for (size_t e = 0; e < full_elems; ++e) {
		count += t[e];
	}

	const ZixBitset mask = ~(~(ZixBitset)0 << extra);
	count += __builtin_popcountl(b[full_elems] & mask);

	return count;
}
