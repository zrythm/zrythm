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

#include <stdio.h>

#include "utils/hash.h"

#include <glib.h>

#include <xxhash.h>

#define BUF_SIZE 16384
#define SEED_32 0xbaad5eed
#define SEED_64 0xbaad5eedbaad5eed

static uint32_t
get_xxh32_hash (
  FILE *  stream,
  char ** hash_str)
{
  /* create a state */
  XXH32_state_t * state = XXH32_createState();
  g_return_val_if_fail (state, 0);

  /* reset state with a seed (0 fastest) */
  XXH32_reset (state, 0);
  size_t amt;
  unsigned char buf[BUF_SIZE];
  while ((amt = fread(buf, 1, sizeof(buf), stream)) != 0)
    {
      /* hash the file in chunks */
      XXH32_update (state, buf, amt);
    }

  /* finalize the hash */
  XXH32_hash_t hash = XXH32_digest (state);

  /* free the state */
  XXH32_freeState (state);

  /* get as canonical string */
  XXH32_canonical_t canonical;
  XXH32_canonicalFromHash (&canonical, hash);
  if (hash_str)
    {
      *hash_str =
        g_strdup_printf (
          "%x%x%x%x",
          canonical.digest[0],
          canonical.digest[1],
          canonical.digest[2],
          canonical.digest[3]);
    }

  return hash;
}

#if XXH_VERSION_NUMBER >= 800
static uint64_t
get_xxh3_64_hash (
  FILE *  stream,
  char ** hash_str)
{
  /* create a state */
  XXH3_state_t * state = XXH3_createState ();
  g_return_val_if_fail (state, 0);

  /* reset state */
  XXH3_64bits_reset (state);
  size_t amt;
  unsigned char buf[BUF_SIZE];
  while ((amt = fread(buf, 1, sizeof(buf), stream)) != 0)
    {
      /* hash the file in chunks */
      XXH3_64bits_update (state, buf, amt);
    }

  /* finalize the hash */
  XXH64_hash_t hash = XXH3_64bits_digest (state);

  /* free the state */
  XXH3_freeState (state);

  /* get as canonical string */
  XXH64_canonical_t canonical;
  XXH64_canonicalFromHash (&canonical, hash);
  if (hash_str)
    {
      *hash_str =
        g_strdup_printf (
          "%x%x%x%x%x%x%x%x",
          canonical.digest[0],
          canonical.digest[1],
          canonical.digest[2],
          canonical.digest[3],
          canonical.digest[4],
          canonical.digest[5],
          canonical.digest[6],
          canonical.digest[7]
          );
    }

  return hash;
}
#endif

char *
hash_get_from_file (
  const char *  filepath,
  HashAlgorithm algo)
{
  g_debug ("calculating hash for %s...", filepath);

  FILE * stream = fopen (filepath, "rb");

  char * ret_str = NULL;
  switch (algo)
    {
    case HASH_ALGORITHM_XXH3_64:
#if XXH_VERSION_NUMBER >= 800
      get_xxh3_64_hash (stream, &ret_str);
      break;
#endif
    case HASH_ALGORITHM_XXH32:
      get_xxh32_hash (stream, &ret_str);
      break;
    }

  fclose (stream);

  g_debug ("hash for %s: %s", filepath, ret_str);

  return ret_str;
}

uint32_t
hash_get_from_file_simple (
  const char *  filepath)
{
  FILE * stream = fopen (filepath, "rb");
  uint32_t hash = get_xxh32_hash (stream, NULL);
  fclose (stream);

  return hash;
}

uint32_t
hash_get_for_struct_full (
  XXH32_state_t *    state,
  const void * const obj,
  size_t             size)
{
  /* reset state with a seed (0 fastest) */
  XXH32_reset (state, SEED_32);

  /* hash the file in chunks */
  XXH32_update (state, obj, size);

  /* finalize the hash */
  return XXH32_digest (state);
}

void *
hash_create_state (void)
{
  XXH32_state_t * state = XXH32_createState();
  return state;
}

void
hash_free_state (
  void * in_state)
{
  XXH32_state_t * state = (XXH32_state_t *) in_state;
  XXH32_freeState (state);
}

uint32_t
hash_get_for_struct (
  const void * const obj,
  size_t             size)
{
  /* create a state */
  XXH32_state_t * state = XXH32_createState();
  g_return_val_if_fail (state, 0);

  XXH32_hash_t hash =
    hash_get_for_struct_full (state, obj, size);

  /* free the state */
  XXH32_freeState (state);

  return hash;
}
