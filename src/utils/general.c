/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#include "utils/general.h"

/**
 * From https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightLinear.
 */
unsigned int
utils_get_uint_from_bitfield_val (unsigned int bitfield)
{
  /* 32-bit word input to count zero bits on
   * right */
  unsigned int v = bitfield;
  unsigned int c = 32; // c will be the number of zero bits on the right
  v &= (unsigned int) (-(signed) (v));
  if (v)
    c--;
  if (v & 0x0000FFFF)
    c -= 16;
  if (v & 0x00FF00FF)
    c -= 8;
  if (v & 0x0F0F0F0F)
    c -= 4;
  if (v & 0x33333333)
    c -= 2;
  if (v & 0x55555555)
    c -= 1;

  return c;
}
