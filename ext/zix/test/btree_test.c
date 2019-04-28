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

#include "test_malloc.h"
#include "zix/zix.h"

static bool expect_failure = false;

// Return a pseudo-pseudo-pseudo-random-ish integer with no duplicates
static uint32_t
unique_rand(uint32_t i)
{
	i ^= 0x00005CA1E;  // Juggle bits to avoid linear clumps

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
	const uintptr_t ia = (uintptr_t)a;
	const uintptr_t ib = (uintptr_t)b;
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
	case 0:  return i + 1;           // Increasing
	case 1:  return n_elems - i;     // Decreasing
	default: return unique_rand(i);  // Pseudo-random
	}
}

static void destroy(void* ptr)
{
}

typedef struct {
	int    test_num;
	size_t n_elems;
} TestContext;

static uint32_t
wildcard_cut(int test_num, size_t n_elems)
{
	return ith_elem(test_num, n_elems, n_elems / 3);
}

/** Wildcard comparator where 0 matches anything >= wildcard_cut(n_elems). */
static int
wildcard_cmp(const void* a, const void* b, void* user_data)
{
	const TestContext* ctx      = (TestContext*)user_data;
	const size_t       n_elems  = ctx->n_elems;
	const int          test_num = ctx->test_num;
	const uintptr_t    ia       = (uintptr_t)a;
	const uintptr_t    ib       = (uintptr_t)b;
	if (ia == 0) {
		if (ib >= wildcard_cut(test_num, n_elems)) {
			return 0;  // Wildcard match
		} else {
			return 1;  // Wildcard a > b
		}
	} else if (ib == 0) {
		if (ia >= wildcard_cut(test_num, n_elems)) {
			return 0;  // Wildcard match
		} else {
			return -1;  // Wildcard b > a
		}
	} else {
		return int_cmp(a, b, user_data);
	}
}

static int
test_fail(ZixBTree* t, const char* fmt, ...)
{
	zix_btree_free(t);
	if (expect_failure) {
		return EXIT_SUCCESS;
	}
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
	uintptr_t     r  = 0;
	ZixBTreeIter* ti = NULL;
	ZixBTree*     t  = zix_btree_new(int_cmp, NULL, NULL);
	ZixStatus     st = ZIX_STATUS_SUCCESS;

	if (!t) {
		return test_fail(t, "Failed to allocate tree\n");
	}

	// Ensure begin iterator is end on empty tree
	ti = zix_btree_begin(t);
	if (!ti) {
		return test_fail(t, "Failed to allocate iterator\n");
	} else if (!zix_btree_iter_is_end(ti)) {
		return test_fail(t, "Begin iterator on empty tree is not end\n");
	}
	zix_btree_iter_free(ti);

	// Insert n_elems elements
	for (size_t i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (!zix_btree_find(t, (void*)r, &ti)) {
			return test_fail(t, "%lu already in tree\n", (uintptr_t)r);
		} else if ((st = zix_btree_insert(t, (void*)r))) {
			return test_fail(t, "Insert %lu failed (%s)\n",
			                 (uintptr_t)r, zix_strerror(st));
		}
	}

	// Ensure tree size is correct
	if (zix_btree_size(t) != n_elems) {
		return test_fail(t, "Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
	}

	// Search for all elements
	for (size_t i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (zix_btree_find(t, (void*)r, &ti)) {
			return test_fail(t, "Find %lu @ %zu failed\n", (uintptr_t)r, i);
		} else if ((uintptr_t)zix_btree_get(ti) != r) {
			return test_fail(t, "Search data corrupt (%" PRIdPTR " != %" PRIdPTR ")\n",
			                 (uintptr_t)zix_btree_get(ti), r);
		}
		zix_btree_iter_free(ti);
	}

	if (zix_btree_lower_bound(NULL, (void*)r, &ti) != ZIX_STATUS_BAD_ARG) {
		return test_fail(t, "Lower bound on NULL tree succeeded\n");
	}

	// Find the lower bound of all elements and ensure it's exact
	for (size_t i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (zix_btree_lower_bound(t, (void*)r, &ti)) {
			return test_fail(t, "Lower bound %lu @ %zu failed\n", (uintptr_t)r, i);
		} else if (zix_btree_iter_is_end(ti)) {
			return test_fail(t, "Lower bound %lu @ %zu hit end\n", (uintptr_t)r, i);
		} else if ((uintptr_t)zix_btree_get(ti) != r) {
			return test_fail(t, "Lower bound corrupt (%" PRIdPTR " != %" PRIdPTR "\n",
			                 (uintptr_t)zix_btree_get(ti), r);
		}
		zix_btree_iter_free(ti);
	}

	// Search for elements that don't exist
	for (size_t i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems * 3, n_elems + i);
		if (!zix_btree_find(t, (void*)r, &ti)) {
			return test_fail(t, "Unexpectedly found %lu\n", (uintptr_t)r);
		}
	}

	// Iterate over all elements
	size_t        i    = 0;
	uintptr_t     last = 0;
	for (ti = zix_btree_begin(t);
	     !zix_btree_iter_is_end(ti);
	     zix_btree_iter_increment(ti), ++i) {
		const uintptr_t iter_data = (uintptr_t)zix_btree_get(ti);
		if (iter_data < last) {
			return test_fail(t, "Iter @ %zu corrupt (%" PRIdPTR " < %" PRIdPTR ")\n",
			                 i, iter_data, last);
		}
		last = iter_data;
	}
	zix_btree_iter_free(ti);
	if (i != n_elems) {
		return test_fail(t, "Iteration stopped at %zu/%zu elements\n", i, n_elems);
	}

	// Insert n_elems elements again, ensuring duplicates fail
	for (i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (!zix_btree_insert(t, (void*)r)) {
			return test_fail(t, "Duplicate insert succeeded\n");
		}
	}

	// Search for the middle element then iterate from there
	r = ith_elem(test_num, n_elems, n_elems / 2);
	if (zix_btree_find(t, (void*)r, &ti)) {
		return test_fail(t, "Find %lu failed\n", (uintptr_t)r);
	}
	last = (uintptr_t)zix_btree_get(ti);
	zix_btree_iter_increment(ti);
	for (i = 1; !zix_btree_iter_is_end(ti); zix_btree_iter_increment(ti), ++i) {
		if ((uintptr_t)zix_btree_get(ti) == last) {
			return test_fail(t, "Duplicate element @ %u %" PRIdPTR "\n", i, last);
		}
		last = (uintptr_t)zix_btree_get(ti);
	}
	zix_btree_iter_free(ti);

	// Delete all elements
	ZixBTreeIter* next = NULL;
	for (size_t e = 0; e < n_elems; e++) {
		r = ith_elem(test_num, n_elems, e);
		uintptr_t removed;
		if (zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
			return test_fail(t, "Error removing item %lu\n", (uintptr_t)r);
		} else if (removed != r) {
			return test_fail(t, "Removed wrong item %lu != %lu\n",
			                 removed, (uintptr_t)r);
		} else if (test_num == 0) {
			const uintptr_t next_value = ith_elem(test_num, n_elems, e + 1);
			if (!((zix_btree_iter_is_end(next) && e == n_elems - 1) ||
			      (uintptr_t)zix_btree_get(next) == next_value)) {
				return test_fail(t, "Delete all next iterator %lu != %lu\n",
				                 (uintptr_t)zix_btree_get(next), next_value);
			}
		}
	}
	zix_btree_iter_free(next);
	next = NULL;

	// Ensure the tree is empty
	if (zix_btree_size(t) != 0) {
		return test_fail(t, "Tree size %zu != 0\n", zix_btree_size(t));
	}

	// Insert n_elems elements again (to test non-empty destruction)
	for (size_t e = 0; e < n_elems; ++e) {
		r = ith_elem(test_num, n_elems, e);
		if (zix_btree_insert(t, (void*)r)) {
			return test_fail(t, "Post-deletion insert failed\n");
		}
	}

	// Delete elements that don't exist
	for (size_t e = 0; e < n_elems; e++) {
		r = ith_elem(test_num, n_elems * 3, n_elems + e);
		uintptr_t removed;
		if (!zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
			return test_fail(t, "Non-existant deletion of %lu succeeded\n", (uintptr_t)r);
		}
	}
	zix_btree_iter_free(next);
	next = NULL;

	// Ensure tree size is still correct
	if (zix_btree_size(t) != n_elems) {
		return test_fail(t, "Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
	}

	// Delete some elements towards the end
	for (size_t e = 0; e < n_elems / 4; e++) {
		r = ith_elem(test_num, n_elems, n_elems - (n_elems / 4) + e);
		uintptr_t     removed;
		if (zix_btree_remove(t, (void*)r, (void**)&removed, &next)) {
			return test_fail(t, "Deletion of %lu failed\n", (uintptr_t)r);
		} else if (removed != r) {
			return test_fail(t, "Removed wrong item %lu != %lu\n",
			                 removed, (uintptr_t)r);
		} else if (test_num == 0) {
			const uintptr_t next_value = ith_elem(test_num, n_elems, e + 1);
			if (!zix_btree_iter_is_end(next) &&
			    (uintptr_t)zix_btree_get(next) == next_value) {
				return test_fail(t, "Next iterator %lu != %lu\n",
				                 (uintptr_t)zix_btree_get(next), next_value);
			}
		}
	}
	zix_btree_iter_free(next);
	next = NULL;

	// Check tree size
	if (zix_btree_size(t) != n_elems - (n_elems / 4)) {
		return test_fail(t, "Tree size %zu != %zu\n", zix_btree_size(t), n_elems);
	}

	// Delete some elements in a random order
	for (size_t e = 0; e < zix_btree_size(t); e++) {
		r = ith_elem(test_num, n_elems, rand() % n_elems);
		uintptr_t removed;
		ZixStatus rst = zix_btree_remove(t, (void*)r, (void**)&removed, &next);
		if (rst != ZIX_STATUS_SUCCESS && rst != ZIX_STATUS_NOT_FOUND) {
			return test_fail(t, "Error deleting %lu\n", (uintptr_t)r);
		}
	}
	zix_btree_iter_free(next);
	next = NULL;

	zix_btree_free(t);

	// Test lower_bound with wildcard comparator

	TestContext ctx = { test_num, n_elems };
	if (!(t = zix_btree_new(wildcard_cmp, &ctx, destroy))) {
		return test_fail(t, "Failed to allocate tree\n");
	}

	// Insert n_elems elements
	for (i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (r != 0) {  // Can't insert wildcards
			if ((st = zix_btree_insert(t, (void*)r))) {
				return test_fail(t, "Insert %lu failed (%s)\n",
				                 (uintptr_t)r, zix_strerror(st));
			}
		}
	}

	// Find lower bound of wildcard
	const uintptr_t wildcard = 0;
	if (zix_btree_lower_bound(t, (void*)wildcard, &ti)) {
		return test_fail(t, "Lower bound failed\n");
	} else if (zix_btree_iter_is_end(ti)) {
		return test_fail(t, "Lower bound reached end\n");
	}

	// Check value
	const uintptr_t iter_data = (uintptr_t)zix_btree_get(ti);
	const uintptr_t cut       = wildcard_cut(test_num, n_elems);
	if (iter_data != cut) {
		return test_fail(t, "Lower bound %" PRIdPTR " != %" PRIdPTR "\n",
		                 iter_data, cut);
	} else if (wildcard_cmp((void*)wildcard, (void*)iter_data, &ctx)) {
		return test_fail(t, "Wildcard lower bound %" PRIdPTR " != %" PRIdPTR "\n",
		                 iter_data, cut);
	}

	zix_btree_iter_free(ti);

	// Find lower bound of value past end
	const uintptr_t max = (uintptr_t)-1;
	if (zix_btree_lower_bound(t, (void*)max, &ti)) {
		return test_fail(t, "Lower bound failed\n");
	} else if (!zix_btree_iter_is_end(ti)) {
		return test_fail(t, "Lower bound of maximum value is not end\n");
	}

	zix_btree_iter_free(ti);
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
	unsigned       n_elems = (argc > 1) ? atol(argv[1]) : 100000;

	printf("Running %u tests with %u elements", n_tests, n_elems);
	for (unsigned i = 0; i < n_tests; ++i) {
		printf(".");
		fflush(stdout);
		if (stress(i, n_elems)) {
			return EXIT_FAILURE;
		}
	}
	printf("\n");

	const size_t total_n_allocs = test_malloc_get_n_allocs();
	const size_t fail_n_elems   = 1000;
	printf("Testing 0 ... %zu failed allocations\n", total_n_allocs);
	expect_failure = true;
	for (size_t i = 0; i < total_n_allocs; ++i) {
		test_malloc_reset(i);
		stress(0, fail_n_elems);
		if (i > test_malloc_get_n_allocs()) {
			break;
		}
	}

	test_malloc_reset((size_t)-1);

	return EXIT_SUCCESS;
}
