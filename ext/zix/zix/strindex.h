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

#ifndef ZIX_STRINDEX_H
#define ZIX_STRINDEX_H

#include "zix/common.h"

/**
   @addtogroup zix
   @{
   @name Strindex
   @{
*/

typedef struct _ZixStrindex ZixStrindex;

/**
   Construct a new strindex that contains all suffixes of the string `s`.
   A copy of `s` is taken and stored for the lifetime of the strindex.
*/
ZIX_API
ZixStrindex*
zix_strindex_new(const char* s);

/**
   Destroy `t`.
*/
ZIX_API
void
zix_strindex_free(ZixStrindex* strindex);

/**
   Check if the string in `strindex` contains the substring `str`.
   If such a substring is found, `match` is pointed at an occurrence of it.
*/
ZIX_API
ZixStatus
zix_strindex_find(ZixStrindex* strindex, const char* str, char** match);

/**
   @}
   @}
*/

#endif  /* ZIX_STRINDEX_H */
