/*
  Copyright 2012-2019 David Robillard <http://drobilla.net>

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

/**
   @file implementation.h Shared declarations for implementation.
*/

#ifndef PUGL_DETAIL_IMPLEMENTATION_H
#define PUGL_DETAIL_IMPLEMENTATION_H

#include "pugl/detail/types.h"
#include "pugl/pugl.h"

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Set `blob` to `data` with length `len`, reallocating if necessary. */
void puglSetBlob(PuglBlob* blob, const void* data, size_t len);

/** Reallocate and set `*dest` to `string`. */
void puglSetString(char** dest, const char* string);

/** Allocate and initialise world internals (implemented once per platform) */
PuglWorldInternals* puglInitWorldInternals(void);

/** Destroy and free world internals (implemented once per platform) */
void puglFreeWorldInternals(PuglWorld* world);

/** Allocate and initialise view internals (implemented once per platform) */
PuglInternals* puglInitViewInternals(void);

/** Destroy and free view internals (implemented once per platform) */
void puglFreeViewInternals(PuglView* view);

/** Return the Unicode code point for `buf` or the replacement character. */
uint32_t puglDecodeUTF8(const uint8_t* buf);

/** Dispatch `event` to `view`, optimising configure/expose if possible. */
void puglDispatchEvent(PuglView* view, const PuglEvent* event);

/** Set internal (stored in view) clipboard contents. */
const void*
puglGetInternalClipboard(const PuglView* view, const char** type, size_t* len);

/** Set internal (stored in view) clipboard contents. */
PuglStatus
puglSetInternalClipboard(PuglView*   view,
                         const char* type,
                         const void* data,
                         size_t      len);

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif // PUGL_DETAIL_IMPLEMENTATION_H
