/*
  Copyright 2014 David Robillard <http://drobilla.net>

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

#ifndef ZIX_TRIE_UTIL_H
#define ZIX_TRIE_UTIL_H

#ifdef __SSE4_2__
#    include <smmintrin.h>
#endif

#ifdef ZIX_TRIE_LINEAR_SEARCH

/** Return the index of `k` in `keys`, or `n` if not found. */
ZIX_PRIVATE size_t
zix_trie_find_key(const uint8_t* const keys, const size_t n, const uint8_t k)
{
	for (size_t i = 0; i < n; ++i) {
		if (keys[i] == k) {
			assert(keys[i] >= k);
			return i;
		}
	}
	return n;
}

#else

/** Return the index of the first element in `keys` >= `k`, or `n`. */
ZIX_PRIVATE size_t
zix_trie_find_key(const uint8_t* const keys, const size_t n, const uint8_t k)
{
	size_t first = 0;
	size_t len   = n;
	while (len > 0) {
		const size_t half = len >> 1;
		const size_t i    = first + half;
		if (keys[i] < k) {
			const size_t chop = half + 1;
			first += chop;
			len   -= chop;
		} else {
			len = half;
		}
	}
	assert(first == n || keys[first] >= k);
	return first;
}

#endif

#if !defined (__SSE4_2__) || !defined(NDEBUG)

ZIX_PRIVATE size_t
zix_trie_change_index_c(const uint8_t* a, const uint8_t* b, size_t len)
{
	for (size_t i = 0; i < len; ++i) {
		if (a[i] != b[i]) {
			return i;
		}
	}
	return len;
}

#endif

#ifdef __SSE4_2__

ZIX_PRIVATE size_t
zix_trie_change_index_sse(const uint8_t* a, const uint8_t* b, const size_t len)
{
	const size_t veclen = len & ~(sizeof(__m128i) - 1);
	size_t       i      = 0;
	for (; i < veclen; i += sizeof(__m128i)) {
		const __m128i  r     = _mm_loadu_si128((const __m128i*)(a + i));
		const __m128i  s     = _mm_loadu_si128((const __m128i*)(b + i));
		const int      index = _mm_cmpistri(
			r, s, _SIDD_SBYTE_OPS|_SIDD_CMP_EQUAL_EACH|_SIDD_NEGATIVE_POLARITY);

		if (index != sizeof(__m128i)) {
			assert(i + index == change_index_c(a, b, len));
			return i + index;
		}
	}

	const __m128i r     = _mm_loadu_si128((const __m128i*)(a + i));
	const __m128i s     = _mm_loadu_si128((const __m128i*)(b + i));
	const size_t  l     = len - i;
	const int     index = _mm_cmpestri(
		r, l, s, l,
		_SIDD_SBYTE_OPS|_SIDD_CMP_EQUAL_EACH|_SIDD_MASKED_NEGATIVE_POLARITY);

	assert(i + index == change_index_c(a, b, len));
	return i + index;
}

#endif

ZIX_PRIVATE size_t
zix_trie_change_index(const uint8_t* a, const uint8_t* b, const size_t len)
{
#ifdef __SSE4_2__
	return zix_trie_change_index_sse(a, b, len);
#else
	return zix_trie_change_index_c(a, b, len);
#endif
}

#endif  /* ZIX_TRIE_UTIL_H */
