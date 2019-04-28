/*
  Copyright 2011 David Robillard <http://drobilla.net>

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
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifdef _MSC_VER
#    define PRIdPTR "Id"
#else
#    include <inttypes.h>
#endif

#include "zix/common.h"
#include "zix/sorted_array.h"

unsigned seed = 1;

static int
int_cmp(const void* a, const void* b, void* user_data)
{
	const intptr_t ia = *(const intptr_t*)a;
	const intptr_t ib = *(const intptr_t*)b;
	return ia - ib;
}

static int
ith_elem(int test_num, unsigned n_elems, int i)
{
	switch (test_num % 3) {
	case 0:
		return i;  // Increasing (worst case)
	case 1:
		return n_elems - i;  // Decreasing (worse case)
	case 2:
	default:
		return rand() % 100;  // Random
	}
}

static int
test_fail(void)
{
	return EXIT_FAILURE;
}

static int
stress(int test_num, unsigned n_elems)
{
	intptr_t    r;
	ZixSortedArrayIter ti;

	ZixSortedArray* t = zix_sorted_array_new(true, int_cmp, NULL, sizeof(intptr_t));

	srand(seed);

	// Insert n_elems elements
	for (unsigned i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		int status = zix_sorted_array_insert(t, &r, &ti);
		if (status) {
			fprintf(stderr, "Insert failed\n");
			return test_fail();
		}
		if (*(intptr_t*)zix_sorted_array_get_data(ti) != r) {
			fprintf(stderr, "Data corrupt (%" PRIdPTR " != %" PRIdPTR ")\n",
			        *(intptr_t*)zix_sorted_array_get_data(ti), r);
			return test_fail();
		}
	}

	srand(seed);

	// Search for all elements
	for (unsigned i = 0; i < n_elems; ++i) {
		r = ith_elem(test_num, n_elems, i);
		if (zix_sorted_array_find(t, &r, &ti)) {
			fprintf(stderr, "Find failed\n");
			return test_fail();
		}
		if (*(intptr_t*)zix_sorted_array_get_data(ti) != r) {
			fprintf(stderr, "Data corrupt (%" PRIdPTR " != %" PRIdPTR ")\n",
			        *(intptr_t*)zix_sorted_array_get_data(ti), r);
			return test_fail();
		}
	}

	srand(seed);

	// Iterate over all elements
	unsigned i    = 0;
	intptr_t last = -1;
	for (ZixSortedArrayIter iter = zix_sorted_array_begin(t);
	     !zix_sorted_array_iter_is_end(t, iter);
	     iter = zix_sorted_array_iter_next(t, iter), ++i) {
		r = ith_elem(test_num, n_elems, i);
		const intptr_t iter_data = *(intptr_t*)zix_sorted_array_get_data(iter);
		if (iter_data < last) {
			fprintf(stderr, "Iter corrupt (%" PRIdPTR " < %" PRIdPTR ")\n",
			        iter_data, last);
			return test_fail();
		}
		last = iter_data;
	}

	srand(seed);

	// Delete all elements
	for (unsigned e = 0; e < n_elems; e++) {
		r = ith_elem(test_num, n_elems, e);
		ZixSortedArrayIter item;
		if (zix_sorted_array_find(t, &r, &item) != ZIX_STATUS_SUCCESS) {
			fprintf(stderr, "Failed to find item to remove\n");
			return test_fail();
		}
		if (zix_sorted_array_remove(t, item)) {
			fprintf(stderr, "Error removing item\n");
		}
	}

	if (zix_sorted_array_size(t) != 0) {
		fprintf(stderr, "Array is not empty after removing all items\n");
		return test_fail();
	}

	zix_sorted_array_free(t);

	return EXIT_SUCCESS;
}

int
main(int argc, char** argv)
{
	const unsigned n_tests = 3;
	unsigned       n_elems = 0;

	if (argc == 1) {
		n_elems = 4096;
	} else {
		n_elems = atol(argv[1]);
		if (argc > 2) {
			seed = atol(argv[2]);
		} else {
			seed = time(NULL);
		}
	}

	if (n_elems == 0) {
		fprintf(stderr, "USAGE: %s [N_ELEMS]\n", argv[0]);
		return 1;
	}

	printf("Running %u tests with %u elements (seed %d)",
	       n_tests, n_elems, seed);

	for (unsigned i = 0; i < n_tests; ++i) {
		printf(".");
		fflush(stdout);
		if (stress(i, n_elems)) {
			fprintf(stderr, "FAIL: Random seed %u\n", seed);
			return test_fail();
		}
	}
	printf("\n");
	return EXIT_SUCCESS;
}
