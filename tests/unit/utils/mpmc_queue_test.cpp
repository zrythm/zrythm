// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <thread>
#include <vector>

#include "utils/gtest_wrapper.h"
#include "utils/mpmc_queue.h"

TEST (MPMCQueueTest, BasicOperations)
{
  MPMCQueue<int> queue (8);

  EXPECT_TRUE (queue.push_back (42));
  int value;
  EXPECT_TRUE (queue.pop_front (value));
  EXPECT_EQ (value, 42);
}

TEST (MPMCQueueTest, QueueCapacity)
{
  MPMCQueue<int> queue (8);
  EXPECT_EQ (queue.capacity (), 8);

  // Fill queue
  for (int i = 0; i < 8; i++)
    {
      EXPECT_TRUE (queue.push_back (i));
    }

  // Queue should be full
  EXPECT_FALSE (queue.push_back (42));
}

TEST (MPMCQueueTest, ClearOperation)
{
  MPMCQueue<int> queue (8);
  for (int i = 0; i < 4; i++)
    {
      queue.push_back (i);
    }

  queue.clear ();

  int value;
  EXPECT_FALSE (queue.pop_front (value));
}

TEST (MPMCQueueTest, MultiThreaded)
{
  MPMCQueue<int>   queue (1024);
  std::atomic<int> sum (0);
  constexpr int    num_producers = 4;
  constexpr int    num_consumers = 4;
  constexpr int    items_per_producer = 1000;

  // Producer threads
  std::vector<std::thread> producers;
  for (int i = 0; i < num_producers; i++)
    {
      producers.emplace_back ([&queue, i] () {
        for (int j = 0; j < items_per_producer; j++)
          {
            while (!queue.push_back (i * items_per_producer + j))
              {
                std::this_thread::yield ();
              }
          }
      });
    }

  // Consumer threads
  std::vector<std::thread> consumers;
  for (int i = 0; i < num_consumers; i++)
    {
      consumers.emplace_back ([&queue, &sum] () {
        int value;
        while (sum < num_producers * items_per_producer)
          {
            if (queue.pop_front (value))
              {
                sum++;
              }
            else
              {
                std::this_thread::yield ();
              }
          }
      });
    }

  // Join all threads
  for (auto &p : producers)
    p.join ();
  for (auto &c : consumers)
    c.join ();

  EXPECT_EQ (sum, num_producers * items_per_producer);
}
