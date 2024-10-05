// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "zrythm-test-config.h"

#include "common/utils/gtest_wrapper.h"
#define DEBUG_OBJECT_POOL 1
#include "common/utils/object_pool.h"

static void
test_with_size (size_t initial_cap, int num_elems, size_t expected_end_cap)
{
  struct TestStruct
  {
    int a;
    int b;
  };
  ObjectPool<TestStruct> pool{ initial_cap };
  ASSERT_EQ (pool.get_capacity (), initial_cap);
  ASSERT_EQ (pool.get_size (), initial_cap);
  std::vector<TestStruct *> objs;
  for (int i = 0; i < num_elems; ++i)
    {
      auto obj = pool.acquire ();
      obj->a = i;
      obj->b = i * 2;
      objs.push_back (obj);
      ASSERT_EQ (pool.get_num_in_use (), i + 1);
    }
  ASSERT_EQ (pool.get_num_in_use (), num_elems);
  ASSERT_EQ (pool.get_capacity (), expected_end_cap);
  ASSERT_EQ (pool.get_size (), expected_end_cap);
  size_t count = objs.size ();
  for (auto obj : objs)
    {
      pool.release (obj);
      --count;
      ASSERT_EQ (pool.get_num_in_use (), count);
    }
  ASSERT_EQ (pool.get_capacity (), expected_end_cap);
  ASSERT_EQ (pool.get_size (), expected_end_cap);
}

TEST (ObjectPool, TestVariousSizes)
{
  // fixed size
  test_with_size (16, 16, 16);

  // expand
  test_with_size (16, 17, 32);
  test_with_size (16, 31, 32);
  test_with_size (16, 32, 32);
}