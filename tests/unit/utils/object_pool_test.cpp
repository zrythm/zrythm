// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <thread>
#include <vector>

#include "utils/gtest_wrapper.h"
#include "utils/object_pool.h"

class TestObject
{
public:
  int value = 0;
};

TEST (ObjectPoolTest, BasicOperations)
{
  ObjectPool<TestObject> pool (4);

  auto obj1 = pool.acquire ();
  EXPECT_NE (obj1, nullptr);
  obj1->value = 42;

  auto obj2 = pool.acquire ();
  EXPECT_NE (obj2, nullptr);
  EXPECT_NE (obj1, obj2);

  pool.release (obj1);
  pool.release (obj2);
}

TEST (ObjectPoolTest, AutoExpansion)
{
  ObjectPool<TestObject> pool (2);

  std::vector<TestObject *> objects;
  for (int i = 0; i < 8; i++)
    {
      objects.push_back (pool.acquire ());
      EXPECT_NE (objects.back (), nullptr);
    }

  for (auto obj : objects)
    {
      pool.release (obj);
    }
}

TEST (ObjectPoolTest, MultiThreaded)
{
  ObjectPool<TestObject> pool (32);
  std::atomic<int>       acquired{ 0 };
  std::atomic<int>       released{ 0 };
  constexpr int          num_threads = 4;
  constexpr int          ops_per_thread = 100;

  std::vector<std::thread> threads;
  for (int i = 0; i < num_threads; i++)
    {
      threads.emplace_back ([&] () {
        std::vector<TestObject *> thread_objects;
        for (int j = 0; j < ops_per_thread; j++)
          {
            // Add small sleep to reduce contention
            std::this_thread::sleep_for (std::chrono::microseconds (1));

            auto obj = pool.acquire ();
            if (obj)
              {
                acquired++;
                thread_objects.push_back (obj);
              }

            if (!thread_objects.empty () && (rand () % 2 == 0))
              {
                pool.release (thread_objects.back ());
                thread_objects.pop_back ();
                released++;
              }
          }

        // Release remaining objects
        for (auto obj : thread_objects)
          {
            pool.release (obj);
            released++;
          }
      });
    }

  for (auto &t : threads)
    {
      t.join ();
    }

  EXPECT_EQ (released.load (), acquired.load ());
}

TEST (ObjectPoolTest, DebugCounters)
{
  ObjectPool<TestObject, true> pool (4);

  EXPECT_EQ (pool.get_num_in_use (), 0);
  EXPECT_EQ (pool.get_capacity (), 4);

  auto obj = pool.acquire ();
  EXPECT_EQ (pool.get_num_in_use (), 1);

  pool.release (obj);
  EXPECT_EQ (pool.get_num_in_use (), 0);
}

template <typename T>
static void
test_with_size (
  size_t initial_capacity,
  size_t num_to_acquire,
  size_t expected_capacity)
{
  ObjectPool<T> pool (initial_capacity);
  EXPECT_EQ (pool.get_capacity (), initial_capacity);

  std::vector<T *> acquired;
  for (size_t i = 0; i < num_to_acquire; i++)
    {
      auto obj = pool.acquire ();
      EXPECT_TRUE (obj);
      acquired.push_back (obj);
    }

  EXPECT_EQ (pool.get_capacity (), expected_capacity);

  for (auto obj : acquired)
    {
      pool.release (obj);
    }
}

TEST (ObjectPoolTest, VariousSizes)
{
  struct TestStruct
  {
    int a;
    int b;
  };

  // fixed size
  test_with_size<TestStruct> (16, 16, 16);

  // expand
  test_with_size<TestStruct> (16, 17, 32);
  test_with_size<TestStruct> (16, 31, 32);
  test_with_size<TestStruct> (16, 32, 32);
}
