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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "test_malloc.h"
#include "zix/hash.h"

static bool expect_failure = false;

static const char* strings[] = {
	"one", "two", "three", "four", "five", "six", "seven", "eight",
	"2one", "2two", "2three", "2four", "2five", "2six", "2seven", "2eight",
	"3one", "3two", "3three", "3four", "3five", "3six", "3seven", "3eight",
	"4one", "4two", "4three", "4four", "4five", "4six", "4seven", "4eight",
	"5one", "5two", "5three", "5four", "5five", "5six", "5seven", "5eight",
	"6one", "6two", "6three", "6four", "6five", "6six", "6seven", "6eight",
	"7one", "7two", "7three", "7four", "7five", "7six", "7seven", "7eight",
	"8one", "8two", "8three", "8four", "8five", "8six", "8seven", "8eight",
	"9one", "9two", "9three", "9four", "9five", "9six", "9seven", "9eight",
	"Aone", "Atwo", "Athree", "Afour", "Afive", "Asix", "Aseven", "Aeight",
	"Bone", "Btwo", "Bthree", "Bfour", "Bfive", "Bsix", "Bseven", "Beight",
	"Cone", "Ctwo", "Cthree", "Cfour", "Cfive", "Csix", "Cseven", "Ceight",
	"Done", "Dtwo", "Dthree", "Dfour", "Dfive", "Dsix", "Dseven", "Deight",
};

static int
test_fail(const char* fmt, ...)
{
	if (expect_failure) {
		return 0;
	}
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
	return 1;
}

static unsigned n_checked = 0;

static void
check(void* value, void* user_data)
{
	if (strlen(*(const char*const*)value) >= 3) {
		++n_checked;
	} else {
		fprintf(stderr, "ERROR: %s\n", (const char*)value);
	}
}

static uint32_t
string_ptr_hash(const void* value)
{
	// Trusty old DJB hash
	const char* str = *(const char*const*)value;
	unsigned    h   = 5381;
	for (const char* s = str; *s != '\0'; ++s) {
		h = (h << 5) + h + *s;  // h = h * 33 + c
	}
	return h;
}

static bool
string_ptr_equal(const void* a, const void* b)
{
	return !strcmp(*(const char*const*)a, *(const char*const*)b);
}

static int
stress(void)
{
	ZixHash* hash = zix_hash_new(
		string_ptr_hash, string_ptr_equal, sizeof(const char*));
	if (!hash) {
		return test_fail("Failed to allocate hash\n");
	}

	const size_t n_strings = sizeof(strings) / sizeof(const char*);

	// Insert each string
	for (size_t i = 0; i < n_strings; ++i) {
		const void* inserted = NULL;
		ZixStatus   st       = zix_hash_insert(hash, &strings[i], &inserted);
		if (st) {
			return test_fail("Failed to insert `%s'\n", strings[i]);
		} else if (*(const void*const*)inserted != strings[i]) {
			return test_fail("Corrupt insertion %s != %s\n",
			                 strings[i], *(const char*const*)inserted);
		}
	}

	// Ensure hash size is correct
	if (zix_hash_size(hash) != n_strings) {
		return test_fail("Hash size %zu != %zu\n",
		                 zix_hash_size(hash), n_strings);
	}

	//zix_hash_print_dot(hash, stdout);

	// Attempt to insert each string again
	for (size_t i = 0; i < n_strings; ++i) {
		const void* inserted = NULL;
		ZixStatus   st       = zix_hash_insert(hash, &strings[i], &inserted);
		if (st != ZIX_STATUS_EXISTS) {
			return test_fail("Double inserted `%s'\n", strings[i]);
		}
	}

	// Search for each string
	for (size_t i = 0; i < n_strings; ++i) {
		const char*const* match = (const char*const*)zix_hash_find(
			hash, &strings[i]);
		if (!match) {
			return test_fail("Failed to find `%s'\n", strings[i]);
		}
		if (*match != strings[i]) {
			return test_fail("Bad match for `%s': `%s'\n", strings[i], match);
		}
	}

	// Try some false matches
	const char* not_indexed[] = {
		"ftp://example.org/not-there-at-all",
		"http://example.org/foobar",
		"http://",
		"http://otherdomain.com"
	};
	const size_t n_not_indexed = sizeof(not_indexed) / sizeof(char*);
	for (size_t i = 0; i < n_not_indexed; ++i) {
		const char*const* match = (const char*const*)zix_hash_find(
			hash, &not_indexed[i]);
		if (match) {
			return test_fail("Unexpectedly found `%s'\n", not_indexed[i]);
		}
	}

	// Remove strings
	for (size_t i = 0; i < n_strings; ++i) {
		// Remove string
		ZixStatus st = zix_hash_remove(hash, &strings[i]);
		if (st) {
			return test_fail("Failed to remove `%s'\n", strings[i]);
		}

		// Ensure second removal fails
		st = zix_hash_remove(hash, &strings[i]);
		if (st != ZIX_STATUS_NOT_FOUND) {
			return test_fail("Unexpectedly removed `%s' twice\n", strings[i]);
		}

		// Check to ensure remaining strings are still present
		for (size_t j = i + 1; j < n_strings; ++j) {
			const char*const* match = (const char*const*)zix_hash_find(
				hash, &strings[j]);
			if (!match) {
				return test_fail("Failed to find `%s' after remove\n", strings[j]);
			}
			if (*match != strings[j]) {
				return test_fail("Bad match for `%s' after remove\n", strings[j]);
			}
		}
	}

	// Insert each string again (to check non-empty desruction)
	for (size_t i = 0; i < n_strings; ++i) {
		ZixStatus st = zix_hash_insert(hash, &strings[i], NULL);
		if (st) {
			return test_fail("Failed to insert `%s'\n", strings[i]);
		}
	}

	// Check key == value (and test zix_hash_foreach)
	zix_hash_foreach(hash, check, NULL);
	if (n_checked != n_strings) {
		return test_fail("Check failed\n");
	}

	zix_hash_free(hash);

	return 0;
}

int
main(int argc, char** argv)
{
	if (stress()) {
		return 1;
	}

	const size_t total_n_allocs = test_malloc_get_n_allocs();
	printf("Testing 0 ... %zu failed allocations\n", total_n_allocs);
	expect_failure = true;
	for (size_t i = 0; i < total_n_allocs; ++i) {
		test_malloc_reset(i);
		stress();
	}

	test_malloc_reset((size_t)-1);

	return 0;
}
