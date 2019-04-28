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

#include "bench.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "zix/ampatree.h"
#include "zix/chunk.h"
#include "zix/hash.h"
#include "zix/patree.h"
#include "zix/trie.h"

const unsigned seed = 1;

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
	if (argc != 2) {
		return test_fail("Usage: %s INPUT_FILE\n", argv[0]);
	}

	const char* file = argv[1];
	FILE*       fd   = fopen(file, "r");
	if (!fd) {
		return test_fail("Failed to open file %s\n", file);
	}

	size_t max_n_strings = 100000000;

	/* Read input strings */
	char**  strings      = NULL;
	size_t* lengths      = NULL;
	size_t  n_strings    = 0;
	char*   buf          = calloc(1, 1);
	size_t  buf_len      = 1;
	size_t  this_str_len = 0;
	for (char c; (c = fgetc(fd)) != EOF;) {
		if (isspace(c)) {
			if (this_str_len == 0) {
				continue;
			}
			strings = realloc(strings, (n_strings + 1) * sizeof(char*));
			lengths = realloc(lengths, (n_strings + 1) * sizeof(size_t));
			strings[n_strings] = malloc(buf_len);
			lengths[n_strings] = this_str_len;
			memcpy(strings[n_strings], buf, buf_len);
			this_str_len = 0;
			if (++n_strings == max_n_strings) {
				break;
			}
		} else {
			++this_str_len;
			if (buf_len < this_str_len + 1) {
				buf_len = this_str_len + 1;
				buf = realloc(buf, buf_len);
			}
			buf[this_str_len - 1] = c;
			buf[this_str_len] = '\0';
		}
	}

	fclose(fd);

	FILE* insert_dat = fopen("dict_insert.txt", "w");
	FILE* search_dat = fopen("dict_search.txt", "w");
	fprintf(insert_dat, "# n\tGHashTable\tZixHash\tZixPatree\tZixTrie\tZixAMPatree\n");
	fprintf(search_dat, "# n\tGHashTable\tZixHash\tZixPatree\tZixTrie\tZixAMPatree\n");

	for (size_t n = n_strings / 16; n <= n_strings; n *= 2) {
		printf("Benchmarking n = %zu\n", n);
		ZixPatree*   patree   = zix_patree_new();
		ZixAMPatree* ampatree = zix_ampatree_new();
		ZixTrie*     trie     = zix_trie_new();
		GHashTable*  hash     = g_hash_table_new(g_str_hash, g_str_equal);
		ZixHash*     zhash    = zix_hash_new((ZixHashFunc)zix_chunk_hash,
		                                     (ZixEqualFunc)zix_chunk_equal,
		                                     sizeof(ZixChunk));
		fprintf(insert_dat, "%zu", n);
		fprintf(search_dat, "%zu", n);

		// Benchmark insertion

		// GHashTable
		struct timespec insert_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			g_hash_table_insert(hash, strings[i], strings[i]);
		}
		fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

		// ZixHash
		insert_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			const ZixChunk chunk = { strings[i], lengths[i] + 1 };
			ZixStatus st = zix_hash_insert(zhash, &chunk, NULL);
			if (st && st != ZIX_STATUS_EXISTS) {
				return test_fail("Failed to insert `%s'\n", strings[i]);
			}
		}
		fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

		// ZixPatree
		insert_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			ZixStatus st = zix_patree_insert(patree, strings[i], lengths[i]);
			if (st && st != ZIX_STATUS_EXISTS) {
				return test_fail("Failed to insert `%s'\n", strings[i]);
			}
		}
		fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

		// ZixTrie
		insert_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			ZixStatus st = zix_trie_insert(trie, strings[i], lengths[i]);
			if (st && st != ZIX_STATUS_EXISTS) {
				return test_fail("Failed to insert `%s'\n", strings[i]);
			}
		}
		fprintf(insert_dat, "\t%lf", bench_end(&insert_start));

		// ZixAMPatree
		insert_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			ZixStatus st = zix_ampatree_insert(ampatree, strings[i], lengths[i]);
			if (st && st != ZIX_STATUS_EXISTS) {
				return test_fail("Failed to insert `%s'\n", strings[i]);
			}
		}
		fprintf(insert_dat, "\t%lf\n", bench_end(&insert_start));

		// Benchmark search

		// GHashTable
		srand(seed);
		struct timespec search_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			const size_t index = rand() % n;
			char* match = g_hash_table_lookup(hash, strings[index]);
			if (strcmp(match, strings[index])) {
				return test_fail("Bad match for `%s'\n", strings[index]);
			}
		}
		fprintf(search_dat, "\t%lf", bench_end(&search_start));

		// ZixHash
		srand(seed);
		search_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			const size_t    index = rand() % n;
			const ZixChunk  key   = { strings[index], lengths[index] + 1 };
			const ZixChunk* match = NULL;
			if (!(match = zix_hash_find(zhash, &key))) {
				return test_fail("Hash: Failed to find `%s'\n", strings[index]);
			}
			if (strcmp(match->buf, strings[index])) {
				return test_fail("Hash: Bad match %p for `%s': `%s'\n",
				                 (void*)match, strings[index], match->buf);
			}
		}
		fprintf(search_dat, "\t%lf", bench_end(&search_start));

		// ZixPatree
		srand(seed);
		search_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			const size_t index = rand() % n;
			const char* match = NULL;
			if (zix_patree_find(patree, strings[index], &match)) {
				return test_fail("Patree: Failed to find `%s'\n", strings[index]);
			}
			if (strcmp(match, strings[index])) {
				return test_fail("Patree: Bad match for `%s'\n", strings[index]);
			}
		}
		fprintf(search_dat, "\t%lf", bench_end(&search_start));

		// ZixTrie
		srand(seed);
		search_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			const size_t index = rand() % n;
			const char* match = NULL;
			if (zix_trie_find(trie, strings[index], &match)) {
				return test_fail("Trie: Failed to find `%s'\n", strings[index]);
			}
			if (strcmp(match, strings[index])) {
				return test_fail("Trie: Bad match `%s' for `%s'\n",
				                 match, strings[index]);
			}
		}
		fprintf(search_dat, "\t%lf", bench_end(&search_start));

		// ZixAMPatree
		srand(seed);
		search_start = bench_start();
		for (size_t i = 0; i < n; ++i) {
			const size_t index = rand() % n;
			const char* match = NULL;
			if (zix_ampatree_find(ampatree, strings[index], &match)) {
				return test_fail("AMPatree: Failed to find `%s'\n", strings[index]);
			}
			if (strcmp(match, strings[index])) {
				return test_fail("AMPatree: Bad match `%s' for `%s'\n",
				                 match, strings[index]);
			}
		}
		fprintf(search_dat, "\t%lf\n", bench_end(&search_start));

		zix_patree_free(patree);
		zix_ampatree_free(ampatree);
		zix_trie_free(trie);
		zix_hash_free(zhash);
		g_hash_table_unref(hash);
	}

	fclose(insert_dat);
	fclose(search_dat);

	fprintf(stderr, "Wrote dict_insert.txt dict_search.txt\n");

	return 0;
}
