/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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

#include "zrythm-test-config.h"

#include <stdlib.h>

#include "utils/arrays.h"
#include "utils/mem.h"
#include "utils/objects.h"

#include <glib.h>

static void
test_array_dynamic_swap (void)
{
  int ** arr1 = calloc (1, sizeof (int *));
  size_t sz1 = 1;
  int ** arr2 = NULL;
  size_t sz2 = 0;

  /* test arr1 allocated and arr2 NULL */
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
    "* assertion*failed");
  array_dynamic_swap (&arr1, &sz1, &arr2, &sz2);
  g_test_assert_expected_messages ();

  /* test arr2 allocated and arr1 NULL */
  arr2 = arr1;
  arr1 = NULL;
  g_test_expect_message (
    G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
    "* assertion*failed");
  array_dynamic_swap (&arr1, &sz1, &arr2, &sz2);
  g_test_assert_expected_messages ();

  /*
   * Setup:
   *
   * a = { 0, 1, 2 }
   * b = { 3, 4 }
   */
  int a = 0, b = 1, c = 2, d = 3, e = 4;
  sz1 = 3;
  arr1 = realloc (arr1, sz1 * sizeof (int *));
  arr1[0] = &a;
  arr1[1] = &b;
  arr1[2] = &c;
  sz2 = 2;
  arr2 = realloc (arr2, sz2 * sizeof (int *));
  arr2[0] = &d;
  arr2[1] = &e;

  /* test swap with the above */
  array_dynamic_swap (&arr1, &sz1, &arr2, &sz2);
  g_assert_cmpuint (sz1, ==, 2);
  g_assert_cmpint (*arr1[0], ==, 3);
  g_assert_cmpint (*arr1[1], ==, 4);
  g_assert_cmpuint (sz2, ==, 3);
  g_assert_cmpint (*arr2[0], ==, 0);
  g_assert_cmpint (*arr2[1], ==, 1);
  g_assert_cmpint (*arr2[2], ==, 2);

  /*
   * Setup:
   *
   * a = { }
   * b = { 3, 4 }
   */
  sz1 = 0;
  arr1 = realloc (arr1, 1 * sizeof (int *));
  sz2 = 2;
  arr2 = realloc (arr2, sz2 * sizeof (int *));
  arr2[0] = &d;
  arr2[1] = &e;

  /* test swap with the above */
  array_dynamic_swap (&arr1, &sz1, &arr2, &sz2);
  g_assert_cmpuint (sz1, ==, 2);
  g_assert_cmpint (*arr1[0], ==, 3);
  g_assert_cmpint (*arr1[1], ==, 4);
  g_assert_cmpuint (sz2, ==, 0);
}

static void
test_double_size_if_full (void)
{
  size_t size = 3;
  int *  arr = g_malloc0_n (size, sizeof (int));

  arr[0] = 15;
  arr[1] = 16;
  arr[2] = 17;
  int num_objs = 3;

  size_t orig_sz = size;
  array_double_size_if_full (
    arr, num_objs, size, int);

  g_assert_cmpuint (size, ==, orig_sz * 2);

  array_append (arr, num_objs, 18);

  g_assert_cmpint (num_objs, ==, 4);
  g_assert_cmpint (arr[0], ==, 15);
  g_assert_cmpint (arr[1], ==, 16);
  g_assert_cmpint (arr[2], ==, 17);
  g_assert_cmpint (arr[3], ==, 18);
  g_assert_cmpint (arr[4], ==, 0);
  g_assert_cmpint (arr[5], ==, 0);
}

int
main (int argc, char * argv[])
{
  g_test_init (&argc, &argv, NULL);

#define TEST_PREFIX "/utils/arrays/"

  g_test_add_func (
    TEST_PREFIX "test array dynamic swap",
    (GTestFunc) test_array_dynamic_swap);
  g_test_add_func (
    TEST_PREFIX "test double size if full",
    (GTestFunc) test_double_size_if_full);

  return g_test_run ();
}
