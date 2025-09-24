// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/audio_bus_track.h"
#include "structure/tracks/folder_track.h"
#include "structure/tracks/track_collection.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{
class TrackCollectionTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create track registry
    track_registry = std::make_unique<TrackRegistry> ();

    // Create test dependencies
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0 * mp_units::si::hertz);
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    // Create track collection
    track_collection = std::make_unique<TrackCollection> (*track_registry);
  }

  // Helper to create a folder track
  TrackUuidReference create_folder_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper,   file_audio_source_registry,
      plugin_registry,      port_registry,
      param_registry,       obj_registry,
      *track_registry,      transport,
      [] { return false; },
    };

    return track_registry->create_object<FolderTrack> (std::move (deps));
  }

  // Helper to create an audio bus track
  TrackUuidReference create_audio_bus_track ()
  {
    FinalTrackDependencies deps{
      *tempo_map_wrapper,  file_audio_source_registry,
      plugin_registry,     port_registry,
      param_registry,      obj_registry,
      *track_registry,     transport,
      [] { return false; }
    };

    return track_registry->create_object<AudioBusTrack> (std::move (deps));
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
  std::unique_ptr<TrackCollection>               track_collection;
};

TEST_F (TrackCollectionTest, InitialState)
{
  EXPECT_EQ (track_collection->rowCount (), 0);
  EXPECT_EQ (track_collection->track_count (), 0);
}

TEST_F (TrackCollectionTest, AddTracks)
{
  auto folder_track = create_folder_track ();
  auto audio_bus_track = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (audio_bus_track);

  EXPECT_EQ (track_collection->rowCount (), 2);
  EXPECT_EQ (track_collection->track_count (), 2);
}

TEST_F (TrackCollectionTest, RemoveTracks)
{
  auto folder_track = create_folder_track ();
  auto audio_bus_track = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (audio_bus_track);

  EXPECT_EQ (track_collection->rowCount (), 2);

  track_collection->remove_track (folder_track.id ());
  EXPECT_EQ (track_collection->rowCount (), 1);

  track_collection->remove_track (audio_bus_track.id ());
  EXPECT_EQ (track_collection->rowCount (), 0);
}

TEST_F (TrackCollectionTest, MoveTracks)
{
  auto track1 = create_folder_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_folder_track ();

  track_collection->add_track (track1);
  track_collection->add_track (track2);
  track_collection->add_track (track3);

  // Move track3 to position 0
  track_collection->move_track (track3.id (), 0);

  // Verify new order
  EXPECT_EQ (track_collection->get_track_ref_at_index (0).id (), track3.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (1).id (), track1.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (2).id (), track2.id ());
}

TEST_F (TrackCollectionTest, FoldableTrackDetection)
{
  auto folder_track = create_folder_track ();
  auto audio_bus_track = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (audio_bus_track);

  EXPECT_TRUE (track_collection->is_track_foldable (folder_track.id ()));
  EXPECT_FALSE (track_collection->is_track_foldable (audio_bus_track.id ()));
}

TEST_F (TrackCollectionTest, ExpandedStateManagement)
{
  auto folder_track = create_folder_track ();

  track_collection->add_track (folder_track);

  // Initially should be expanded
  EXPECT_TRUE (track_collection->get_track_expanded (folder_track.id ()));

  // Set to collapsed
  track_collection->set_track_expanded (folder_track.id (), false);
  EXPECT_FALSE (track_collection->get_track_expanded (folder_track.id ()));

  // Set back to expanded
  track_collection->set_track_expanded (folder_track.id (), true);
  EXPECT_TRUE (track_collection->get_track_expanded (folder_track.id ()));
}

TEST_F (TrackCollectionTest, FolderParentManagement)
{
  auto folder_track = create_folder_track ();
  auto child_track1 = create_audio_bus_track ();
  auto child_track2 = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (child_track1);
  track_collection->add_track (child_track2);

  // Set folder parent for child tracks
  track_collection->set_folder_parent (child_track1.id (), folder_track.id ());
  track_collection->set_folder_parent (child_track2.id (), folder_track.id ());

  // Verify parent relationships
  EXPECT_EQ (
    track_collection->get_folder_parent (child_track1.id ()).value (),
    folder_track.id ());
  EXPECT_EQ (
    track_collection->get_folder_parent (child_track2.id ()).value (),
    folder_track.id ());

  // Verify child count
  EXPECT_EQ (track_collection->get_child_count (folder_track.id ()), 2);

  // Verify track order (children should be after parent)
  EXPECT_EQ (
    track_collection->get_track_ref_at_index (0).id (), folder_track.id ());
  EXPECT_EQ (
    track_collection->get_track_ref_at_index (1).id (), child_track1.id ());
  EXPECT_EQ (
    track_collection->get_track_ref_at_index (2).id (), child_track2.id ());

  // Remove parent from one child
  track_collection->remove_folder_parent (child_track1.id ());

  // Verify parent removed
  EXPECT_FALSE (
    track_collection->get_folder_parent (child_track1.id ()).has_value ());

  // Verify child count decreased
  EXPECT_EQ (track_collection->get_child_count (folder_track.id ()), 1);

  // Verify track order (removed child should be after folder's last child)
  EXPECT_EQ (
    track_collection->get_track_ref_at_index (0).id (), folder_track.id ());
  EXPECT_EQ (
    track_collection->get_track_ref_at_index (1).id (), child_track2.id ());
  EXPECT_EQ (
    track_collection->get_track_ref_at_index (2).id (), child_track1.id ());
}

TEST_F (TrackCollectionTest, LastChildIndex)
{
  auto folder_track = create_folder_track ();
  auto child_track1 = create_audio_bus_track ();
  auto child_track2 = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (child_track1);
  track_collection->add_track (child_track2);

  // Initially no children, last child index should be parent index
  EXPECT_EQ (
    track_collection->get_last_child_index (folder_track.id ()),
    track_collection->get_track_index (folder_track.id ()));

  // Add children
  track_collection->set_folder_parent (child_track1.id (), folder_track.id ());
  track_collection->set_folder_parent (child_track2.id (), folder_track.id ());

  // Last child index should be index of last child
  EXPECT_EQ (
    track_collection->get_last_child_index (folder_track.id ()),
    track_collection->get_track_index (child_track2.id ()));
}

TEST_F (TrackCollectionTest, DataRoles)
{
  auto folder_track = create_folder_track ();
  auto audio_bus_track = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (audio_bus_track);

  // Test TrackPtrRole
  auto track_var = track_collection->data (
    track_collection->index (0), TrackCollection::TrackPtrRole);
  EXPECT_TRUE (track_var.isValid ());

  // Test TrackFoldableRole
  auto foldable_var = track_collection->data (
    track_collection->index (0), TrackCollection::TrackFoldableRole);
  EXPECT_TRUE (foldable_var.toBool ());

  foldable_var = track_collection->data (
    track_collection->index (1), TrackCollection::TrackFoldableRole);
  EXPECT_FALSE (foldable_var.toBool ());

  // Test TrackExpandedRole
  auto expanded_var = track_collection->data (
    track_collection->index (0), TrackCollection::TrackExpandedRole);
  EXPECT_TRUE (expanded_var.toBool ());

  // Test TrackDepthRole
  auto depth_var = track_collection->data (
    track_collection->index (0), TrackCollection::TrackDepthRole);
  EXPECT_EQ (depth_var.toInt (), 0);

  depth_var = track_collection->data (
    track_collection->index (1), TrackCollection::TrackDepthRole);
  EXPECT_EQ (depth_var.toInt (), 0);
}

TEST_F (TrackCollectionTest, Clear)
{
  auto folder_track = create_folder_track ();
  auto audio_bus_track = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (audio_bus_track);

  EXPECT_EQ (track_collection->rowCount (), 2);

  track_collection->clear ();
  EXPECT_EQ (track_collection->rowCount (), 0);
}

TEST_F (TrackCollectionTest, Serialization)
{
  auto folder_track = create_folder_track ();
  auto child_track = create_audio_bus_track ();

  track_collection->add_track (folder_track);
  track_collection->add_track (child_track);

  // Set up folder relationship and expanded state
  track_collection->set_folder_parent (child_track.id (), folder_track.id ());
  track_collection->set_track_expanded (folder_track.id (), false);

  // Serialize to JSON
  nlohmann::json j;
  to_json (j, *track_collection);

  // Create new collection for deserialization
  auto new_collection = std::make_unique<TrackCollection> (*track_registry);
  from_json (j, *new_collection);

  // Verify deserialization
  EXPECT_EQ (new_collection->rowCount (), 2);
  EXPECT_EQ (new_collection->track_count (), 2);

  // Verify folder relationships
  auto new_folder_track = new_collection->get_track_ref_at_index (0);
  auto new_child_track = new_collection->get_track_ref_at_index (1);

  EXPECT_TRUE (new_collection->is_track_foldable (new_folder_track.id ()));
  EXPECT_EQ (
    new_collection->get_folder_parent (new_child_track.id ()).value (),
    new_folder_track.id ());

  // Verify expanded state
  EXPECT_FALSE (new_collection->get_track_expanded (new_folder_track.id ()));
}

TEST_F (TrackCollectionTest, SoloMuteListenCounts)
{
  // Test initial state
  EXPECT_EQ (track_collection->numSoloedTracks (), 0);
  EXPECT_EQ (track_collection->numMutedTracks (), 0);
  EXPECT_EQ (track_collection->numListenedTracks (), 0);

  // Create tracks
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  // Add tracks and verify counts remain 0 (no tracks are soloed/muted/listened
  // initially)
  track_collection->add_track (track1);
  track_collection->add_track (track2);
  track_collection->add_track (track3);

  EXPECT_EQ (track_collection->numSoloedTracks (), 0);
  EXPECT_EQ (track_collection->numMutedTracks (), 0);
  EXPECT_EQ (track_collection->numListenedTracks (), 0);

  // Get track objects to access their faders
  auto track1_obj = track1.get_object_base ();
  auto track2_obj = track2.get_object_base ();
  auto track3_obj = track3.get_object_base ();

  // Test solo counts
  track1_obj->channel ()->fader ()->get_solo_param ().setBaseValue (
    1.0f); // Solo track1
  EXPECT_EQ (track_collection->numSoloedTracks (), 1);

  track2_obj->channel ()->fader ()->get_solo_param ().setBaseValue (
    1.0f); // Solo track2
  EXPECT_EQ (track_collection->numSoloedTracks (), 2);

  track1_obj->channel ()->fader ()->get_solo_param ().setBaseValue (
    0.0f); // Unsolo track1
  EXPECT_EQ (track_collection->numSoloedTracks (), 1);

  // Test mute counts
  track1_obj->channel ()->fader ()->get_mute_param ().setBaseValue (
    1.0f); // Mute track1
  EXPECT_EQ (track_collection->numMutedTracks (), 1);

  track3_obj->channel ()->fader ()->get_mute_param ().setBaseValue (
    1.0f); // Mute track3
  EXPECT_EQ (track_collection->numMutedTracks (), 2);

  track1_obj->channel ()->fader ()->get_mute_param ().setBaseValue (
    0.0f); // Unmute track1
  EXPECT_EQ (track_collection->numMutedTracks (), 1);

  // Test listen counts
  track2_obj->channel ()->fader ()->get_listen_param ().setBaseValue (
    1.0f); // Listen track2
  EXPECT_EQ (track_collection->numListenedTracks (), 1);

  track3_obj->channel ()->fader ()->get_listen_param ().setBaseValue (
    1.0f); // Listen track3
  EXPECT_EQ (track_collection->numListenedTracks (), 2);

  track2_obj->channel ()->fader ()->get_listen_param ().setBaseValue (
    0.0f); // Unlisten track2
  EXPECT_EQ (track_collection->numListenedTracks (), 1);

  // Remove a track and verify counts update
  track_collection->remove_track (track3.id ());
  EXPECT_EQ (track_collection->numMutedTracks (), 0);    // track3 was muted
  EXPECT_EQ (track_collection->numListenedTracks (), 0); // track3 was listened

  // Clear all tracks and verify counts are 0
  track_collection->clear ();
  EXPECT_EQ (track_collection->numSoloedTracks (), 0);
  EXPECT_EQ (track_collection->numMutedTracks (), 0);
  EXPECT_EQ (track_collection->numListenedTracks (), 0);
}

TEST_F (TrackCollectionTest, SoloMuteListenSignals)
{
  // Test that signals are emitted when tracks are added/removed and when
  // parameters change
  auto track = create_audio_bus_track ();

  // Count signal emissions
  int soloed_signals = 0;
  int muted_signals = 0;
  int listened_signals = 0;

  QObject::connect (
    track_collection.get (), &TrackCollection::numSoloedTracksChanged,
    track_collection.get (), [&soloed_signals] () { soloed_signals++; });
  QObject::connect (
    track_collection.get (), &TrackCollection::numMutedTracksChanged,
    track_collection.get (), [&muted_signals] () { muted_signals++; });
  QObject::connect (
    track_collection.get (), &TrackCollection::numListenedTracksChanged,
    track_collection.get (), [&listened_signals] () { listened_signals++; });

  // Add track - should emit signals for all counts (even if 0)
  track_collection->add_track (track);

  // Signals should be emitted when track is added
  EXPECT_GT (soloed_signals, 0);
  EXPECT_GT (muted_signals, 0);
  EXPECT_GT (listened_signals, 0);

  // Reset counters
  soloed_signals = 0;
  muted_signals = 0;
  listened_signals = 0;

  // Change parameters and verify signals are emitted
  auto track_obj = track.get_object_base ();
  track_obj->channel ()->fader ()->get_solo_param ().setBaseValue (1.0f);
  EXPECT_GT (soloed_signals, 0);

  track_obj->channel ()->fader ()->get_mute_param ().setBaseValue (1.0f);
  EXPECT_GT (muted_signals, 0);

  track_obj->channel ()->fader ()->get_listen_param ().setBaseValue (1.0f);
  EXPECT_GT (listened_signals, 0);

  // Remove track - should emit signals
  soloed_signals = 0;
  muted_signals = 0;
  listened_signals = 0;

  track_collection->remove_track (track.id ());
  EXPECT_GT (soloed_signals, 0);
  EXPECT_GT (muted_signals, 0);
  EXPECT_GT (listened_signals, 0);
}

} // namespace zrythm::structure::tracks
