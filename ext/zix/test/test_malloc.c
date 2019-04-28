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

#define _GNU_SOURCE

#include <dlfcn.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <assert.h>
#include <stdio.h>

#include "test_malloc.h"

static void* (*test_malloc_sys_malloc)(size_t size) = NULL;

static size_t        test_malloc_n_allocs   = 0;
static size_t        test_malloc_fail_after = (size_t)-1;
static volatile bool in_test_malloc_init    = false;

void*
malloc(size_t size)
{
	if (in_test_malloc_init) {
		return NULL;  // dlsym is asking for memory, but handles this fine
	} else if (!test_malloc_sys_malloc) {
		test_malloc_init();
	}

	if (test_malloc_n_allocs < test_malloc_fail_after) {
		++test_malloc_n_allocs;
		return test_malloc_sys_malloc(size);
	}

	return NULL;
}

void*
calloc(size_t nmemb, size_t size)
{
	void* ptr = malloc(nmemb * size);
	if (ptr) {
		memset(ptr, 0, nmemb * size);
	}
	return ptr;
}

void
test_malloc_reset(size_t fail_after)
{
	test_malloc_fail_after = fail_after;
	test_malloc_n_allocs   = 0;
}

void
test_malloc_init(void)
{
	in_test_malloc_init = true;

	/* Avoid pedantic warnings about casting pointer to function pointer by
	   casting dlsym instead. */

	typedef void*      (*MallocFunc)(size_t);
	typedef MallocFunc (*MallocFuncGetter)(void*, const char*);

	MallocFuncGetter dlfunc = (MallocFuncGetter)dlsym;
	test_malloc_sys_malloc = (MallocFunc)dlfunc(RTLD_NEXT, "malloc");

	in_test_malloc_init = false;
}

size_t
test_malloc_get_n_allocs(void)
{
	return test_malloc_n_allocs;
}
