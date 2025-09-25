// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/chord_track.h"
#include "structure/tracks/marker_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/modulator_track.h"
#include "structure/tracks/singleton_tracks.h"

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
    track_registry = std::make_unique<TrackRegistry> ();

    // Create test dependencies
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    // Create singleton tracks
    singleton_tracks = std::make_unique<SingletonTracks> ();
  }

  // Helper to create a chord track
  TrackUuidReference create_chord_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper,   file_audio_source_registry,
      plugin_registry,      port_registry,
      param_registry,       obj_registry,
      *track_registry,      transport,
      [] { return false; },
    };

    return track_registry->create_object<ChordTrack> (std::move (deps));
  }

  // Helper to create a modulator track
  TrackUuidReference create_modulator_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper,  file_audio_source_registry,
      plugin_registry,     port_registry,
      param_registry,      obj_registry,
      *track_registry,     transport,
      [] { return false; }
    };

    return track_registry->create_object<ModulatorTrack> (std::move (deps));
  }

  // Helper to create a master track
  TrackUuidReference create_master_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper,  file_audio_source_registry,
      plugin_registry,     port_registry,
      param_registry,      obj_registry,
      *track_registry,     transport,
      [] { return false; }
    };

    return track_registry->create_object<MasterTrack> (std::move (deps));
  }

  // Helper to create a marker track
  TrackUuidReference create_marker_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper,  file_audio_source_registry,
      plugin_registry,     port_registry,
      param_registry,      obj_registry,
      *track_registry,     transport,
      [] { return false; }
    };

    return track_registry->create_object<MarkerTrack> (std::move (deps));
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  dsp::PortRegistry                     port_registry;
  dsp::ProcessorParameterRegistry       param_registry{ port_registry };
  structure::arrangement::ArrangerObjectRegistry obj_registry;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;
  plugins::PluginRegistry                        plugin_registry;
  dsp::graph_test::MockTransport                 transport;
  std::unique_ptr<TrackRegistry>                 track_registry;
  std::unique_ptr<SingletonTracks>               singleton_tracks;
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
