// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/midi_track.h"
#include "structure/tracks/playback_cache_activity_aggregator.h"
#include "structure/tracks/track_collection.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>
#include <QTest>

#include "helpers/scoped_qcoreapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

using namespace zrythm::test_helpers;

namespace zrythm::structure::tracks
{

class PlaybackCacheActivityAggregatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<ScopedQCoreApplication> ();

    registry_ = std::make_unique<utils::ObjectRegistry> ();
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    track_collection = std::make_unique<TrackCollection> (*registry_);
    aggregator = std::make_unique<PlaybackCacheActivityAggregator> ();
    aggregator->setCollection (track_collection.get ());
  }

  TrackUuidReference create_midi_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper, *registry_, *transport_, [] { return false; }, {},
    };
    return utils::create_object<MidiTrack> (*registry_, std::move (deps));
  }

  void wait_for_complete_count (int expected, int timeout_ms = 5000)
  {
    ASSERT_TRUE (
      QTest::qWaitFor (
        [this, expected] {
          return aggregator->cacheCompleteCount () >= expected;
        },
        timeout_ms))
      << "Timed out waiting for cacheCompleteCount >= " << expected << " (got "
      << aggregator->cacheCompleteCount () << ")";
  }

  void wait_for_counts_reset (int timeout_ms = 5000)
  {
    ASSERT_TRUE (
      QTest::qWaitFor (
        [this] {
          return aggregator->cachePendingCount () == 0
                 && aggregator->cacheCompleteCount () == 0;
        },
        timeout_ms))
      << "Timed out waiting for counts to reset";
  }

  void wait_for_complete_count_on (
    PlaybackCacheActivityAggregator &agg,
    int                              expected,
    int                              timeout_ms = 5000)
  {
    ASSERT_TRUE (
      QTest::qWaitFor (
        [&agg, expected] { return agg.cacheCompleteCount () >= expected; },
        timeout_ms))
      << "Timed out waiting for cacheCompleteCount >= " << expected;
  }

  std::unique_ptr<ScopedQCoreApplication>          app_;
  std::unique_ptr<dsp::TempoMap>                   tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper>            tempo_map_wrapper;
  std::unique_ptr<dsp::graph_test::MockTransport>  transport_;
  std::unique_ptr<utils::ObjectRegistry>           registry_;
  std::unique_ptr<TrackCollection>                 track_collection;
  std::unique_ptr<PlaybackCacheActivityAggregator> aggregator;
};

TEST_F (PlaybackCacheActivityAggregatorTest, InitialCountsAreZero)
{
  EXPECT_EQ (aggregator->cachePendingCount (), 0);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);
}

TEST_F (PlaybackCacheActivityAggregatorTest, CompleteCountFromRegeneration)
{
  auto track = create_midi_track ();
  track_collection->add_track (track);
  auto * track_obj = track.get ();

  track_obj->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  track_obj->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (20.0, 30.0) });

  wait_for_complete_count (2);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 2);
}

TEST_F (PlaybackCacheActivityAggregatorTest, CountsResetOnRemove)
{
  auto track = create_midi_track ();
  track_collection->add_track (track);
  auto * track_obj = track.get ();

  track_obj->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  wait_for_complete_count (1);

  EXPECT_GT (aggregator->cacheCompleteCount (), 0);

  track_collection->remove_track (track.id ());
  wait_for_counts_reset ();
  EXPECT_EQ (aggregator->cachePendingCount (), 0);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);
}

TEST_F (PlaybackCacheActivityAggregatorTest, CountsResetOnRemoveAllTracks)
{
  auto track1 = create_midi_track ();
  auto track2 = create_midi_track ();
  track_collection->add_track (track1);
  track_collection->add_track (track2);

  track1.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  track2.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });

  wait_for_complete_count (2);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 2);

  track_collection->remove_track (track1.id ());
  track_collection->remove_track (track2.id ());
  wait_for_counts_reset ();
  EXPECT_EQ (aggregator->cachePendingCount (), 0);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);
}

TEST_F (PlaybackCacheActivityAggregatorTest, MultipleTracksCompleteCount)
{
  auto track1 = create_midi_track ();
  auto track2 = create_midi_track ();
  track_collection->add_track (track1);
  track_collection->add_track (track2);

  track1.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  track2.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  track1.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (20.0, 30.0) });

  wait_for_complete_count (3);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 3);
}

TEST_F (PlaybackCacheActivityAggregatorTest, SignalsEmittedOnCountChange)
{
  QSignalSpy complete_spy (
    aggregator.get (),
    &PlaybackCacheActivityAggregator::cacheCompleteCountChanged);

  auto track = create_midi_track ();
  track_collection->add_track (track);

  track.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });

  ASSERT_TRUE (complete_spy.wait (5000));
  EXPECT_GE (complete_spy.count (), 1);
}

TEST_F (PlaybackCacheActivityAggregatorTest, DetectsExistingTracksOnSetCollection)
{
  auto track = create_midi_track ();
  track_collection->add_track (track);

  // Create aggregator AFTER track is already in the collection
  auto late_aggregator = std::make_unique<PlaybackCacheActivityAggregator> ();
  late_aggregator->setCollection (track_collection.get ());

  track.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });

  wait_for_complete_count_on (*late_aggregator, 1);
  EXPECT_EQ (late_aggregator->cacheCompleteCount (), 1);
}

TEST_F (
  PlaybackCacheActivityAggregatorTest,
  TracksAddedAfterRemovalAreStillTracked)
{
  // Add a track and generate some cache activity
  auto track1 = create_midi_track ();
  track_collection->add_track (track1);
  track1.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  wait_for_complete_count (1);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 1);

  track_collection->remove_track (track1.id ());
  wait_for_counts_reset ();
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);

  // Add a new track after the removal — the aggregator should still be
  // connected to rowsInserted, so this track's activity should be picked up.
  auto track2 = create_midi_track ();
  track_collection->add_track (track2);
  track2.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (5.0, 15.0) });
  wait_for_complete_count (1);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 1);

  // A second removal should also work (verifies the connection survived)
  track_collection->remove_track (track2.id ());
  wait_for_counts_reset ();
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);
}

TEST_F (PlaybackCacheActivityAggregatorTest, SignalsEmittedOnRemoval)
{
  QSignalSpy pending_spy (
    aggregator.get (),
    &PlaybackCacheActivityAggregator::cachePendingCountChanged);
  QSignalSpy complete_spy (
    aggregator.get (),
    &PlaybackCacheActivityAggregator::cacheCompleteCountChanged);

  auto track = create_midi_track ();
  track_collection->add_track (track);

  track.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  ASSERT_TRUE (complete_spy.wait (5000));

  // Clear spy counts so we only measure the removal-related emissions
  pending_spy.clear ();
  complete_spy.clear ();

  // Remove the track — disconnectAll() emits both signals
  track_collection->remove_track (track.id ());

  EXPECT_GE (pending_spy.count (), 1);
  EXPECT_GE (complete_spy.count (), 1);
  EXPECT_EQ (aggregator->cachePendingCount (), 0);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);
}

TEST_F (
  PlaybackCacheActivityAggregatorTest,
  AutomationTrackCountsResetOnTrackRemoval)
{
  // Create a track and add it to the collection
  auto track = create_midi_track ();
  track_collection->add_track (track);
  auto * track_obj = track.get ();
  auto * atl = track_obj->automationTracklist ();
  ASSERT_NE (atl, nullptr);

  // Add an automation track
  auto param_id = utils::create_object<dsp::ProcessorParameter> (
    *registry_, *registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Test Param");
  auto at = utils::make_qobject_unique<AutomationTrack> (
    *tempo_map_wrapper, *registry_, std::move (param_id));
  auto * at_ptr = at.get ();
  atl->add_automation_track (std::move (at));

  // Trigger cache regeneration on the automation track
  at_ptr->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  wait_for_complete_count (1);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 1);

  // Remove the track — aggregator should reset all counts including
  // automation track entries
  track_collection->remove_track (track.id ());
  wait_for_counts_reset ();
  EXPECT_EQ (aggregator->cachePendingCount (), 0);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);
}

TEST_F (PlaybackCacheActivityAggregatorTest, CountsResetOnSetCollectionNull)
{
  auto track = create_midi_track ();
  track_collection->add_track (track);
  track.get ()->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  wait_for_complete_count (1);
  EXPECT_GT (aggregator->cacheCompleteCount (), 0);

  // Setting collection to null should clear all counts
  aggregator->setCollection (nullptr);
  EXPECT_EQ (aggregator->cachePendingCount (), 0);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);
}

TEST_F (
  PlaybackCacheActivityAggregatorTest,
  AutomationTrackCountsResetOnIndividualRemoval)
{
  auto track = create_midi_track ();
  track_collection->add_track (track);
  auto * track_obj = track.get ();
  auto * atl = track_obj->automationTracklist ();
  ASSERT_NE (atl, nullptr);

  // Add an automation track
  auto param_id = utils::create_object<dsp::ProcessorParameter> (
    *registry_, *registry_, dsp::ProcessorParameter::UniqueId (u8"test_param"),
    dsp::ParameterRange (dsp::ParameterRange::Type::Linear, 0.0f, 1.0f),
    u8"Test Param");
  auto at = utils::make_qobject_unique<AutomationTrack> (
    *tempo_map_wrapper, *registry_, std::move (param_id));
  auto * at_ptr = at.get ();
  atl->add_automation_track (std::move (at));

  // Trigger cache regeneration on the automation track
  at_ptr->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (0.0, 10.0) });
  wait_for_complete_count (1);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 1);

  // Remove only the automation track (not the whole track)
  atl->remove_automation_track (*at_ptr);
  wait_for_counts_reset ();
  EXPECT_EQ (aggregator->cachePendingCount (), 0);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 0);

  // Track itself should still be tracked
  track_obj->regeneratePlaybackCaches (
    utils::ExpandableTickRange{ std::make_pair (20.0, 30.0) });
  wait_for_complete_count (1);
  EXPECT_EQ (aggregator->cacheCompleteCount (), 1);
}

} // namespace zrythm::structure::tracks
