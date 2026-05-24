// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/singleton_tracks.h"
#include "structure/tracks/track_all.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <qsignalspy.h>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
class SingletonTracksTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create track registry
    registry_ = std::make_unique<utils::ObjectRegistry> ();

    // Create test dependencies
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();

    // Create singleton tracks
    singleton_tracks = std::make_unique<SingletonTracks> ();
  }

  // Helper to create a chord track
  TrackUuidReference create_chord_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper, *registry_, *transport_, [] { return false; }, {},
    };

    return utils::create_object<ChordTrack> (*registry_, std::move (deps));
  }

  // Helper to create a modulator track
  TrackUuidReference create_modulator_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper, *registry_, *transport_, [] { return false; }, {}
    };

    return utils::create_object<ModulatorTrack> (*registry_, std::move (deps));
  }

  // Helper to create a master track
  TrackUuidReference create_master_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper, *registry_, *transport_, [] { return false; }, {}
    };

    return utils::create_object<MasterTrack> (*registry_, std::move (deps));
  }

  // Helper to create a marker track
  TrackUuidReference create_marker_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper, *registry_, *transport_, [] { return false; }, {}
    };

    return utils::create_object<MarkerTrack> (*registry_, std::move (deps));
  }

  std::unique_ptr<dsp::TempoMap>                  tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper>           tempo_map_wrapper;
  std::unique_ptr<dsp::graph_test::MockTransport> transport_;
  std::unique_ptr<utils::ObjectRegistry>          registry_;
  std::unique_ptr<SingletonTracks>                singleton_tracks;
};

TEST_F (SingletonTracksTest, InitialState)
{
  // All track pointers should be null initially
  EXPECT_EQ (singleton_tracks->chordTrack (), nullptr);
  EXPECT_EQ (singleton_tracks->modulatorTrack (), nullptr);
  EXPECT_EQ (singleton_tracks->masterTrack (), nullptr);
  EXPECT_EQ (singleton_tracks->markerTrack (), nullptr);
}

TEST_F (SingletonTracksTest, SetChordTrackEmitsSignal)
{
  auto chord_track_ref = create_chord_track ();
  auto chord_track = chord_track_ref.get_object_as<ChordTrack> ();

  QSignalSpy spy (singleton_tracks.get (), &SingletonTracks::chordTrackChanged);
  singleton_tracks->setChordTrack (chord_track);

  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (singleton_tracks->chordTrack (), chord_track);
}

TEST_F (SingletonTracksTest, SetChordTrackNoSignalWhenUnchanged)
{
  auto chord_track_ref = create_chord_track ();
  auto chord_track = chord_track_ref.get_object_as<ChordTrack> ();

  // Set first time
  QSignalSpy spy (singleton_tracks.get (), &SingletonTracks::chordTrackChanged);
  singleton_tracks->setChordTrack (chord_track);
  EXPECT_EQ (spy.count (), 1);

  // Set same track again - should not emit signal
  singleton_tracks->setChordTrack (chord_track);
  EXPECT_EQ (spy.count (), 1); // Count should remain the same
}

TEST_F (SingletonTracksTest, SetToNullEmitsSignal)
{
  auto chord_track_ref = create_chord_track ();
  auto chord_track = chord_track_ref.get_object_as<ChordTrack> ();

  // Set to non-null first
  singleton_tracks->setChordTrack (chord_track);

  // Now set to null
  QSignalSpy spy (singleton_tracks.get (), &SingletonTracks::chordTrackChanged);
  singleton_tracks->setChordTrack (nullptr);

  EXPECT_EQ (spy.count (), 1);
  EXPECT_EQ (singleton_tracks->chordTrack (), nullptr);
}

} // namespace zrythm::structure::tracks
