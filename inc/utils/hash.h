// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Hash utils.
 */

#ifndef __UTILS_HASH_H__
#define __UTILS_HASH_H__

#include <stdint.h>

#include <xxhash.h>

/**
 * @addtogroup utils
 *
 * @{
 */

typedef enum HashAlgorithm
{
  HASH_ALGORITHM_XXH32,
  HASH_ALGORITHM_XXH3_64,
} HashAlgorithm;

char *
hash_get_from_file (const char * filepath, HashAlgorithm algo);

uint32_t
hash_get_from_file_simple (const char * filepath);

void *
hash_create_state (void);

void
hash_free_state (void * in_state);

uint32_t
hash_get_for_struct_full (
  XXH32_state_t *    state,
  const void * const obj,
  size_t             size);

uint32_t
hash_get_for_struct (const void * const obj, size_t size);

/**
 * @}
 */

#endif
