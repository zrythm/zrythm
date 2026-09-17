// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <thread>
#include <vector>

#include "plugins/lv2_worker_queue.h"

#include <gtest/gtest.h>

namespace zrythm::plugins
{

namespace
{

std::vector<std::byte>
make_payload (uint32_t seed, uint32_t size)
{
  std::vector<std::byte> bytes (size);
  for (size_t i = 0; i < bytes.size (); ++i)
    {
      bytes[i] = static_cast<std::byte> ((seed * 31 + i) % 251);
    }
  return bytes;
}

bool
popped_equals (Lv2WorkerQueue &queue, const std::vector<std::byte> &expected)
{
  std::array<std::byte, kMaxWorkerRecordSize> scratch{};
  uint32_t                                    size = 0;
  if (!queue.pop (scratch, size))
    return false;
  return size == expected.size ()
         && std::memcmp (scratch.data (), expected.data (), size) == 0;
}

} // namespace

// A record with a null buffer and a positive size is refused whole:
// nothing enters the queue and the failure is counted
TEST (Lv2WorkerQueueTest, NullPayloadWithSizeIsRejected)
{
  Lv2WorkerQueue queue (128);
  EXPECT_FALSE (queue.push ({ static_cast<const std::byte *> (nullptr), 16 }));
  EXPECT_EQ (queue.dropped_records (), 1u);
  EXPECT_TRUE (queue.empty ());
}

// Records of varying sizes, including empty ones, come back in order
// and byte-identical
TEST (Lv2WorkerQueueTest, RecordsRoundTripInOrder)
{
  Lv2WorkerQueue queue (128);
  const auto     first = make_payload (1, 1);
  const auto     second = make_payload (2, 0);
  const auto     third = make_payload (3, 40);
  ASSERT_TRUE (queue.push (first));
  ASSERT_TRUE (queue.push (second));
  ASSERT_TRUE (queue.push (third));

  EXPECT_TRUE (popped_equals (queue, first));
  EXPECT_TRUE (popped_equals (queue, second));
  EXPECT_TRUE (popped_equals (queue, third));
  std::array<std::byte, kMaxWorkerRecordSize> scratch{};
  uint32_t                                    size = 0;
  EXPECT_FALSE (queue.pop (scratch, size));
}

// A push that does not fit is refused and counted; the records
// already queued stay intact and in order
TEST (Lv2WorkerQueueTest, OverflowIsRefusedAndCounted)
{
  Lv2WorkerQueue queue (64);
  const auto     kept = make_payload (1, 8);
  ASSERT_TRUE (queue.push (kept));

  EXPECT_FALSE (queue.push (make_payload (2, 60)));
  EXPECT_EQ (queue.dropped_records (), 1u);
  EXPECT_TRUE (popped_equals (queue, kept));

  EXPECT_FALSE (queue.push (make_payload (3, kMaxWorkerRecordSize + 1)));
  EXPECT_EQ (queue.dropped_records (), 2u);
}

// Records continue to round-trip while the ring wraps repeatedly
TEST (Lv2WorkerQueueTest, RecordsSurviveWrapAround)
{
  Lv2WorkerQueue queue (64);
  for (uint32_t i = 0; i < 128; ++i)
    {
      const auto payload = make_payload (i, (i % 3 + 1) * 5);
      ASSERT_TRUE (queue.push (payload)) << "iteration " << i;
      EXPECT_TRUE (popped_equals (queue, payload)) << "iteration " << i;
    }
  EXPECT_EQ (queue.dropped_records (), 0u);
}

// A producer and a consumer on separate threads exchange a stream of
// records without corruption
TEST (Lv2WorkerQueueTest, ConcurrentProducerConsumerKeepIntegrity)
{
  Lv2WorkerQueue queue (1024);
  constexpr auto kRecordCount = 2000u;

  std::atomic<bool> producer_done{ false };
  std::jthread      consumer ([&] {
    uint32_t                                    received = 0;
    std::array<std::byte, kMaxWorkerRecordSize> scratch{};
    uint32_t                                    size = 0;
    while (received < kRecordCount)
      {
        if (queue.pop (scratch, size))
          {
            const auto expected = make_payload (received, size);
            ASSERT_EQ (std::memcmp (scratch.data (), expected.data (), size), 0)
              << "record " << received;
            ++received;
          }
        else if (producer_done.load ())
          {
            break;
          }
      }
    EXPECT_EQ (received, kRecordCount);
  });

  for (uint32_t i = 0; i < kRecordCount; ++i)
    {
      const auto size = (i % 7) * 13;
      while (!queue.push (make_payload (i, size)))
        std::this_thread::yield ();
    }
  producer_done.store (true);
}

// reset() empties the queue and clears the drop counter
TEST (Lv2WorkerQueueTest, ResetClearsContentsAndCounters)
{
  Lv2WorkerQueue queue (64);
  ASSERT_TRUE (queue.push (make_payload (1, 8)));
  EXPECT_FALSE (queue.push (make_payload (2, 60)));

  queue.reset ();

  std::array<std::byte, kMaxWorkerRecordSize> scratch{};
  uint32_t                                    size = 0;
  EXPECT_FALSE (queue.pop (scratch, size));
  EXPECT_EQ (queue.dropped_records (), 0u);
}

} // namespace zrythm::plugins
