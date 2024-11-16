// SPDX-FileCopyrightText: © 2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Memory utils.
 */

#ifndef __UTILS_MEM_H__
#define __UTILS_MEM_H__

#include <cstddef>

void *
z_realloc (void * ptr, size_t size);

void
z_free_strv (char ** strv);

/**
 * Reallocate and zero out newly added memory.
 */
void *
realloc_zero (void * pBuffer, size_t oldSize, size_t newSize);

#endif
