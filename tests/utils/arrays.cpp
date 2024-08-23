// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN

#include "utils/arrays.h"
#include "utils/objects.h"

#include "tests/helpers/zrythm_helper.h"

TEST_SUITE_BEGIN ("utils/arrays");

TEST_CASE ("double size if full")
{
  size_t size = 3;
  int *  arr = object_new_n (size, int);

  arr[0] = 15;
  arr[1] = 16;
  arr[2] = 17;
  int num_objs = 3;

  size_t orig_sz = size;
  array_double_size_if_full (arr, num_objs, size, int);

  REQUIRE_EQ (size, orig_sz * 2);

  array_append (arr, num_objs, 18);

  REQUIRE_EQ (num_objs, 4);
  REQUIRE_EQ (arr[0], 15);
  REQUIRE_EQ (arr[1], 16);
  REQUIRE_EQ (arr[2], 17);
  REQUIRE_EQ (arr[3], 18);
  REQUIRE_EQ (arr[4], 0);
  REQUIRE_EQ (arr[5], 0);
}

TEST_SUITE_END;