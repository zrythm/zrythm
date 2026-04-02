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
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
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

  // Initial order: track1, track2, track3
  EXPECT_EQ (track_collection->get_track_ref_at_index (0).id (), track1.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (1).id (), track2.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (2).id (), track3.id ());

  // Move track3 to position 0 (backward move)
  track_collection->move_track (track3.id (), 0);

  // New order: track3, track1, track2
  EXPECT_EQ (track_collection->get_track_ref_at_index (0).id (), track3.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (1).id (), track1.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (2).id (), track2.id ());
}

TEST_F (TrackCollectionTest, MoveTrackToLastPosition)
{
  auto track1 = create_folder_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_folder_track ();

  track_collection->add_track (track1);
  track_collection->add_track (track2);
  track_collection->add_track (track3);

  // Initial order: track1, track2, track3 (indices 0, 1, 2)
  // Move track1 to position 2 (last valid index)
  track_collection->move_track (track1.id (), 2);

  // Expected: track1 should end at index 2, others shift up
  // track2 moves to 0, track3 moves to 1, track1 at 2
  // Result: track2, track3, track1
  EXPECT_EQ (track_collection->get_track_ref_at_index (0).id (), track2.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (1).id (), track3.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (2).id (), track1.id ());
}

TEST_F (TrackCollectionTest, MoveTrackToMiddlePosition)
{
  auto track1 = create_folder_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_folder_track ();
  auto track4 = create_audio_bus_track ();

  track_collection->add_track (track1);
  track_collection->add_track (track2);
  track_collection->add_track (track3);
  track_collection->add_track (track4);

  // Initial order: track1, track2, track3, track4 (indices 0, 1, 2, 3)
  // Move track1 to position 2
  track_collection->move_track (track1.id (), 2);

  // Expected: track1 ends at index 2
  // track2 at 0, track3 at 1, track1 at 2, track4 at 3
  // Result: track2, track3, track1, track4
  EXPECT_EQ (track_collection->get_track_ref_at_index (0).id (), track2.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (1).id (), track3.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (2).id (), track1.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (3).id (), track4.id ());
}

TEST_F (TrackCollectionTest, MoveTrackToSamePosition)
{
  auto track1 = create_folder_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_folder_track ();

  track_collection->add_track (track1);
  track_collection->add_track (track2);
  track_collection->add_track (track3);

  // Move track2 to position 1 (where it already is) - should be no-op
  track_collection->move_track (track2.id (), 1);

  // Order should remain unchanged
  EXPECT_EQ (track_collection->get_track_ref_at_index (0).id (), track1.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (1).id (), track2.id ());
  EXPECT_EQ (track_collection->get_track_ref_at_index (2).id (), track3.id ());
}

TEST_F (TrackCollectionTest, MoveTrackBeyondLastPosition)
{
  auto track1 = create_folder_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_folder_track ();

  track_collection->add_track (track1);
  track_collection->add_track (track2);
  track_collection->add_track (track3);

  // Initial order: track1, track2, track3 (indices 0, 1, 2)
  // Try to move track1 to position 3 (beyond last valid index 2)
  // This tests whether move_track handles out-of-bounds gracefully
  // Current implementation will crash/assert - this test documents that
  EXPECT_DEATH (track_collection->move_track (track1.id (), 3), "");
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

  // Remove parent from one child (auto-reposition moves it after folder's last
  // child)
  track_collection->remove_folder_parent (child_track1.id (), true);

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

// Test set_folder_parent without auto_reposition keeps the track in place.
TEST_F (TrackCollectionTest, SetFolderParentWithoutReposition)
{
  auto folder = create_folder_track ();
  auto child = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, other, child]
  track_collection->add_track (folder);
  track_collection->add_track (other);
  track_collection->add_track (child);

  // Set parent without repositioning - child stays at index 2
  track_collection->set_folder_parent (child.id (), folder.id ());

  ASSERT_TRUE (track_collection->get_folder_parent (child.id ()).has_value ());
  EXPECT_EQ (
    track_collection->get_folder_parent (child.id ()).value (), folder.id ());

  // Position should NOT have changed
  EXPECT_EQ (track_collection->get_track_index (child.id ()), 2);
}

// Test remove_folder_parent without auto_reposition keeps the track in place.
TEST_F (TrackCollectionTest, RemoveFolderParentWithoutReposition)
{
  auto folder = create_folder_track ();
  auto child = create_audio_bus_track ();

  // Layout: [folder, child]
  track_collection->add_track (folder);
  track_collection->add_track (child);
  track_collection->set_folder_parent (child.id (), folder.id ());

  // Remove parent without repositioning
  track_collection->remove_folder_parent (child.id ());

  EXPECT_FALSE (track_collection->get_folder_parent (child.id ()).has_value ());
  // Position should not change
  EXPECT_EQ (track_collection->get_track_index (child.id ()), 1);
}

// Test is_ancestor_of for direct and indirect ancestors.
TEST_F (TrackCollectionTest, IsAncestorOf)
{
  auto folder1 = create_folder_track ();
  auto folder2 = create_folder_track ();
  auto child = create_audio_bus_track ();

  // Layout: [folder1, folder2, child]
  track_collection->add_track (folder1);
  track_collection->add_track (folder2);
  track_collection->add_track (child);

  track_collection->set_folder_parent (folder2.id (), folder1.id ());
  track_collection->set_folder_parent (child.id (), folder2.id ());

  // Direct parent
  EXPECT_TRUE (track_collection->is_ancestor_of (folder2.id (), child.id ()));
  // Grandparent
  EXPECT_TRUE (track_collection->is_ancestor_of (folder1.id (), child.id ()));
  // Not an ancestor
  EXPECT_FALSE (track_collection->is_ancestor_of (child.id (), folder1.id ()));
  // Self is not ancestor
  EXPECT_FALSE (track_collection->is_ancestor_of (folder1.id (), folder1.id ()));
}

// Test get_enclosing_folder with expanded folder.
TEST_F (TrackCollectionTest, GetEnclosingFolderExpanded)
{
  auto folder = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child1, child2, other]
  track_collection->add_track (folder);
  track_collection->add_track (child1);
  track_collection->add_track (child2);
  track_collection->add_track (other);

  track_collection->set_folder_parent (child1.id (), folder.id ());
  track_collection->set_folder_parent (child2.id (), folder.id ());
  track_collection->set_track_expanded (folder.id (), true);

  // Indices inside folder's child range
  auto enclosing1 = track_collection->get_enclosing_folder (1);
  ASSERT_TRUE (enclosing1.has_value ());
  EXPECT_EQ (enclosing1.value (), folder.id ());

  auto enclosing2 = track_collection->get_enclosing_folder (2);
  ASSERT_TRUE (enclosing2.has_value ());
  EXPECT_EQ (enclosing2.value (), folder.id ());

  // Index outside folder's child range
  EXPECT_FALSE (track_collection->get_enclosing_folder (3).has_value ());
}

// Test get_enclosing_folder with collapsed folder returns nullopt.
TEST_F (TrackCollectionTest, GetEnclosingFolderCollapsed)
{
  auto folder = create_folder_track ();
  auto child = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child, other]
  track_collection->add_track (folder);
  track_collection->add_track (child);
  track_collection->add_track (other);

  track_collection->set_folder_parent (child.id (), folder.id ());
  track_collection->set_track_expanded (folder.id (), false);

  // Collapsed folder should not enclose
  EXPECT_FALSE (track_collection->get_enclosing_folder (1).has_value ());
}

// Test get_enclosing_folder with nested folders returns innermost.
TEST_F (TrackCollectionTest, GetEnclosingFolderNested)
{
  auto outer = create_folder_track ();
  auto inner = create_folder_track ();
  auto child = create_audio_bus_track ();

  // Layout: [outer, inner, child]
  track_collection->add_track (outer);
  track_collection->add_track (inner);
  track_collection->add_track (child);

  track_collection->set_folder_parent (inner.id (), outer.id ());
  track_collection->set_folder_parent (child.id (), inner.id ());
  track_collection->set_track_expanded (outer.id (), true);
  track_collection->set_track_expanded (inner.id (), true);

  // Child at index 2 should be enclosed by inner (not outer)
  auto enclosing = track_collection->get_enclosing_folder (2);
  ASSERT_TRUE (enclosing.has_value ());
  EXPECT_EQ (enclosing.value (), inner.id ());
}

// Test get_enclosing_folder when no folder exists.
TEST_F (TrackCollectionTest, GetEnclosingFolderNoFolder)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();

  track_collection->add_track (track1);
  track_collection->add_track (track2);

  EXPECT_FALSE (track_collection->get_enclosing_folder (1).has_value ());
  EXPECT_FALSE (track_collection->get_enclosing_folder (0).has_value ());
}

// Test get_enclosing_folder with a folder containing a direct child followed
// by a subfolder with its own children. The outer folder's child range should
// cover only its direct children; grandchildren are enclosed by the inner folder.
TEST_F (TrackCollectionTest, GetEnclosingFolderWithNestedSubfolder)
{
  auto outer = create_folder_track ();
  auto direct_child = create_audio_bus_track ();
  auto inner = create_folder_track ();
  auto grandchild1 = create_audio_bus_track ();
  auto grandchild2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [outer, direct_child, inner, grandchild1, grandchild2, other]
  track_collection->add_track (outer);
  track_collection->add_track (direct_child);
  track_collection->add_track (inner);
  track_collection->add_track (grandchild1);
  track_collection->add_track (grandchild2);
  track_collection->add_track (other);

  track_collection->set_folder_parent (direct_child.id (), outer.id ());
  track_collection->set_folder_parent (inner.id (), outer.id ());
  track_collection->set_folder_parent (grandchild1.id (), inner.id ());
  track_collection->set_folder_parent (grandchild2.id (), inner.id ());
  track_collection->set_track_expanded (outer.id (), true);
  track_collection->set_track_expanded (inner.id (), true);

  // grandchild1 at index 3 is inside inner.
  auto enclosing3 = track_collection->get_enclosing_folder (3);
  ASSERT_TRUE (enclosing3.has_value ()) << "index 3 should be enclosed by inner";
  EXPECT_EQ (enclosing3.value (), inner.id ());

  // grandchild2 at index 4 is inside inner.
  auto enclosing4 = track_collection->get_enclosing_folder (4);
  ASSERT_TRUE (enclosing4.has_value ()) << "index 4 should be enclosed by inner";
  EXPECT_EQ (enclosing4.value (), inner.id ());

  // inner itself at index 2 is enclosed by outer.
  auto enclosing2 = track_collection->get_enclosing_folder (2);
  ASSERT_TRUE (enclosing2.has_value ()) << "index 2 should be enclosed by outer";
  EXPECT_EQ (enclosing2.value (), outer.id ());

  // direct_child at index 1 is enclosed by outer.
  auto enclosing1 = track_collection->get_enclosing_folder (1);
  ASSERT_TRUE (enclosing1.has_value ());
  EXPECT_EQ (enclosing1.value (), outer.id ());

  // other at index 5 should NOT be enclosed by any folder.
  EXPECT_FALSE (track_collection->get_enclosing_folder (5).has_value ());
}

// Test reattach_track at boundary positions (0, size, and beyond).
TEST_F (TrackCollectionTest, ReattachTrackBoundaryPositions)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();

  // Start with one track
  track_collection->add_track (track1);
  ASSERT_EQ (track_collection->rowCount (), 1);

  // Detach and reattach at position 0
  track_collection->detach_track (track1.id ());
  EXPECT_EQ (track_collection->rowCount (), 0);
  track_collection->reattach_track (track1, 0);
  EXPECT_EQ (track_collection->rowCount (), 1);
  EXPECT_EQ (track_collection->get_track_index (track1.id ()), 0u);

  // Add second track, detach first, reattach at end (position == size)
  track_collection->add_track (track2);
  track_collection->detach_track (track1.id ());
  EXPECT_EQ (track_collection->rowCount (), 1);
  track_collection->reattach_track (
    track1, 1); // position == size (valid append)
  EXPECT_EQ (track_collection->get_track_index (track1.id ()), 1u);

  // Detach and reattach at position beyond size - should clamp to end
  track_collection->detach_track (track1.id ());
  EXPECT_EQ (track_collection->rowCount (), 1);
  track_collection->reattach_track (track1, 99); // way past end
  EXPECT_EQ (track_collection->get_track_index (track1.id ()), 1u)
    << "reattach_track should clamp out-of-range positions";

  // Negative position should clamp to 0
  track_collection->detach_track (track2.id ());
  track_collection->reattach_track (track2, -5);
  EXPECT_EQ (track_collection->get_track_index (track2.id ()), 0u)
    << "reattach_track should clamp negative positions to 0";
}

// Test get_child_count and get_last_child_index include all descendants
// (not just direct children) for nested folders.
TEST_F (TrackCollectionTest, ChildCountIncludesNestedDescendants)
{
  auto folderA = create_folder_track ();
  auto folderB = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folderA, folderB, child1, child2, other]
  track_collection->add_track (folderA);
  track_collection->add_track (folderB);
  track_collection->add_track (child1);
  track_collection->add_track (child2);
  track_collection->add_track (other);

  // folderB is child of folderA
  track_collection->set_folder_parent (folderB.id (), folderA.id ());
  // child1, child2 are children of folderB (grandchildren of folderA)
  track_collection->set_folder_parent (child1.id (), folderB.id ());
  track_collection->set_folder_parent (child2.id (), folderB.id ());

  // folderA has 3 descendants: folderB, child1, child2
  EXPECT_EQ (track_collection->get_child_count (folderA.id ()), 3)
    << "get_child_count should count all descendants, not just direct children";

  // get_last_child_index should point to child2 (index 3), not folderB (index 1)
  EXPECT_EQ (track_collection->get_last_child_index (folderA.id ()), 3)
    << "get_last_child_index should include all descendants";
}

// Test that set_folder_parent places a new child after all descendants when the
// folder has nested subfolders.
TEST_F (TrackCollectionTest, SetFolderParentPlacementWithNestedFolders)
{
  auto folderA = create_folder_track ();
  auto folderB = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto newChild = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folderA, folderB, child1, newChild, other]
  track_collection->add_track (folderA);
  track_collection->add_track (folderB);
  track_collection->add_track (child1);
  track_collection->add_track (newChild);
  track_collection->add_track (other);

  track_collection->set_folder_parent (folderB.id (), folderA.id ());
  track_collection->set_folder_parent (child1.id (), folderB.id ());
  track_collection->set_track_expanded (folderA.id (), true);
  track_collection->set_track_expanded (folderB.id (), true);

  // Add newChild to folderA. With the bug, get_last_child_index returns 1
  // (just folderB), so newChild would move to index 2 (between folderB and
  // child1). With the fix, get_last_child_index returns 2 (including child1),
  // so newChild stays at index 3 (right after all descendants).
  track_collection->set_folder_parent (newChild.id (), folderA.id (), true);

  // newChild should be directly child of folderA
  ASSERT_TRUE (
    track_collection->get_folder_parent (newChild.id ()).has_value ());
  EXPECT_EQ (
    track_collection->get_folder_parent (newChild.id ()).value (),
    folderA.id ());

  // newChild should be at index 3 (after folderB and child1, before other)
  EXPECT_EQ (track_collection->get_track_index (newChild.id ()), 3)
    << "newChild should be placed after all of folderA's descendants";
}

// Test get_all_descendants returns direct and nested children in list order.
TEST_F (TrackCollectionTest, GetAllDescendantsNested)
{
  auto folderA = create_folder_track ();
  auto folderB = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  track_collection->add_track (folderA);
  track_collection->add_track (folderB);
  track_collection->add_track (child1);
  track_collection->add_track (child2);
  track_collection->add_track (other);

  track_collection->set_folder_parent (folderB.id (), folderA.id ());
  track_collection->set_folder_parent (child1.id (), folderB.id ());
  track_collection->set_folder_parent (child2.id (), folderB.id ());
  track_collection->set_track_expanded (folderA.id (), true);
  track_collection->set_track_expanded (folderB.id (), true);

  auto descendants = track_collection->get_all_descendants (folderA.id ());
  ASSERT_EQ (descendants.size (), 3u);
  EXPECT_EQ (descendants[0], folderB.id ());
  EXPECT_EQ (descendants[1], child1.id ());
  EXPECT_EQ (descendants[2], child2.id ());

  // folderB's descendants are just its direct children
  auto b_descendants = track_collection->get_all_descendants (folderB.id ());
  ASSERT_EQ (b_descendants.size (), 2u);
  EXPECT_EQ (b_descendants[0], child1.id ());
  EXPECT_EQ (b_descendants[1], child2.id ());

  // Non-foldable track has no descendants
  EXPECT_TRUE (track_collection->get_all_descendants (child1.id ()).empty ());
}

} // namespace zrythm::structure::tracks
