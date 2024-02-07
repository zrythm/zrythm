/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
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
