// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/playback_cache_scheduler.h"

#include <QSignalSpy>
#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;
using namespace std::chrono_literals;

namespace zrythm::utils
{
class PlaybackCacheSchedulerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Set up QCoreApplication for Qt event handling
    app_ = std::make_unique<ScopedQCoreApplication> ();
    scheduler = new PlaybackCacheScheduler ();
    // Use a smaller delay for faster tests, but not too small to be unreliable
    scheduler->setDelay (40ms);
  }

  void TearDown () override { delete scheduler; }

  std::unique_ptr<ScopedQCoreApplication> app_;
  PlaybackCacheScheduler *                scheduler{ nullptr };
};

TEST_F (PlaybackCacheSchedulerTest, QueueRangeRequest)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  scheduler->queueCacheRequestForRange (10.0, 20.0);

  // Process events to trigger the timer
  QTest::qWait (70ms); // Wait slightly longer than 40ms delay

  EXPECT_EQ (spy.count (), 1);
  auto signalArgs = spy.takeFirst ();
  auto range = signalArgs.at (0).value<ExpandableTickRange> ();
  EXPECT_FALSE (range.is_full_content ());
  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 10.0);
  EXPECT_EQ (result_range->second, 20.0);
}

TEST_F (PlaybackCacheSchedulerTest, QueueFullCacheRequest)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  scheduler->queueFullCacheRequest ();

  QTest::qWait (70ms);

  EXPECT_EQ (spy.count (), 1);
  auto signalArgs = spy.takeFirst ();
  auto range = signalArgs.at (0).value<ExpandableTickRange> ();
  EXPECT_TRUE (range.is_full_content ());
  EXPECT_FALSE (range.range ().has_value ());
}

TEST_F (PlaybackCacheSchedulerTest, DebouncingMultipleRequests)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  // Queue multiple rapid requests
  scheduler->queueCacheRequestForRange (10.0, 20.0);
  scheduler->queueCacheRequestForRange (15.0, 25.0);
  scheduler->queueCacheRequestForRange (5.0, 30.0);

  QTest::qWait (70ms);

  // Should only emit once due to debouncing
  EXPECT_EQ (spy.count (), 1);

  auto signalArgs = spy.takeFirst ();
  auto range = signalArgs.at (0).value<ExpandableTickRange> ();
  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 5.0);   // Expanded to min start
  EXPECT_EQ (result_range->second, 30.0); // Expanded to max end
}

TEST_F (PlaybackCacheSchedulerTest, CustomDelay)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  scheduler->setDelay (100ms);
  scheduler->queueCacheRequestForRange (10.0, 20.0);

  QTest::qWait (50);
  EXPECT_EQ (spy.count (), 0); // Should not have fired yet

  QTest::qWait (100); // Total wait 150ms, should be enough
  EXPECT_EQ (spy.count (), 1);
}

TEST_F (PlaybackCacheSchedulerTest, MixedRangeAndFullRequests)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  // Queue range request followed by full request
  scheduler->queueCacheRequestForRange (10.0, 20.0);
  scheduler->queueFullCacheRequest ();

  QTest::qWait (70ms);

  EXPECT_EQ (spy.count (), 1);
  auto signalArgs = spy.takeFirst ();
  auto range = signalArgs.at (0).value<ExpandableTickRange> ();
  EXPECT_TRUE (range.is_full_content ()); // Full request should take precedence
}

TEST_F (PlaybackCacheSchedulerTest, MultipleSeparateRequests)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  // First request
  scheduler->queueCacheRequestForRange (10.0, 20.0);
  QTest::qWait (70ms);
  EXPECT_EQ (spy.count (), 1);

  // Second request after first completed
  scheduler->queueCacheRequestForRange (30.0, 40.0);
  QTest::qWait (70ms);
  EXPECT_EQ (spy.count (), 2);
}

TEST_F (PlaybackCacheSchedulerTest, ExpandableTickRangeIntegration)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  // Test using ExpandableTickRange directly
  ExpandableTickRange range1{ std::make_optional (std::make_pair (10.0, 20.0)) };
  ExpandableTickRange range2{ std::make_optional (std::make_pair (15.0, 25.0)) };

  scheduler->queueCacheRequest (range1);
  scheduler->queueCacheRequest (range2);

  QTest::qWait (70ms);

  EXPECT_EQ (spy.count (), 1);
  auto signalArgs = spy.takeFirst ();
  auto result_range = signalArgs.at (0).value<ExpandableTickRange> ();
  auto range_value = result_range.range ();
  EXPECT_TRUE (range_value.has_value ());
  EXPECT_EQ (range_value->first, 10.0);
  EXPECT_EQ (range_value->second, 25.0);
}

TEST_F (PlaybackCacheSchedulerTest, TimerResetOnNewRequest)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  scheduler->setDelay (200ms);

  // First request
  scheduler->queueCacheRequestForRange (10.0, 20.0);

  // Second request before timer expires - should reset timer
  QTest::qWait (150);
  scheduler->queueCacheRequestForRange (15.0, 25.0);

  // Check that timer was reset - should not fire at original time
  QTest::qWait (100);          // Total 250ms from start
  EXPECT_EQ (spy.count (), 0); // Only 100ms since reset, so not fired yet

  // Enough time has passed - should fire
  QTest::qWait (150);          // Total 250ms from reset
  EXPECT_EQ (spy.count (), 1); // Should have fired once

  auto signalArgs = spy.takeFirst ();
  auto range = signalArgs.at (0).value<ExpandableTickRange> ();
  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 10.0);
  EXPECT_EQ (result_range->second, 25.0);
}

TEST_F (PlaybackCacheSchedulerTest, NoPendingCallAfterExecution)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  scheduler->queueCacheRequestForRange (10.0, 20.0);
  QTest::qWait (70ms);

  EXPECT_EQ (spy.count (), 1);

  // Internal state should be reset
  // This is testing implementation details, but useful for coverage
  scheduler->queueCacheRequestForRange (30.0, 40.0);
  QTest::qWait (70ms);

  EXPECT_EQ (spy.count (), 2);
}

TEST_F (PlaybackCacheSchedulerTest, EdgeCaseZeroRange)
{
  QSignalSpy spy (scheduler, &PlaybackCacheScheduler::cacheRequested);

  scheduler->queueCacheRequestForRange (0.0, 0.0);
  QTest::qWait (70ms);

  EXPECT_EQ (spy.count (), 1);
  auto signalArgs = spy.takeFirst ();
  auto range = signalArgs.at (0).value<ExpandableTickRange> ();
  auto result_range = range.range ();
  EXPECT_TRUE (result_range.has_value ());
  EXPECT_EQ (result_range->first, 0.0);
  EXPECT_EQ (result_range->second, 0.0);
}
}
