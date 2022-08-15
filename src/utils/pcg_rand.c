// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 * ---
 *
 * *Really* minimal PCG32 code / (c) 2014 M.E. O'Neill / pcg-random.org
 * Licensed under Apache License 2.0 (NO WARRANTY, etc. see website)
 * Adapted for airwindows (c) 2021 by Robin Gareus <robin@gareus.org>
 *
 * ---
 */

#include "utils/objects.h"
#include "utils/pcg_rand.h"

#include <glib.h>

#include <time.h>

typedef struct PCGRand
{
  uint64_t _state;
  uint64_t _inc;
} PCGRand;

PCGRand *
pcg_rand_new (void)
{
  PCGRand * self = object_new (PCGRand);
  int       foo = 0;
  uint64_t  initseq = (uint64_t) (intptr_t) &foo;
  self->_state = 0;
  self->_inc = (initseq << 1) | 1;
  pcg_rand_u32 (self);
  self->_state += (uint64_t) (time (NULL) ^ (intptr_t) self);
  pcg_rand_u32 (self);

  return self;
}

/* unsigned float [0..1] */
float
pcg_rand_uf (PCGRand * self)
{
  return (float) pcg_rand_u32 (self) / 4294967295.f;
}

/* signed float [-1..+1] */
float
pcg_rand_sf (PCGRand * self)
{
  return ((float) pcg_rand_u32 (self) / 2147483647.5f) - 1.f;
}

uint32_t
pcg_rand_u32 (PCGRand * self)
{
  uint64_t oldstate = self->_state;
  self->_state =
    oldstate * 6364136223846793005ULL + self->_inc;
  uint32_t xorshifted =
    (uint32_t) (((oldstate >> 18u) ^ oldstate) >> 27u);
  uint32_t rot = (uint32_t) (oldstate >> 59u);
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
