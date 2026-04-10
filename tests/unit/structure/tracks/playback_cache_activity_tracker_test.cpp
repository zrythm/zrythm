// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/playback_cache_activity_tracker.h"
#include "utils/playback_cache_scheduler.h"

#include <QSignalSpy>
#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;
using namespace std::chrono_literals;

namespace zrythm::structure::tracks
{

class PlaybackCacheActivityTrackerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<ScopedQCoreApplication> ();
    scheduler_ = new utils::PlaybackCacheScheduler ();
    scheduler_->setDelay (kDelay);
  }

  void TearDown () override { delete scheduler_; }

// On Mac there are some issues with timing...
#ifdef Q_OS_MACOS
  static constexpr auto kDelay = 500ms;
#else
  static constexpr auto kDelay = 50ms;
#endif
  static constexpr auto kWaitTime = (kDelay * 3) / 2;

  utils::PlaybackCacheScheduler * scheduler_{};

private:
  std::unique_ptr<ScopedQCoreApplication> app_;
};

// ========================================================================
// Pending state
// ========================================================================

TEST_F (PlaybackCacheActivityTrackerTest, InitiallyNotPending)
{
  PlaybackCacheActivityTracker tracker (scheduler_);
  QSignalSpy                   pending_spy (
    &tracker, &PlaybackCacheActivityTracker::pendingChanged);

  EXPECT_FALSE (tracker.isPending ());
  EXPECT_EQ (pending_spy.count (), 0);
}

TEST_F (PlaybackCacheActivityTrackerTest, SyncsInitialPendingStateFromScheduler)
{
  // Queue a request BEFORE creating the tracker
  scheduler_->queueCacheRequestForRange (10.0, 20.0);
  EXPECT_TRUE (scheduler_->isPending ());

  // Tracker should immediately see the scheduler's pending state
  PlaybackCacheActivityTracker tracker (scheduler_);
  EXPECT_TRUE (tracker.isPending ());
}

TEST_F (PlaybackCacheActivityTrackerTest, PendingFollowsScheduler)
{
  PlaybackCacheActivityTracker tracker (scheduler_);
  QSignalSpy                   pending_spy (
    &tracker, &PlaybackCacheActivityTracker::pendingChanged);

  scheduler_->queueCacheRequestForRange (10.0, 20.0);
  EXPECT_TRUE (tracker.isPending ());
  EXPECT_GE (pending_spy.count (), 1);

  QTest::qWait (kWaitTime);
  EXPECT_FALSE (tracker.isPending ());
  EXPECT_GE (pending_spy.count (), 2);
}

// ========================================================================
// Entry creation
// ========================================================================

TEST_F (PlaybackCacheActivityTrackerTest, EntryCreatedOnRegenerationComplete)
{
  PlaybackCacheActivityTracker tracker (scheduler_);
  QSignalSpy                   entries_spy (
    &tracker, &PlaybackCacheActivityTracker::entriesChanged);

  tracker.onRegenerationComplete (
    utils::ExpandableTickRange{ std::make_pair (10.0, 20.0) });

  auto entries = tracker.entries ();
  ASSERT_EQ (entries.size (), 1);
  EXPECT_EQ (entries_spy.count (), 1);

  auto entry = entries.first ().value<PlaybackCacheActivityEntry> ();
  EXPECT_DOUBLE_EQ (entry.startTick, 10.0);
  EXPECT_DOUBLE_EQ (entry.endTick, 20.0);
  EXPECT_FALSE (entry.isFullContent);
}

TEST_F (PlaybackCacheActivityTrackerTest, FullContentEntry)
{
  PlaybackCacheActivityTracker tracker (scheduler_);

  tracker.onRegenerationComplete (utils::ExpandableTickRange{});

  auto entries = tracker.entries ();
  ASSERT_EQ (entries.size (), 1);

  auto entry = entries.first ().value<PlaybackCacheActivityEntry> ();
  EXPECT_TRUE (entry.isFullContent);
}

TEST_F (PlaybackCacheActivityTrackerTest, MultipleEntries)
{
  PlaybackCacheActivityTracker tracker (scheduler_);

  tracker.onRegenerationComplete (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  tracker.onRegenerationComplete (utils::ExpandableTickRange{});
  tracker.onRegenerationComplete (
    utils::ExpandableTickRange{ std::make_pair (20.0, 30.0) });

  EXPECT_EQ (tracker.entries ().size (), 3);
}

// ========================================================================
// Cap enforcement
// ========================================================================

TEST_F (PlaybackCacheActivityTrackerTest, EntriesCappedAtMax)
{
  PlaybackCacheActivityTracker tracker (scheduler_);

  // Add more entries than the cap
  for (
    const auto i :
    std::views::iota (0zu, PlaybackCacheActivityTracker::kMaxEntries + 10))
    {
      tracker.onRegenerationComplete (
        utils::ExpandableTickRange{ std::make_pair (
          static_cast<double> (i), static_cast<double> (i + 1)) });
    }

  EXPECT_EQ (
    tracker.entries ().size (), PlaybackCacheActivityTracker::kMaxEntries);

  // Verify oldest entries were dropped — first entry should be tick 10
  auto first = tracker.entries ().first ().value<PlaybackCacheActivityEntry> ();
  EXPECT_DOUBLE_EQ (first.startTick, 10.0);
}

// ========================================================================
// Auto-removal via sweep timer
// ========================================================================

TEST_F (PlaybackCacheActivityTrackerTest, EntryAutoRemovedAfterTimeout)
{
  PlaybackCacheActivityTracker tracker (scheduler_);

  tracker.onRegenerationComplete (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  ASSERT_EQ (tracker.entries ().size (), 1);

  // Wait longer than the entry lifetime (sweep timer runs at kEntryLifetimeMs
  // intervals)
  QTest::qWait (PlaybackCacheActivityTracker::kEntryLifetimeMs * 3);
  EXPECT_EQ (tracker.entries ().size (), 0);
}

// ========================================================================
// Pending state is sole source of truth
// ========================================================================

TEST_F (
  PlaybackCacheActivityTrackerTest,
  PendingNotModifiedByRegenerationComplete)
{
  PlaybackCacheActivityTracker tracker (scheduler_);

  // Queue a request — pending should become true
  scheduler_->queueCacheRequestForRange (0.0, 10.0);
  EXPECT_TRUE (tracker.isPending ());

  // Regeneration complete should NOT change pending state
  // (only isPendingChanged from scheduler does that)
  tracker.onRegenerationComplete (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  EXPECT_TRUE (tracker.isPending ());
}

} // namespace zrythm::structure::tracks
