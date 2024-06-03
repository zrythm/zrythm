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

#include "utils/pcg_rand.h"

#include <glib.h>

#include <time.h>

PCGRand::PCGRand ()
{
  int      foo = 0;
  uint64_t initseq = (uint64_t) (intptr_t) &foo;
  this->_state = 0;
  this->_inc = (initseq << 1) | 1;
  u32 ();
  this->_state += (uint64_t) (time (NULL) ^ (intptr_t) this);
  u32 ();
}

JUCE_IMPLEMENT_SINGLETON (PCGRand)

/* unsigned float [0..1] */
float
PCGRand::uf ()
{
  return (float) u32 () / 4294967295.f;
}

/* signed float [-1..+1] */
float
PCGRand::sf ()
{
  return ((float) u32 () / 2147483647.5f) - 1.f;
}

uint32_t
PCGRand::u32 ()
{
  uint64_t oldstate = this->_state;
  this->_state = oldstate * 6364136223846793005ULL + this->_inc;
  uint32_t xorshifted = (uint32_t) (((oldstate >> 18u) ^ oldstate) >> 27u);
  uint32_t rot = (uint32_t) (oldstate >> 59u);
  return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}
