/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * \file
 *
 * General utils.
 */

#ifndef __UTILS_GENERAL_H__
#define __UTILS_GENERAL_H__

/**
 * @addtogroup utils
 *
 * @{
 */

#define RETURN_OK return 0;
#define RETURN_ERROR return 1;

/**
 * From https://graphics.stanford.edu/~seander/bithacks.html#ZerosOnRightLinear.
 */
unsigned int
utils_get_uint_from_bitfield_val (unsigned int bitfield);

/**
 * @}
 */

#endif
