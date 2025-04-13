// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/gtest_wrapper.h"
#include "utils/monotonic_time_provider.h"

using namespace zrythm::utils;

TEST (MonotonicTimeProviderTest, TimeIncrements)
{
  QElapsedTimeProvider provider;

  // First measurement
  auto first_nsecs = provider.get_monotonic_time_nsecs ();
  auto first_usecs = provider.get_monotonic_time_usecs ();

  // Sleep for a small duration
  QThread::msleep (10);

  // Second measurement
  auto second_nsecs = provider.get_monotonic_time_nsecs ();
  auto second_usecs = provider.get_monotonic_time_usecs ();

  // Time should increase
  EXPECT_GT (second_nsecs, first_nsecs);
  EXPECT_GT (second_usecs, first_usecs);
}

TEST (MonotonicTimeProviderTest, TimeConversion)
{
  QElapsedTimeProvider provider;

  auto nsecs = provider.get_monotonic_time_nsecs ();
  auto usecs = provider.get_monotonic_time_usecs ();

  // Verify that nanoseconds to microseconds conversion is correct
  // Allow for small timing differences due to the two separate calls
  const qint64 calculated_usecs = nsecs / 1000;
  const qint64 tolerance = 100; // 100 microseconds tolerance

  EXPECT_NEAR (usecs, calculated_usecs, tolerance);
}

TEST (MonotonicTimeProviderTest, Monotonicity)
{
  QElapsedTimeProvider provider;

  std::vector<qint64> measurements;
  const int           num_measurements = 10;

  // Take several quick measurements
  for (int i = 0; i < num_measurements; i++)
    {
      measurements.push_back (provider.get_monotonic_time_nsecs ());
      QThread::usleep (100);
    }

  // Verify timestamps are strictly increasing
  for (int i = 1; i < num_measurements; i++)
    {
      EXPECT_GT (measurements[i], measurements[i - 1]);
    }
}
