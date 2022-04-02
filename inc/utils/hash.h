/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Hash utils.
 */

#ifndef __UTILS_HASH_H__
#define __UTILS_HASH_H__

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
hash_get_from_file (
  const char *  filepath,
  HashAlgorithm algo);

uint32_t
hash_get_from_file_simple (
  const char *  filepath);

void *
hash_create_state (void);

void
hash_free_state (
  void * in_state);

uint32_t
hash_get_for_struct_full (
  XXH32_state_t *    state,
  const void * const obj,
  size_t             size);

uint32_t
hash_get_for_struct (
  const void * const obj,
  size_t             size);

/**
 * @}
 */

#endif
