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

static char *
get_xxh32_hash (
  FILE * stream)
{
  /* create a state */
  XXH32_state_t * state = XXH32_createState();
  g_return_val_if_fail (state, NULL);

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
  char * ret =
    g_strdup_printf (
      "%x%x%x%x",
      canonical.digest[0],
      canonical.digest[1],
      canonical.digest[2],
      canonical.digest[3]);

  return ret;
}

#if XXH_VERSION_NUMBER >= 800
static char *
get_xxh3_64_hash (
  FILE * stream)
{
  /* create a state */
  XXH3_state_t * state = XXH3_createState ();
  g_return_val_if_fail (state, NULL);

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
  char * ret =
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

  return ret;
}
#endif

char *
hash_get_from_file (
  const char *  filepath,
  HashAlgorithm algo)
{
  g_debug ("calculating hash for %s...", filepath);

  FILE * stream = fopen (filepath, "rb");

  char * ret = NULL;
  switch (algo)
    {
    case HASH_ALGORITHM_XXH3_64:
#if XXH_VERSION_NUMBER >= 800
      ret = get_xxh3_64_hash (stream);
      break;
#endif
    case HASH_ALGORITHM_XXH32:
      ret = get_xxh32_hash (stream);
      break;
    }

  fclose (stream);

  g_debug ("hash for %s: %s", filepath, ret);

  return ret;
}
