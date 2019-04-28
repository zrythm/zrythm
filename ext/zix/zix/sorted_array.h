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

#ifndef ZIX_SORTED_ARRAY_H
#define ZIX_SORTED_ARRAY_H

#include "zix/common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name SortedArray
   @{
*/

/**
   A sorted array.
*/
typedef struct ZixSortedArrayImpl ZixSortedArray;

/**
   An iterator over a ZixSortedArray.
*/
typedef void* ZixSortedArrayIter;

/**
   Create a new (empty) sorted array.
*/
ZIX_API
ZixSortedArray*
zix_sorted_array_new(bool allow_duplicates, ZixComparator cmp, void* cmp_data,
                     size_t elem_size);

/**
   Free `a`.
*/
ZIX_API
void
zix_sorted_array_free(ZixSortedArray* a);

/**
   Return the number of elements in `a`.
*/
ZIX_API
size_t
zix_sorted_array_size(ZixSortedArray* a);

/**
   Insert the element `e` into `a` and point `ai` at the new element.
*/
ZIX_API
ZixStatus
zix_sorted_array_insert(ZixSortedArray*     a,
                        const void*         e,
                        ZixSortedArrayIter* ai);

/**
   Remove the item pointed at by `ai` from `a`.
*/
ZIX_API
ZixStatus
zix_sorted_array_remove(ZixSortedArray* a, ZixSortedArrayIter ai);

/**
   Set `ai` to be the largest element <= `e` in `a`.
   If no such item exists, `ai` is set to NULL.
*/
ZIX_API
ZixStatus
zix_sorted_array_find(const ZixSortedArray* a,
                      const void*           e,
                      ZixSortedArrayIter*   ai);

/**
   Return the element at index `index`.
*/
ZIX_API
void*
zix_sorted_array_index(const ZixSortedArray* a, size_t index);

/**
   Return the data associated with the given array item.
*/
ZIX_API
void*
zix_sorted_array_get_data(ZixSortedArrayIter ai);

/**
   Return an iterator to the first (smallest) element in `a`.
*/
ZIX_API
ZixSortedArrayIter
zix_sorted_array_begin(ZixSortedArray* a);

/**
   Return an iterator the the element one past the last element in `a`.
*/
ZIX_API
ZixSortedArrayIter
zix_sorted_array_end(ZixSortedArray* a);

/**
   Return true iff `a` is an iterator to the end of its tree.
*/
ZIX_API
bool
zix_sorted_array_iter_is_end(ZixSortedArray* a, ZixSortedArrayIter i);

/**
   Return an iterator that points to the element one past `a`.
*/
ZIX_API
ZixSortedArrayIter
zix_sorted_array_iter_next(ZixSortedArray* a, ZixSortedArrayIter i);

/**
   @}
   @}
*/

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* ZIX_SORTED_ARRAY_H */
