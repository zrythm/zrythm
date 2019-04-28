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

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zix/common.h"
#include "zix/sorted_array.h"

// #define ZIX_SORTED_ARRAY_DUMP 1

struct ZixSortedArrayImpl {
	void*         array;
	ZixComparator cmp;
	void*         cmp_data;
	size_t        elem_size;
	size_t        num_elems;
	bool          allow_duplicates;
};

#ifdef ZIX_SORTED_ARRAY_DUMP
static void
zix_sorted_array_print(ZixSortedArray* a)
{
	for (size_t i = 0; i < a->num_elems; ++i) {
		printf(" %ld", *(intptr_t*)((char*)a->array + (i * a->elem_size)));
	}
	printf("\n");
}
#    define DUMP(a) zix_sorted_array_print(a)
#else
#    define DUMP(a)
#endif

ZIX_API
ZixSortedArray*
zix_sorted_array_new(bool          allow_duplicates,
                     ZixComparator cmp,
                     void*         cmp_data,
                     size_t        elem_size)
{
	ZixSortedArray* a = (ZixSortedArray*)malloc(sizeof(ZixSortedArray));
	a->array            = NULL;
	a->cmp              = cmp;
	a->cmp_data         = cmp_data;
	a->elem_size        = elem_size;
	a->num_elems        = 0;
	a->allow_duplicates = allow_duplicates;
	return a;
}

ZIX_API
void
zix_sorted_array_free(ZixSortedArray* a)
{
	free(a->array);
	free(a);
}

ZIX_API
size_t
zix_sorted_array_size(ZixSortedArray* a)
{
	return a->num_elems;
}

ZIX_API
ZixStatus
zix_sorted_array_insert(ZixSortedArray*     a,
                        const void*         e,
                        ZixSortedArrayIter* ai)
{
	if (a->num_elems == 0) {
		assert(!a->array);
		a->array = malloc(a->elem_size);
		memcpy(a->array, e, a->elem_size);
		++a->num_elems;
		*ai = a->array;
		return ZIX_STATUS_SUCCESS;
	}

	zix_sorted_array_find(a, e, ai);
	assert(*ai);
	const size_t i = ((char*)*ai - (char*)a->array) / a->elem_size;

	a->array = realloc(a->array, ++a->num_elems * a->elem_size);
	memmove((char*)a->array + ((i + 1) * a->elem_size),
	        (char*)a->array + (i * a->elem_size),
	        (a->num_elems - i - 1) * a->elem_size);
	memcpy((char*)a->array + (i * a->elem_size),
	       e,
	       a->elem_size);

	*ai = (char*)a->array + (i * a->elem_size);
	DUMP(t);
	return ZIX_STATUS_SUCCESS;
}

ZIX_API
ZixStatus
zix_sorted_array_remove(ZixSortedArray* a, ZixSortedArrayIter ai)
{
	const size_t i = ((char*)ai - (char*)a->array) / a->elem_size;
	memmove((char*)a->array + (i * a->elem_size),
	        (char*)a->array + ((i + 1) * a->elem_size),
	        (a->num_elems - i - 1) * a->elem_size);
	--a->num_elems;
	DUMP(a);
	return ZIX_STATUS_SUCCESS;
}

static inline void*
zix_sorted_array_index_unchecked(const ZixSortedArray* a, size_t index)
{
	return (char*)a->array + (a->elem_size * index);
}

ZIX_API
void*
zix_sorted_array_index(const ZixSortedArray* a, size_t index)
{
	if (index >= a->num_elems) {
		return NULL;
	}

	return zix_sorted_array_index_unchecked(a, index);
}

ZIX_API
ZixStatus
zix_sorted_array_find(const ZixSortedArray* a,
                      const void*           e,
                      ZixSortedArrayIter*   ai)
{
	intptr_t lower = 0;
	intptr_t upper = a->num_elems - 1;
	while (upper >= lower) {
		const intptr_t i      = lower + ((upper - lower) / 2);
		void* const    elem_i = zix_sorted_array_index_unchecked(a, i);
		const int      cmp    = a->cmp(elem_i, e, a->cmp_data);

		if (cmp == 0) {
			*ai = elem_i;
			return ZIX_STATUS_SUCCESS;
		} else if (cmp > 0) {
			upper = i - 1;
		} else {
			lower = i + 1;
		}
	}

	*ai = zix_sorted_array_index_unchecked(a, lower);
	return ZIX_STATUS_NOT_FOUND;
}

ZIX_API
void*
zix_sorted_array_get_data(ZixSortedArrayIter ai)
{
	return ai;
}

ZIX_API
ZixSortedArrayIter
zix_sorted_array_begin(ZixSortedArray* a)
{
	return a->array;
}

ZIX_API
ZixSortedArrayIter
zix_sorted_array_end(ZixSortedArray* a)
{
	return (char*)a->array + (a->elem_size * a->num_elems);
}

ZIX_API
bool
zix_sorted_array_iter_is_end(ZixSortedArray* a, ZixSortedArrayIter i)
{
	return i != zix_sorted_array_end(a);
}

ZIX_API
ZixSortedArrayIter
zix_sorted_array_iter_next(ZixSortedArray* a, ZixSortedArrayIter i)
{
	return (char*)i + a->elem_size;
}
