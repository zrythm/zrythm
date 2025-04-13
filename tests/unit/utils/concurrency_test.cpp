// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/concurrency.h"
#include "utils/gtest_wrapper.h"

TEST (ConcurrencyTest, AtomicBoolRAII)
{
  std::atomic<bool> flag{ false };

  {
    AtomicBoolRAII raii (flag);
    EXPECT_TRUE (flag.load ());
  }

  EXPECT_FALSE (flag.load ());
}

TEST (ConcurrencyTest, SemaphoreRAIIBasic)
{
  std::binary_semaphore sem (1);

  {
    SemaphoreRAII raii (sem);
    EXPECT_TRUE (raii.is_acquired ());

    // Second attempt should fail
    SemaphoreRAII raii2 (sem);
    EXPECT_FALSE (raii2.is_acquired ());
  }

  // After release, should be able to acquire again
  SemaphoreRAII raii3 (sem);
  EXPECT_TRUE (raii3.is_acquired ());
}

TEST (ConcurrencyTest, SemaphoreRAIIForceAcquire)
{
  std::binary_semaphore sem (1);
  std::atomic<bool>     acquired{ false };
  std::atomic<bool>     thread_started{ false };

  std::thread t ([&] () {
    thread_started = true;
    SemaphoreRAII raii (sem, true); // Force acquire
    acquired = raii.is_acquired ();
  });

  // Hold the semaphore briefly
  {
    SemaphoreRAII raii (sem);
    while (!thread_started)
      {
        std::this_thread::yield ();
      }
    std::this_thread::sleep_for (std::chrono::milliseconds (10));
  }

  t.join ();
  EXPECT_TRUE (acquired);
}

TEST (ConcurrencyTest, SemaphoreRAIIManualRelease)
{
  std::binary_semaphore sem (1);

  SemaphoreRAII raii (sem);
  EXPECT_TRUE (raii.is_acquired ());

  raii.release ();
  EXPECT_FALSE (raii.is_acquired ());

  // Should be able to acquire again
  EXPECT_TRUE (raii.try_acquire ());
}

TEST (ConcurrencyTest, SemaphoreRAIIMultiThreaded)
{
  std::binary_semaphore sem (1);
  std::atomic<int>      counter{ 0 };
  constexpr int         num_iterations = 1000;

  auto worker = [&] () {
    for (int i = 0; i < num_iterations; i++)
      {
        SemaphoreRAII raii (sem, true);
        counter++;
      }
  };

  std::thread t1 (worker);
  std::thread t2 (worker);

  t1.join ();
  t2.join ();

  EXPECT_EQ (counter, num_iterations * 2);
}
