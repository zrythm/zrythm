// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/move_tracks_command.h"
#include "structure/tracks/folder_track.h"
#include "structure/tracks/track_collection.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{

class MoveTracksCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create track registry
    track_registry_ = std::make_unique<structure::tracks::TrackRegistry> ();

    // Create tempo map
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);

    // Create track collection
    collection_ =
      std::make_unique<structure::tracks::TrackCollection> (*track_registry_);
  }

  // Helper to create default track dependencies
  structure::tracks::FinalTrackDependencies make_deps ()
  {
    return structure::tracks::FinalTrackDependencies{
      *tempo_map_wrapper_,  file_audio_source_registry_,
      plugin_registry_,     port_registry_,
      param_registry_,      obj_registry_,
      *track_registry_,     transport_,
      [] { return false; },
    };
  }

  // Helper to create an audio bus track
  structure::tracks::TrackUuidReference create_audio_bus_track ()
  {
    return track_registry_->create_object<structure::tracks::AudioBusTrack> (
      make_deps ());
  }

  // Helper to create a folder track
  structure::tracks::TrackUuidReference create_folder_track ()
  {
    return track_registry_->create_object<structure::tracks::FolderTrack> (
      make_deps ());
  }

  // Test dependencies
  std::unique_ptr<dsp::TempoMap>        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper_;
  dsp::PortRegistry                     port_registry_;
  dsp::ProcessorParameterRegistry       param_registry_{ port_registry_ };
  structure::arrangement::ArrangerObjectRegistry    obj_registry_;
  dsp::FileAudioSourceRegistry                      file_audio_source_registry_;
  plugins::PluginRegistry                           plugin_registry_;
  dsp::graph_test::MockTransport                    transport_;
  std::unique_ptr<structure::tracks::TrackRegistry> track_registry_;
  std::unique_ptr<structure::tracks::TrackCollection> collection_;
};

// Test initial state and command construction
TEST_F (MoveTracksCommandTest, InitialState)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);

  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 2);

  // Command should be constructed without error
  // Initial positions should be stored internally
  EXPECT_EQ (collection_->track_count (), 3);
}

// Test moving a single track forward
TEST_F (MoveTracksCommandTest, MoveSingleTrackForward)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);

  // Initial order: track1, track2, track3
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);

  // Move track1 to the end (pre-removal target 3 = past-the-end)
  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 3);
  command.redo ();

  // New order: track2, track3, track1
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 2);

  // Undo should restore original order
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
}

// Test moving a single track backward
TEST_F (MoveTracksCommandTest, MoveSingleTrackBackward)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);

  // Initial order: track1, track2, track3
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);

  // Move track3 to position 0
  std::vector<structure::tracks::TrackUuidReference> tracks{ track3 };
  MoveTracksCommand command (*collection_, tracks, 0);
  command.redo ();

  // New order: track3, track1, track2
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 2);

  // Undo should restore original order
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
}

// Test moving multiple adjacent tracks together
TEST_F (MoveTracksCommandTest, MoveMultipleAdjacentTracks)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();
  auto track4 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);
  collection_->add_track (track4);

  // Initial order: track1, track2, track3, track4
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track4.id ()), 3);

  // Move track2 and track3 (adjacent) after track4
  // Pre-removal target 4 = past-the-end in [T1,T2,T3,T4]
  std::vector<structure::tracks::TrackUuidReference> tracks{ track2, track3 };
  MoveTracksCommand command (*collection_, tracks, 4);
  command.redo ();

  // New order: track1, track4, track2, track3
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track4.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 3);

  // Undo should restore original order
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track4.id ()), 3);
}

// Test multiple undo/redo cycles
TEST_F (MoveTracksCommandTest, MultipleUndoRedoCycles)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);

  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 3);

  // First cycle
  command.redo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 2);

  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);

  // Second cycle
  command.redo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 2);

  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
}

// Test command text
TEST_F (MoveTracksCommandTest, CommandText)
{
  auto track1 = create_audio_bus_track ();
  collection_->add_track (track1);

  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 0);

  EXPECT_EQ (command.text (), QString ("Move Tracks"));
}

// Test command ID
TEST_F (MoveTracksCommandTest, CommandId)
{
  auto track1 = create_audio_bus_track ();
  collection_->add_track (track1);

  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 0);

  EXPECT_EQ (command.id (), 1774496026);
}

// Test move to same position (should be no-op in terms of order)
TEST_F (MoveTracksCommandTest, MoveToSamePosition)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);

  // Move track2 to position 1 (where it already is)
  std::vector<structure::tracks::TrackUuidReference> tracks{ track2 };
  MoveTracksCommand command (*collection_, tracks, 1);
  command.redo ();

  // Order should remain unchanged
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
}

// Test moving to end of list
TEST_F (MoveTracksCommandTest, MoveToEndOfList)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);

  // Move track1 to last position (pre-removal target 3 = past-the-end)
  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 3);
  command.redo ();

  // New order: track2, track3, track1
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 2);

  // Undo should restore original order
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
}

// Test moving non-adjacent tracks
TEST_F (MoveTracksCommandTest, MoveNonAdjacentTracks)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();
  auto track4 = create_audio_bus_track ();
  auto track5 = create_audio_bus_track ();

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);
  collection_->add_track (track4);
  collection_->add_track (track5);

  // Move track2 and track4 (non-adjacent) after track5
  // Pre-removal target 5 = past-the-end in [T1,T2,T3,T4,T5]
  std::vector<structure::tracks::TrackUuidReference> tracks{ track2, track4 };
  MoveTracksCommand command (*collection_, tracks, 5);
  command.redo ();

  // New order: track1, track3, track5, track2, track4
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track5.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 3);
  EXPECT_EQ (collection_->get_track_index (track4.id ()), 4);

  // Undo should restore original order
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track4.id ()), 3);
  EXPECT_EQ (collection_->get_track_index (track5.id ()), 4);
}

// Test moving non-contiguous tracks where some are above and some below the
// target.
TEST_F (MoveTracksCommandTest, MoveNonContiguousTracksAcrossTarget)
{
  auto track0 = create_audio_bus_track ();
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();
  auto track4 = create_audio_bus_track ();
  auto track5 = create_audio_bus_track ();

  collection_->add_track (track0);
  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);
  collection_->add_track (track4);
  collection_->add_track (track5);

  // Initial: [T0, T1, T2, T3, T4, T5]
  // Select T1 (above target), T3 and T4 (below target).
  // Pre-removal target 2 = position of T2 in the current list.
  // After removing selected tracks, remaining: [T0, T2, T5]
  // Insert [T1, T3, T4] at position 1 → [T0, T1, T3, T4, T2, T5]
  std::vector<structure::tracks::TrackUuidReference> tracks{
    track1, track3, track4
  };
  MoveTracksCommand command (*collection_, tracks, 2);
  command.redo ();

  // All 3 moved tracks must be contiguous starting at position 1,
  // with T2 (unselected) after them, not interleaved.
  EXPECT_EQ (collection_->get_track_index (track0.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track4.id ()), 3);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 4);
  EXPECT_EQ (collection_->get_track_index (track5.id ()), 5);

  // Undo should restore original order
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (track0.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track3.id ()), 3);
  EXPECT_EQ (collection_->get_track_index (track4.id ()), 4);
  EXPECT_EQ (collection_->get_track_index (track5.id ()), 5);
}

// Test that moving a folder track with children preserves folder parent
// relationships and expanded state.
TEST_F (MoveTracksCommandTest, MoveFolderTrackWithChildrenPreservesMetadata)
{
  auto folder = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child1, child2, other]
  collection_->add_track (folder);
  collection_->add_track (child1);
  collection_->add_track (child2);
  collection_->add_track (other);

  // Set up folder relationships
  collection_->set_folder_parent (child1.id (), folder.id ());
  collection_->set_folder_parent (child2.id (), folder.id ());

  // Collapse the folder (insert_track auto-expands, so test that collapsed
  // state is preserved)
  collection_->set_track_expanded (folder.id (), false);

  // Verify initial state
  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folder.id ());
  ASSERT_TRUE (collection_->get_folder_parent (child2.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child2.id ()).value (), folder.id ());
  EXPECT_FALSE (collection_->get_track_expanded (folder.id ()));

  // Move folder + children to the end (after 'other')
  // Pre-removal target 4 = past-the-end in [folder,child1,child2,other]
  // This effectively swaps the groups: [other, folder, child1, child2]
  std::vector<structure::tracks::TrackUuidReference> tracks{
    folder, child1, child2
  };
  MoveTracksCommand command (*collection_, tracks, 4);
  command.redo ();

  // Verify positions moved correctly
  EXPECT_EQ (collection_->get_track_index (other.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (child2.id ()), 3);

  // Verify folder parent relationships survived the move
  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ())
    << "child1 lost its folder parent after move";
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folder.id ());
  ASSERT_TRUE (collection_->get_folder_parent (child2.id ()).has_value ())
    << "child2 lost its folder parent after move";
  EXPECT_EQ (
    collection_->get_folder_parent (child2.id ()).value (), folder.id ());

  // Verify expanded state survived (was collapsed, should stay collapsed)
  EXPECT_FALSE (collection_->get_track_expanded (folder.id ()))
    << "folder was collapsed before move but became expanded after";

  // Undo should also preserve metadata
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (child2.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 3);

  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ())
    << "child1 lost its folder parent after undo";
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folder.id ());
  EXPECT_FALSE (collection_->get_track_expanded (folder.id ()))
    << "folder was collapsed before undo but became expanded after";
}

// Test that moving a child track out of a folder preserves the folder parent
// relationship of the remaining children.
TEST_F (MoveTracksCommandTest, MoveChildOutOfFolderPreservesRemainingMetadata)
{
  auto folder = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child1, child2, other]
  collection_->add_track (folder);
  collection_->add_track (child1);
  collection_->add_track (child2);
  collection_->add_track (other);

  collection_->set_folder_parent (child1.id (), folder.id ());
  collection_->set_folder_parent (child2.id (), folder.id ());
  collection_->set_track_expanded (folder.id (), true);

  // Move child1 after 'other' (pre-removal target 4 = past-the-end)
  std::vector<structure::tracks::TrackUuidReference> tracks{ child1 };
  MoveTracksCommand command (*collection_, tracks, 4);
  command.redo ();

  // Verify positions: [folder, child2, other, child1]
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (child2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 3);

  // child2 should still belong to the folder
  ASSERT_TRUE (collection_->get_folder_parent (child2.id ()).has_value ())
    << "child2 lost its folder parent when child1 was moved";
  EXPECT_EQ (
    collection_->get_folder_parent (child2.id ()).value (), folder.id ());

  // child1 should no longer have a folder parent (it moved out)
  EXPECT_FALSE (collection_->get_folder_parent (child1.id ()).has_value ())
    << "child1 should have lost its folder parent after moving out";

  // Folder should still be expanded
  EXPECT_TRUE (collection_->get_track_expanded (folder.id ()));

  // Undo should restore original positions and metadata
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (child2.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 3);

  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ())
    << "child1 lost its folder parent after undo";
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folder.id ());
  ASSERT_TRUE (collection_->get_folder_parent (child2.id ()).has_value ())
    << "child2 lost its folder parent after undo";
  EXPECT_EQ (
    collection_->get_folder_parent (child2.id ()).value (), folder.id ());
  EXPECT_TRUE (collection_->get_track_expanded (folder.id ()));
}

// Test moving tracks into a folder sets folder parent relationships.
TEST_F (MoveTracksCommandTest, MoveIntoFolder)
{
  auto folder = create_folder_track ();
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, other, track1, track2]
  collection_->add_track (folder);
  collection_->add_track (other);
  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->set_track_expanded (folder.id (), true);

  // Move track1 and track2 into the folder (at position 1, right after folder)
  std::vector<structure::tracks::TrackUuidReference> tracks{ track1, track2 };
  MoveTracksCommand command (*collection_, tracks, 1, folder.id ());
  command.redo ();

  // Verify positions: [folder, track1, track2, other]
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 3);

  // Verify folder parent relationships
  ASSERT_TRUE (collection_->get_folder_parent (track1.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (track1.id ()).value (), folder.id ());
  ASSERT_TRUE (collection_->get_folder_parent (track2.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (track2.id ()).value (), folder.id ());

  // Folder should be expanded
  EXPECT_TRUE (collection_->get_track_expanded (folder.id ()));

  // Undo should restore original parents (none)
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (track1.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (track2.id ()), 3);

  EXPECT_FALSE (collection_->get_folder_parent (track1.id ()).has_value ());
  EXPECT_FALSE (collection_->get_folder_parent (track2.id ()).has_value ());
}

// Test moving tracks out of a folder removes folder parent relationships.
TEST_F (MoveTracksCommandTest, MoveOutOfFolder)
{
  auto folder = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child1, child2, other]
  collection_->add_track (folder);
  collection_->add_track (child1);
  collection_->add_track (child2);
  collection_->add_track (other);

  collection_->set_folder_parent (child1.id (), folder.id ());
  collection_->set_folder_parent (child2.id (), folder.id ());
  collection_->set_track_expanded (folder.id (), true);

  // Move child1 out of the folder to the end (pre-removal target 4)
  std::vector<structure::tracks::TrackUuidReference> tracks{ child1 };
  MoveTracksCommand command (*collection_, tracks, 4, std::nullopt);
  command.redo ();

  // Verify positions: [folder, child2, other, child1]
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (child2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 3);

  // child1 should no longer have a folder parent
  EXPECT_FALSE (collection_->get_folder_parent (child1.id ()).has_value ());
  // child2 should still belong to folder
  ASSERT_TRUE (collection_->get_folder_parent (child2.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child2.id ()).value (), folder.id ());

  // Undo should restore child1's folder parent
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 1);
  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folder.id ());
}

// Test moving tracks from one folder to another.
TEST_F (MoveTracksCommandTest, MoveBetweenFolders)
{
  auto folder1 = create_folder_track ();
  auto folder2 = create_folder_track ();
  auto child = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder1, child, folder2, other]
  collection_->add_track (folder1);
  collection_->add_track (child);
  collection_->add_track (folder2);
  collection_->add_track (other);

  collection_->set_folder_parent (child.id (), folder1.id ());
  collection_->set_track_expanded (folder1.id (), true);
  collection_->set_track_expanded (folder2.id (), true);

  // Move child from folder1 to folder2 (pre-removal target 3 = position of
  // 'other')
  std::vector<structure::tracks::TrackUuidReference> tracks{ child };
  MoveTracksCommand command (*collection_, tracks, 3, folder2.id ());
  command.redo ();

  // After detach of child (pos 1): [folder1, folder2, other]
  // Insert child at pos 2: [folder1, folder2, child, other]
  EXPECT_EQ (collection_->get_track_index (folder1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (folder2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (child.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 3);

  // child should now belong to folder2
  ASSERT_TRUE (collection_->get_folder_parent (child.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child.id ()).value (), folder2.id ());

  // Undo should restore child to folder1
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (folder1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (child.id ()), 1);
  ASSERT_TRUE (collection_->get_folder_parent (child.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child.id ()).value (), folder1.id ());
}

// Test that moving into a collapsed folder auto-expands it.
TEST_F (MoveTracksCommandTest, MoveIntoCollapsedFolderAutoExpands)
{
  auto folder = create_folder_track ();
  auto track1 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, track1, other]
  collection_->add_track (folder);
  collection_->add_track (track1);
  collection_->add_track (other);

  // Collapse the folder
  collection_->set_track_expanded (folder.id (), false);
  ASSERT_FALSE (collection_->get_track_expanded (folder.id ()));

  // Move track1 into the folder
  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 1, folder.id ());
  command.redo ();

  // Folder should now be expanded
  EXPECT_TRUE (collection_->get_track_expanded (folder.id ()));
  ASSERT_TRUE (collection_->get_folder_parent (track1.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (track1.id ()).value (), folder.id ());

  // Undo should restore collapsed state
  command.undo ();
  EXPECT_FALSE (collection_->get_track_expanded (folder.id ()));
  EXPECT_FALSE (collection_->get_folder_parent (track1.id ()).has_value ());
}

// Test circular nesting prevention: moving a folder into its own child fails
// gracefully.
TEST_F (MoveTracksCommandTest, CircularNestingPrevention)
{
  auto folder1 = create_folder_track ();
  auto folder2 = create_folder_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder1, folder2, other]
  collection_->add_track (folder1);
  collection_->add_track (folder2);
  collection_->add_track (other);

  // folder2 is a child of folder1
  collection_->set_folder_parent (folder2.id (), folder1.id ());
  collection_->set_track_expanded (folder1.id (), true);
  collection_->set_track_expanded (folder2.id (), true);

  // Try to move folder1 into folder2 (would create circular nesting)
  // Pre-removal target 3 = past-the-end in [folder1, folder2, other]
  std::vector<structure::tracks::TrackUuidReference> tracks{ folder1 };
  MoveTracksCommand command (*collection_, tracks, 3, folder2.id ());
  command.redo ();

  // folder1 should NOT be a child of folder2 (circular prevention)
  EXPECT_FALSE (collection_->get_folder_parent (folder1.id ()).has_value ());
  // folder2 should still be child of folder1
  ASSERT_TRUE (collection_->get_folder_parent (folder2.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (folder2.id ()).value (), folder1.id ());

  // Undo should restore positions (folder parents were unchanged by redo)
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (folder1.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (folder2.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 2);

  // Folder parents unchanged after undo
  EXPECT_FALSE (collection_->get_folder_parent (folder1.id ()).has_value ());
  ASSERT_TRUE (collection_->get_folder_parent (folder2.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (folder2.id ()).value (), folder1.id ());
}

// Test moving a folder with children out of a nested folder preserves
// internal parent relationships while clearing the external one.
TEST_F (MoveTracksCommandTest, MoveFolderWithChildrenOutOfNestedFolder)
{
  auto folderA = create_folder_track ();
  auto folderB = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folderA, folderB, child1, child2, other]
  collection_->add_track (folderA);
  collection_->add_track (folderB);
  collection_->add_track (child1);
  collection_->add_track (child2);
  collection_->add_track (other);

  // folderB is child of folderA
  collection_->set_folder_parent (folderB.id (), folderA.id ());
  // child1, child2 are children of folderB
  collection_->set_folder_parent (child1.id (), folderB.id ());
  collection_->set_folder_parent (child2.id (), folderB.id ());
  collection_->set_track_expanded (folderA.id (), true);
  collection_->set_track_expanded (folderB.id (), true);

  // Move folderB + children to top-level position 0 (out of folderA)
  // With target_folder=nullopt, external parents should be cleared
  std::vector<structure::tracks::TrackUuidReference> tracks{
    folderB, child1, child2
  };
  MoveTracksCommand command (*collection_, tracks, 0, std::nullopt);
  command.redo ();

  // Positions: [folderB, child1, child2, folderA, other]
  EXPECT_EQ (collection_->get_track_index (folderB.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (child2.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (folderA.id ()), 3);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 4);

  // External relationship cleared: folderB no longer child of folderA
  EXPECT_FALSE (collection_->get_folder_parent (folderB.id ()).has_value ())
    << "folderB should no longer be child of folderA";

  // Internal relationships preserved: children still belong to folderB
  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ())
    << "child1 should still belong to folderB";
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folderB.id ());
  ASSERT_TRUE (collection_->get_folder_parent (child2.id ()).has_value ())
    << "child2 should still belong to folderB";
  EXPECT_EQ (
    collection_->get_folder_parent (child2.id ()).value (), folderB.id ());

  // Undo restores everything
  command.undo ();
  EXPECT_EQ (collection_->get_track_index (folderA.id ()), 0);
  EXPECT_EQ (collection_->get_track_index (folderB.id ()), 1);
  EXPECT_EQ (collection_->get_track_index (child1.id ()), 2);
  EXPECT_EQ (collection_->get_track_index (child2.id ()), 3);
  EXPECT_EQ (collection_->get_track_index (other.id ()), 4);

  ASSERT_TRUE (collection_->get_folder_parent (folderB.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (folderB.id ()).value (), folderA.id ());
  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folderB.id ());
  ASSERT_TRUE (collection_->get_folder_parent (child2.id ()).has_value ());
  EXPECT_EQ (
    collection_->get_folder_parent (child2.id ()).value (), folderB.id ());
}

} // namespace zrythm::commands
