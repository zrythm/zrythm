// SPDX-FileCopyrightText: © 2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include <stdlib.h>

#include "utils/hash.h"

#include <glib.h>

#include <xxhash.h>

static void
test_get_from_file (void)
{
  char * filepath =
    g_build_filename (TESTS_SRCDIR, "test_start_with_signal.mp3", NULL);

  g_message ("start");
  char *   hash = hash_get_from_file (filepath, HASH_ALGORITHM_XXH32);
  uint32_t hash_simple = hash_get_from_file_simple (filepath);
  g_message ("end");
  g_assert_cmpstr (hash, ==, "ca5b86cb");
  g_assert_cmpuint (hash_simple, ==, 3394995915);
  g_free (hash);

#if XXH_VERSION_NUMBER >= 800
  g_message ("start");
  hash = hash_get_from_file (filepath, HASH_ALGORITHM_XXH3_64);
  g_message ("end");
  g_assert_cmpstr (hash, ==, "e9cd4b9c1e12785e");
  g_free (hash);
#endif

  g_free (filepath);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/hash/"

  g_test_add_func (
    TEST_PREFIX "test get from file", (GTestFunc) test_get_from_file);

  return g_test_run ();
}
