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

#include <stdarg.h>
#include <stdio.h>

#include "zix/bitset.h"

#define N_BITS  256
#define N_ELEMS (ZIX_BITSET_ELEMS(N_BITS))

static int
test_fail(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
	return 1;
}

int
main(int argc, char** argv)
{
	ZixBitset      b[N_ELEMS];
	ZixBitsetTally t[N_ELEMS];

	zix_bitset_clear(b, t, N_BITS);
	if (zix_bitset_count_up_to(b, t, N_BITS) != 0) {
		return test_fail("Cleared bitset has non-zero count\n");
	}

	for (size_t i = 0; i < N_BITS; ++i) {
		zix_bitset_set(b, t, i);
		const size_t count = zix_bitset_count_up_to(b, t, N_BITS);
		if (count != i + 1) {
			return test_fail("Count %zu != %zu\n", count, i + 1);
		} else if (!zix_bitset_get(b, i)) {
			return test_fail("Bit %zu is not set\n", i);
		}
	}

	for (size_t i = 0; i <= N_BITS; ++i) {
		const size_t count = zix_bitset_count_up_to(b, t, i);
		if (count != i) {
			return test_fail("Count up to %zu is %zu != %zu\n", i, count, i);
		}
	}

	for (size_t i = 0; i <= N_BITS; ++i) {
		zix_bitset_reset(b, t, i);
		const size_t count = zix_bitset_count_up_to(b, t, i);
		if (count != 0) {
			return test_fail("Count up to %zu is %zu != %zu\n", i, count, 0);
		}
	}

	zix_bitset_clear(b, t, N_BITS);
	for (size_t i = 0; i <= N_BITS; i += 2) {
		zix_bitset_set(b, t, i);
		const size_t count = zix_bitset_count_up_to(b, t, i + 1);
		if (count != i / 2 + 1) {
			return test_fail("Count up to %zu is %zu != %zu\n", i, count, i / 2 + 1);
		}
	}

	return 0;
}
