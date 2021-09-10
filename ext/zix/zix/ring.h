/*
  Copyright 2011-2020 David Robillard <d@drobilla.net>

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

#ifndef ZIX_RING_H
#define ZIX_RING_H

#include "zix/common.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   @addtogroup zix
   @{
   @name Ring
   @{
*/

/**
   A lock-free ring buffer.

   Thread-safe with a single reader and single writer, and realtime safe
   on both ends.
*/
typedef struct ZixRingImpl ZixRing;

/**
   Create a new ring.
   @param size Size in bytes (note this may be rounded up).

   At most `size` - 1 bytes may be stored in the ring at once.
*/
ZIX_MALLOC_API
ZixRing*
zix_ring_new(uint32_t size);

/**
   Destroy a ring.
*/
ZIX_API
void
zix_ring_free(ZixRing* ring);

/**
   Lock the ring data into physical memory.

   This function is NOT thread safe or real-time safe, but it should be called
   after zix_ring_new() to lock all ring memory to avoid page faults while
   using the ring (i.e. this function MUST be called first in order for the
   ring to be truly real-time safe).

*/
ZIX_API
void
zix_ring_mlock(ZixRing* ring);

/**
   Reset (empty) a ring.

   This function is NOT thread-safe, it may only be called when there are no
   readers or writers.
*/
ZIX_API
void
zix_ring_reset(ZixRing* ring);

/**
   Return the number of bytes of space available for reading.
*/
ZIX_CONST_API
uint32_t
zix_ring_read_space(const ZixRing* ring);

/**
   Return the number of bytes of space available for writing.
*/
ZIX_CONST_API
uint32_t
zix_ring_write_space(const ZixRing* ring);

/**
   Return the capacity (i.e. total write space when empty).
*/
ZIX_CONST_API
uint32_t
zix_ring_capacity(const ZixRing* ring);

/**
   Read from the ring without advancing the read head.
*/
ZIX_API
uint32_t
zix_ring_peek(ZixRing* ring, void* dst, uint32_t size);

/**
   Read from the ring and advance the read head.
*/
ZIX_API
uint32_t
zix_ring_read(ZixRing* ring, void* dst, uint32_t size);

/**
   Skip data in the ring (advance read head without reading).
*/
ZIX_API
uint32_t
zix_ring_skip(ZixRing* ring, uint32_t size);

/**
   Write data to the ring.
*/
ZIX_API
uint32_t
zix_ring_write(ZixRing* ring, const void* src, uint32_t size);

/**
   @}
   @}
*/

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ZIX_RING_H */
