/*
 * Copyright (C) 2020 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
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
