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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "zix/patree.h"

static const char* strings[] = {
	"http://example.org/foo",
	"http://example.org/bar",
	"http://example.org/baz",
	"http://example.net/foo",
	"http://example.net/bar",
	"http://example.net/baz",
	"http://drobilla.net/",
	"http://drobilla.net/software/zix",
	"http://www.gbengasesan.com/blog",
	"http://www.gbengasesan.com",
	"http://echo.jpl.nasa.gov/~lance/delta_v/delta_v.rendezvous.html",
	"http://echo.jpl.nasa.gov/asteroids/1986da/1986DA.html",
	"http://echo.jpl.nasa.gov/",
	"http://echo.jpl.nasa.gov/asteroids/1620_Geographos/geographos.html",
	"http://echo.jpl.nasa.gov/~ostro/KY26/",
	"http://echo.jpl.nasa.gov/~ostro/KY26/JPL_press_release.text",
	"http://echo.jpl.nasa.gov",
	"http://echo.jpl.nasa.gov/asteroids/4179_Toutatis/toutatis.html",
	"http://echo.jpl.nasa.gov/asteroids/4769_Castalia/cast01.html",
	"http://echo.jpl.nasa.gov/publications/review_abs.html",
};

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
	ZixPatree* patree = zix_patree_new();

	const size_t n_strings = sizeof(strings) / sizeof(char*);

	// Insert each string
	for (size_t i = 0; i < n_strings; ++i) {
		ZixStatus st = zix_patree_insert(patree, strings[i], strlen(strings[i]));
		if (st) {
			return test_fail("Failed to insert `%s'\n", strings[i]);
		}
	}

	FILE* dot_file = fopen("patree.dot", "w");
	zix_patree_print_dot(patree, dot_file);
	fclose(dot_file);

	// Attempt to insert each string again
	for (size_t i = 0; i < n_strings; ++i) {
		ZixStatus st = zix_patree_insert(patree, strings[i], strlen(strings[i]));
		if (st != ZIX_STATUS_EXISTS) {
			return test_fail("Double inserted `%s'\n", strings[i]);
		}
	}

	// Search for each string
	for (size_t i = 0; i < n_strings; ++i) {
		const char* match = NULL;
		ZixStatus   st    = zix_patree_find(patree, strings[i], &match);
		if (st) {
			return test_fail("Failed to find `%s'\n", strings[i]);
		}
		if (match != strings[i]) {
			return test_fail("Bad match for `%s'\n", strings[i]);
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
		const char* match = NULL;
		ZixStatus   st    = zix_patree_find(patree, not_indexed[i], &match);
		if (st != ZIX_STATUS_NOT_FOUND) {
			return test_fail("Unexpectedly found `%s'\n", not_indexed[i]);
		}
	}

	zix_patree_free(patree);

	return 0;
}
