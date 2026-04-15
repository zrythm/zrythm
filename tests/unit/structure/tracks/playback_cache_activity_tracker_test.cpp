// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map.h"
#include "dsp/timeline_data_cache.h"
#include "structure/tracks/playback_cache_activity_tracker.h"
#include "utils/playback_cache_scheduler.h"
#include "utils/qt.h"

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
    cache_ = utils::make_qobject_unique<dsp::MidiTimelineDataCache> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100));
  }

  void TearDown () override { delete scheduler_; }

  // Mac and Windows CI runners have timing issues with tight delays.
#if defined(Q_OS_MACOS) || defined(Q_OS_WIN)
  static constexpr auto kDelay = 500ms;
#else
  static constexpr auto kDelay = 50ms;
#endif
  static constexpr auto kWaitTime = (kDelay * 3) / 2;

  PlaybackCacheActivityTracker make_tracker ()
  {
    return PlaybackCacheActivityTracker (
      scheduler_, *cache_, [this] (units::sample_t sample) {
        return tempo_map_->samples_to_tick (sample).in (units::ticks);
      });
  }

  utils::PlaybackCacheScheduler *                     scheduler_{};
  utils::QObjectUniquePtr<dsp::MidiTimelineDataCache> cache_;
  std::unique_ptr<dsp::TempoMap>                      tempo_map_;

private:
  std::unique_ptr<ScopedQCoreApplication> app_;
};

// ========================================================================
// Pending state
// ========================================================================

TEST_F (PlaybackCacheActivityTrackerTest, InitiallyNotPending)
{
  auto       tracker = make_tracker ();
  QSignalSpy pending_spy (
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
  auto tracker = make_tracker ();
  EXPECT_TRUE (tracker.isPending ());
}

TEST_F (PlaybackCacheActivityTrackerTest, PendingFollowsScheduler)
{
  auto       tracker = make_tracker ();
  QSignalSpy pending_spy (
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
  auto       tracker = make_tracker ();
  QSignalSpy entries_spy (
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
  auto tracker = make_tracker ();

  tracker.onRegenerationComplete (utils::ExpandableTickRange{});

  auto entries = tracker.entries ();
  ASSERT_EQ (entries.size (), 1);

  auto entry = entries.first ().value<PlaybackCacheActivityEntry> ();
  EXPECT_TRUE (entry.isFullContent);
}

TEST_F (PlaybackCacheActivityTrackerTest, MultipleEntries)
{
  auto tracker = make_tracker ();

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
  auto tracker = make_tracker ();

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
  auto tracker = make_tracker ();

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
  auto tracker = make_tracker ();

  // Queue a request — pending should become true
  scheduler_->queueCacheRequestForRange (0.0, 10.0);
  EXPECT_TRUE (tracker.isPending ());

  // Regeneration complete should NOT change pending state
  // (only isPendingChanged from scheduler does that)
  tracker.onRegenerationComplete (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  EXPECT_TRUE (tracker.isPending ());
}

// ========================================================================
// Cached ranges from signal
// ========================================================================

TEST_F (PlaybackCacheActivityTrackerTest, CachedRangesUpdatedOnFinalize)
{
  auto       tracker = make_tracker ();
  QSignalSpy ranges_spy (
    &tracker, &PlaybackCacheActivityTracker::cachedRangesChanged);

  juce::MidiMessageSequence seq;
  seq.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq.addEvent (juce::MidiMessage::noteOff (1, 60), 50.0);
  seq.updateMatchedPairs ();

  cache_->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq);
  cache_->finalize_changes ();

  ASSERT_EQ (ranges_spy.count (), 1);
  auto ranges = tracker.cachedRanges ();
  ASSERT_EQ (ranges.size (), 1);

  const auto expected_start_tick =
    tempo_map_->samples_to_tick (units::samples (0)).in (units::ticks);
  const auto expected_end_tick =
    tempo_map_->samples_to_tick (units::samples (100)).in (units::ticks);
  auto range = ranges.first ().value<CachedTickRange> ();
  EXPECT_DOUBLE_EQ (range.startTick, expected_start_tick);
  EXPECT_DOUBLE_EQ (range.endTick, expected_end_tick);
}

TEST_F (PlaybackCacheActivityTrackerTest, CachedRangesClearedOnCacheClear)
{
  auto tracker = make_tracker ();

  juce::MidiMessageSequence seq;
  seq.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq.addEvent (juce::MidiMessage::noteOff (1, 60), 50.0);
  seq.updateMatchedPairs ();
  cache_->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq);
  cache_->finalize_changes ();

  ASSERT_EQ (tracker.cachedRanges ().size (), 1);

  cache_->clear ();
  EXPECT_EQ (tracker.cachedRanges ().size (), 0);
}

} // namespace zrythm::structure::tracks
