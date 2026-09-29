// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>
#include <string>
#include <thread>
#include <vector>

#include "utils/logger.h"
#include "utils/rt_logger.h"

#include <fmt/format.h>
#include <gtest/gtest.h>

namespace zrythm::utils
{

namespace
{

bool
entries_contain (std::string_view needle)
{
  const auto entries = get_last_log_entries (1000);
  return std::ranges::any_of (entries, [needle] (const Utf8String &entry) {
    return entry.str ().find (needle) != std::string::npos;
  });
}

std::size_t
entries_containing_count (std::string_view needle)
{
  const auto entries = get_last_log_entries (1000);
  return static_cast<std::size_t> (
    std::ranges::count_if (entries, [needle] (const Utf8String &entry) {
      return entry.str ().find (needle) != std::string::npos;
    }));
}

} // namespace

TEST (RtLoggerTest, UninitializedRealtimeLoggingDoesNotCrash)
{
  // Realtime log calls before init_logging() are no-ops; running after
  // another test initialized logging still exercises the queue path
  z_rt_trace ("rt trace {}", 42);
  z_rt_debug ("rt debug");
  z_rt_info ("rt info");
  z_rt_warning ("rt warning");
}

TEST (RtLoggerTest, DeliversMessagesWithProducerThreadMarker)
{
  init_logging (LoggerType::Test);

  z_rt_warning ("rt-deliver marker {}", 123);

  ASSERT_TRUE (drain_rt_log (std::chrono::milliseconds{ 2000 }));
  EXPECT_TRUE (entries_contain ("rt-deliver marker 123"));
  EXPECT_TRUE (entries_contain ("[rt thread "));
}

TEST (RtLoggerTest, TruncatesMessagesToSlotCapacity)
{
  init_logging (LoggerType::Test);

  const auto prefix = std::string (RtLogMsg::text_capacity - 1, 'x');
  z_rt_warning ("{} tail-beyond-capacity", prefix);

  ASSERT_TRUE (drain_rt_log (std::chrono::milliseconds{ 2000 }));
  EXPECT_TRUE (entries_contain (prefix));
  EXPECT_FALSE (entries_contain ("tail-beyond-capacity"));
}

TEST (RtLoggerTest, DeliversMessagesFromMultipleProducerThreads)
{
  init_logging (LoggerType::Test);

  const auto    dropped_before = rt_log_dropped_count ();
  constexpr int num_threads = 4;
  constexpr int msgs_per_thread = 50;

  std::vector<std::jthread> producers;
  for (const auto t : std::views::iota (0, num_threads))
    {
      producers.emplace_back ([t, msgs_per_thread] {
        for (const auto i : std::views::iota (0, msgs_per_thread))
          {
            z_rt_info ("rt-mpsc t{:02}-i{:02}", t, i);
          }
      });
    }
  for (auto &thread : producers)
    thread.join ();

  ASSERT_TRUE (drain_rt_log (std::chrono::milliseconds{ 2000 }));

  std::size_t delivered = 0;
  for (const auto t : std::views::iota (0, num_threads))
    {
      for (const auto i : std::views::iota (0, msgs_per_thread))
        {
          delivered += entries_containing_count (
            fmt::format ("rt-mpsc t{:02}-i{:02}", t, i));
        }
    }
  // Each message is unique, so exactly one entry matches each marker
  EXPECT_EQ (delivered, num_threads * msgs_per_thread);
  EXPECT_EQ (rt_log_dropped_count () - dropped_before, 0);
}

TEST (RtLoggerTest, FullQueueRejectsPushesAndCountsDrops)
{
  init_logging (LoggerType::Test);

  const auto dropped_before = rt_log_dropped_count ();
  set_rt_log_consumer_paused (true);

  constexpr int total_messages = 700;
  for (const auto i : std::views::iota (0, total_messages))
    {
      z_rt_warning ("rt-overflow #{}", i);
    }

  set_rt_log_consumer_paused (false);
  ASSERT_TRUE (drain_rt_log (std::chrono::milliseconds{ 2000 }));

  // While the consumer is suspended, pushes beyond the queue capacity are
  // rejected and counted; the queued messages are delivered in order
  const auto rejected_count =
    total_messages - static_cast<int> (rt_log_queue_capacity);
  EXPECT_TRUE (entries_contain (
    fmt::format ("rt-overflow #{}", rt_log_queue_capacity - 1)));
  EXPECT_FALSE (
    entries_contain (fmt::format ("rt-overflow #{}", rt_log_queue_capacity)));
  EXPECT_EQ (
    rt_log_dropped_count () - dropped_before,
    static_cast<std::uint64_t> (rejected_count));
  EXPECT_TRUE (entries_contain (
    fmt::format ("RT log queue full: {} message(s) dropped", rejected_count)));
}

} // namespace zrythm::utils
