// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Memory utils.
 */

#ifndef __UTILS_MEM_H__
#define __UTILS_MEM_H__

#include <stddef.h>

/**
 * Reallocate and zero out newly added memory.
 */
void *
realloc_zero (void * pBuffer, size_t oldSize, size_t newSize);

#endif
