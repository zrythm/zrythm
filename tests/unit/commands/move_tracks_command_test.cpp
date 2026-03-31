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

  // Move track1 to position 2
  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 2);
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

  // Move track2 and track3 (adjacent) so track2 ends at position 2, track3 at 3
  std::vector<structure::tracks::TrackUuidReference> tracks{ track2, track3 };
  MoveTracksCommand command (*collection_, tracks, 2);
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
  MoveTracksCommand command (*collection_, tracks, 2);

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

  // Move track1 to last position (index 2)
  std::vector<structure::tracks::TrackUuidReference> tracks{ track1 };
  MoveTracksCommand command (*collection_, tracks, 2);
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

  // Move track2 and track4 (non-adjacent) so track2 ends at position 3, track4
  // at 4
  std::vector<structure::tracks::TrackUuidReference> tracks{ track2, track4 };
  MoveTracksCommand command (*collection_, tracks, 3);
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
  // Drop target is position 2 (top of T2, so T2 shifts down).
  // After removing selected tracks, remaining: [T0, T2, T5]
  // Insert [T1, T3, T4] at position 1 → [T0, T1, T3, T4, T2, T5]
  std::vector<structure::tracks::TrackUuidReference> tracks{
    track1, track3, track4
  };
  MoveTracksCommand command (*collection_, tracks, 1);
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

  // Move folder + children to position 1 (after 'other')
  // This effectively swaps the groups: [other, folder, child1, child2]
  std::vector<structure::tracks::TrackUuidReference> tracks{
    folder, child1, child2
  };
  MoveTracksCommand command (*collection_, tracks, 1);
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

  // Move child1 after 'other' - it leaves the folder
  std::vector<structure::tracks::TrackUuidReference> tracks{ child1 };
  MoveTracksCommand command (*collection_, tracks, 3);
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

  // child1 should still have its folder parent (even though it moved away)
  ASSERT_TRUE (collection_->get_folder_parent (child1.id ()).has_value ())
    << "child1 lost its folder parent after moving out of the folder";
  EXPECT_EQ (
    collection_->get_folder_parent (child1.id ()).value (), folder.id ());

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

} // namespace zrythm::commands
