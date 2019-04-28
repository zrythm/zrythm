/*
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
*/

#include <limits.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _MSC_VER
#    define PRIdPTR "Id"
#else
#    include <inttypes.h>
#endif

#include "zix/zix.h"

unsigned seed = 1;

// Return a pseudo-pseudo-pseudo-random-ish integer with no duplicates
static uint32_t
unique_rand(uint32_t i)
{
	i ^= 0x5CA1AB1E;  // Juggle bits to avoid linear clumps

	// Largest prime < 2^32 which satisfies (2^32 = 3 mod 4)
	static const uint32_t prime = 4294967291;
	if (i >= prime) {
		return i;  // Values >= prime are mapped to themselves
	} else {
		const uint32_t residue = ((uint64_t)i * i) % prime;
		return (i <= prime / 2) ? residue : prime - residue;
	}
}

static int
int_cmp(const void* a, const void* b, void* user_data)
{
	const intptr_t ia = (intptr_t)a;
	const intptr_t ib = (intptr_t)b;
	// note the (ia - ib) trick here would overflow
	if (ia == ib) {
		return 0;
	} else if (ia < ib) {
		return -1;
	} else {
		return 1;
	}
}

static uint32_t
ith_elem(int test_num, size_t n_elems, int i)
{
	switch (test_num % 3) {
	case 0:  return i;                // Increasing
	case 1:  return n_elems - i - 1;  // Decreasing
	default: return unique_rand(i);   // Pseudo-random
	}
}

static int
test_fail(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
	return EXIT_FAILURE;
}

static int
stress(int test_num, size_t n_elems)
{
	intptr_t      r  = 0;
	ZixBTreeIter* ti = NULL;
	ZixBTree*     t  = zix_btree_new(int_cmp, NULL, NULL);

	// Ensure begin iterator is end on empty tree
	ti = zix_btree_begin(t);
	if (!zix_btree_iter_is_end(ti)) {
		return test_fail("Begin iterator on empty tree is not end\n");
	}
	zix_btree_iter_free(ti);

	// Insert n_elems elements
	for (size_t i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (zix_btree_insert(t, (void*)r)) {
			return test_fail("Insert failed\n");
		}
	}

	// Ensure tree size is correct
	if (zix_btree_size(t) != n_elems) {
		return test_fail("Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
	}

	// Search for all elements
	for (size_t i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (zix_btree_find(t, (void*)r, &ti)) {
			return test_fail("Find %lu @ %zu failed\n", (uintptr_t)r, i);
		}
		if ((intptr_t)zix_btree_get(ti) != r) {
			return test_fail("Corrupt search: %" PRIdPTR " != %" PRIdPTR "\n",
			                 (intptr_t)zix_btree_get(ti), r);
		}
		zix_btree_iter_free(ti);
	}

	// Search for elements that don't exist
	for (size_t i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems * 3, n_elems + i);
		if (!zix_btree_find(t, (void*)r, &ti)) {
			return test_fail("Unexpectedly found %lu\n", (uintptr_t)r);
		}
	}

	// Iterate over all elements
	size_t        i    = 0;
	intptr_t      last = -1;
	for (ti = zix_btree_begin(t);
	     !zix_btree_iter_is_end(ti);
	     zix_btree_iter_increment(ti), ++i) {
		const intptr_t iter_data = (intptr_t)zix_btree_get(ti);
		if (iter_data < last) {
			return test_fail("Corrupt iter: %" PRIdPTR " < %" PRIdPTR "\n",
			                 iter_data, last);
		}
		last = iter_data;
	}
	zix_btree_iter_free(ti);
	if (i != n_elems) {
		return test_fail("Iteration stopped at %zu/%zu elements\n", i, n_elems);
	}

	// Insert n_elems elements again, ensuring duplicates fail
	for (i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (!zix_btree_insert(t, (void*)r)) {
			return test_fail("Duplicate insert succeeded\n");
		}
	}

	// Search for the middle element then iterate from there
	r = ith_elem(test_num, n_elems, n_elems / 2);
	if (zix_btree_find(t, (void*)r, &ti)) {
		return test_fail("Find %lu failed\n", (uintptr_t)r);
	}
	zix_btree_iter_increment(ti);
	for (i = 0; !zix_btree_iter_is_end(ti); zix_btree_iter_increment(ti), ++i) {
		if ((intptr_t)zix_btree_get(ti) == r) {
			return test_fail("Duplicate element %" PRIdPTR "\n",
			                 (intptr_t)zix_btree_get(ti), r);
		}
		r = ith_elem(test_num, n_elems, n_elems / 2 + i + 1);
	}
	zix_btree_iter_free(ti);

	// Delete all elements
	for (size_t e = 0; e < n_elems; e++) {
		r = ith_elem(test_num, n_elems, e);
		if (zix_btree_remove(t, (void*)r)) {
			return test_fail("Error removing item %lu\n", (uintptr_t)r);
		}
	}

	// Ensure the tree is empty
	if (zix_btree_size(t) != 0) {
		return test_fail("Tree size %zu != 0\n", zix_btree_size(t));
	}

	// Insert n_elems elements again (to test non-empty destruction)
	for (size_t e = 0; e < n_elems; ++e) {
		r = ith_elem(test_num, n_elems, e);
		if (zix_btree_insert(t, (void*)r)) {
			return test_fail("Post-deletion insert failed\n");
		}
	}

	// Delete elements that don't exist
	for (size_t e = 0; e < n_elems; e++) {
		r = ith_elem(test_num, n_elems * 3, n_elems + e);
		if (!zix_btree_remove(t, (void*)r)) {
			return test_fail("Unexpected successful deletion of %lu\n",
			                 (uintptr_t)r);
		}
	}

	// Ensure tree size is still correct
	if (zix_btree_size(t) != n_elems) {
		return test_fail("Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
	}

	// Delete some elements towards the end
	for (size_t e = 0; e < n_elems / 4; e++) {
		r = ith_elem(test_num, n_elems, n_elems - (n_elems / 4) + e);
		if (zix_btree_remove(t, (void*)r)) {
			return test_fail("Deletion of %lu faileded\n", (uintptr_t)r);
		}
	}

	// Check tree size
	if (zix_btree_size(t) != n_elems - (n_elems / 4)) {
		return test_fail("Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
	}

	zix_btree_free(t);

	return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
	if (argc > 2) {
		fprintf(stderr, "Usage: %s [N_ELEMS]\n", argv[0]);
		return EXIT_FAILURE;
	}

	const unsigned n_tests = 3;
	unsigned       n_elems = (argc > 1) ? atol(argv[1]) : 10000;

	printf("Running %u tests with %u elements", n_tests, n_elems);
	for (unsigned i = 0; i < n_tests; ++i) {
		printf(".");
		fflush(stdout);
		if (stress(i, n_elems)) {
			return EXIT_FAILURE;
		}
	}
	printf("\n");
	return EXIT_SUCCESS;
}
