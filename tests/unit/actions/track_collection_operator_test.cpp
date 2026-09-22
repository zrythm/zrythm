// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_collection_operator.h"
#include "controllers/clipboard.h"
#include "dsp/file_audio_source.h"
#include "plugins/faust/faust_plugin.h"
#include "plugins/juce_plugin.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_factory.h"
#include "structure/arrangement/arranger_object_factory.h"
#include "structure/project/project_registry.h"
#include "structure/tracks/audio_group_track.h"
#include "structure/tracks/folder_track.h"
#include "structure/tracks/master_track.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "undo/undo_stack.h"
#include "utils/audio.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include <QSignalSpy>
#include <QTest>

#include "helpers/in_memory_settings_backend.h"
#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_qcoreapplication.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class TrackCollectionOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (registry_);

    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      registry_,
      soloed_tracks_exist_getter_,
      {},
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] () -> structure::tracks::FinalTrackDependencies {
        return factory_deps;
      });

    // Create undo stack
    undo_stack_ = create_mock_undo_stack ();

    // Create track collection operator
    track_collection_operator_ = std::make_unique<TrackCollectionOperator> ();
    track_collection_operator_->setCollection (track_collection_.get ());
    track_collection_operator_->setUndoStack (undo_stack_.get ());
  }

  // Helper to create an audio bus track
  structure::tracks::TrackUuidReference create_audio_bus_track ()
  {
    return track_factory_
      ->create_empty_track<structure::tracks::AudioBusTrack> ();
  }

  // Helper to create a folder track
  structure::tracks::TrackUuidReference create_folder_track ()
  {
    return track_factory_->create_empty_track<structure::tracks::FolderTrack> ();
  }

  dsp::TempoMap                  tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper           tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry          registry_;
  dsp::graph_test::MockTransport transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<undo::UndoStack>                    undo_stack_;
  std::unique_ptr<TrackCollectionOperator> track_collection_operator_;
};

// ============================================================================
// Basic Tests
// ============================================================================

TEST_F (TrackCollectionOperatorTest, InitialState)
{
  EXPECT_EQ (track_collection_->track_count (), 0);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (undo_stack_->index (), 0);
}

TEST_F (TrackCollectionOperatorTest, MoveTracksWithNullCollection)
{
  track_collection_operator_->setCollection (nullptr);

  auto track = create_audio_bus_track ();
  track_collection_->add_track (track);

  QList<structure::tracks::Track *> tracks;
  tracks.append (track.get ());

  track_collection_operator_->moveTracks (tracks, 0);

  // Should not crash and no command pushed
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TrackCollectionOperatorTest, MoveTracksWithNullUndoStack)
{
  track_collection_operator_->setUndoStack (nullptr);

  auto track = create_audio_bus_track ();
  track_collection_->add_track (track);

  QList<structure::tracks::Track *> tracks;
  tracks.append (track.get ());

  track_collection_operator_->moveTracks (tracks, 0);

  // Should not crash and no command pushed
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TrackCollectionOperatorTest, MoveTracksWithEmptyList)
{
  auto track = create_audio_bus_track ();
  track_collection_->add_track (track);

  QList<structure::tracks::Track *> tracks;

  track_collection_operator_->moveTracks (tracks, 0);

  // Should not crash and no command pushed
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TrackCollectionOperatorTest, MoveTracksWithNullTrackInList)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  track_collection_->add_track (track1);
  track_collection_->add_track (track2);

  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get ());
  tracks.append (nullptr); // Add null track
  tracks.append (track2.get ());

  track_collection_operator_->moveTracks (tracks, 0);

  // Should still work, null tracks are skipped
  EXPECT_EQ (undo_stack_->count (), 1);
}

// ============================================================================
// Single Track Move Tests
// ============================================================================

TEST_F (TrackCollectionOperatorTest, MoveSingleTrackForward)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);

  // Initial order: track1, track2, track3
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 2);

  // Move track1 to the end (pre-removal target 3)
  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get ());

  track_collection_operator_->moveTracks (tracks, 3);

  // New order: track2, track3, track1
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 2);

  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (undo_stack_->canUndo ());
}

TEST_F (TrackCollectionOperatorTest, MoveSingleTrackBackward)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);

  // Move track3 to position 0
  QList<structure::tracks::Track *> tracks;
  tracks.append (track3.get ());

  track_collection_operator_->moveTracks (tracks, 0);

  // New order: track3, track1, track2
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 2);
}

TEST_F (TrackCollectionOperatorTest, MoveSingleTrackToSamePosition)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);

  // Move track2 to position 1 (where it already is)
  QList<structure::tracks::Track *> tracks;
  tracks.append (track2.get ());

  track_collection_operator_->moveTracks (tracks, 1);

  // Order should remain unchanged
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 2);
}

// ============================================================================
// Multiple Tracks Move Tests
// ============================================================================

TEST_F (TrackCollectionOperatorTest, MoveMultipleAdjacentTracks)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();
  auto track4 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);
  track_collection_->add_track (track4);

  // Move track2 and track3 after track4 (pre-removal target 4)
  QList<structure::tracks::Track *> tracks;
  tracks.append (track2.get ());
  tracks.append (track3.get ());

  track_collection_operator_->moveTracks (tracks, 4);

  // New order: track1, track4, track2, track3
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track4.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 2);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 3);
}

TEST_F (TrackCollectionOperatorTest, MoveNonAdjacentTracks)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();
  auto track4 = create_audio_bus_track ();
  auto track5 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);
  track_collection_->add_track (track4);
  track_collection_->add_track (track5);

  // Move track2 and track4 after track5 (pre-removal target 5)
  QList<structure::tracks::Track *> tracks;
  tracks.append (track2.get ());
  tracks.append (track4.get ());

  track_collection_operator_->moveTracks (tracks, 5);

  // New order: track1, track3, track5, track2, track4
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track5.id ()), 2);
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 3);
  EXPECT_EQ (track_collection_->get_track_index (track4.id ()), 4);
}

// ============================================================================
// Undo/Redo Tests
// ============================================================================

TEST_F (TrackCollectionOperatorTest, UndoAfterMove)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);

  // Move track1 to the end (pre-removal target 3)
  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get ());

  track_collection_operator_->moveTracks (tracks, 3);

  // Verify move happened
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 2);

  // Undo
  undo_stack_->undo ();

  // Should restore original order
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 2);

  EXPECT_TRUE (undo_stack_->canRedo ());
}

TEST_F (TrackCollectionOperatorTest, RedoAfterUndo)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);

  // Move track1 to the end (pre-removal target 3)
  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get ());

  track_collection_operator_->moveTracks (tracks, 3);
  undo_stack_->undo ();
  undo_stack_->redo ();

  // Should be back to moved state
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 2);

  EXPECT_TRUE (undo_stack_->canUndo ());
  EXPECT_FALSE (undo_stack_->canRedo ());
}

TEST_F (TrackCollectionOperatorTest, MultipleUndoRedoCycles)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);

  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get ());

  track_collection_operator_->moveTracks (tracks, 3);

  // First cycle
  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);

  undo_stack_->redo ();
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 2);

  // Second cycle
  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);

  undo_stack_->redo ();
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 2);
}

TEST_F (TrackCollectionOperatorTest, CommandTextInUndoStack)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);

  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get ());

  track_collection_operator_->moveTracks (tracks, 1);

  EXPECT_EQ (undo_stack_->text (0), QString ("Move Tracks"));
}

TEST_F (TrackCollectionOperatorTest, UndoMultipleTracksMove)
{
  auto track1 = create_audio_bus_track ();
  auto track2 = create_audio_bus_track ();
  auto track3 = create_audio_bus_track ();
  auto track4 = create_audio_bus_track ();

  track_collection_->add_track (track1);
  track_collection_->add_track (track2);
  track_collection_->add_track (track3);
  track_collection_->add_track (track4);

  // Move track2 and track3 (pre-removal target 4)
  QList<structure::tracks::Track *> tracks;
  tracks.append (track2.get ());
  tracks.append (track3.get ());

  track_collection_operator_->moveTracks (tracks, 4);

  // Verify move happened
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 2);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 3);

  // Undo
  undo_stack_->undo ();

  // Should restore original order
  EXPECT_EQ (track_collection_->get_track_index (track1.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track2.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track3.id ()), 2);
  EXPECT_EQ (track_collection_->get_track_index (track4.id ()), 3);
}

// ============================================================================
// Folder-Aware Move Tests
// ============================================================================

// Moving a folder track should automatically include all its descendants.
TEST_F (TrackCollectionOperatorTest, MoveFolderAutoExpandsDescendants)
{
  auto folder = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child1, child2, other]
  track_collection_->add_track (folder);
  track_collection_->add_track (child1);
  track_collection_->add_track (child2);
  track_collection_->add_track (other);

  track_collection_->set_folder_parent (child1.id (), folder.id ());
  track_collection_->set_folder_parent (child2.id (), folder.id ());
  track_collection_->set_track_expanded (folder.id (), true);

  // Pass only the folder to moveTracks - operator should auto-expand to
  // include child1 and child2.
  QList<structure::tracks::Track *> tracks;
  tracks.append (folder.get ());

  // Pre-removal target 4 = past-the-end of [folder, child1, child2, other]
  track_collection_operator_->moveTracks (tracks, 4);

  // Expected: [other, folder, child1, child2]
  EXPECT_EQ (track_collection_->get_track_index (other.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (folder.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (child1.id ()), 2);
  EXPECT_EQ (track_collection_->get_track_index (child2.id ()), 3);

  // Folder parent relationships preserved (internal to moved set)
  ASSERT_TRUE (track_collection_->get_folder_parent (child1.id ()).has_value ());
  EXPECT_EQ (
    track_collection_->get_folder_parent (child1.id ()).value (), folder.id ());
  ASSERT_TRUE (track_collection_->get_folder_parent (child2.id ()).has_value ());
  EXPECT_EQ (
    track_collection_->get_folder_parent (child2.id ()).value (), folder.id ());
}

// Dropping a track inside an expanded folder (between its children) should
// auto-detect the enclosing folder via get_enclosing_folder.
TEST_F (TrackCollectionOperatorTest, MoveIntoFolderByEnclosingFolder)
{
  auto folder = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto track = create_audio_bus_track ();

  // Layout: [folder, child1, child2, track]
  track_collection_->add_track (folder);
  track_collection_->add_track (child1);
  track_collection_->add_track (child2);
  track_collection_->add_track (track);

  track_collection_->set_folder_parent (child1.id (), folder.id ());
  track_collection_->set_folder_parent (child2.id (), folder.id ());
  track_collection_->set_track_expanded (folder.id (), true);

  // Move 'track' to pre-removal position 2 (inside folder's child range).
  // get_enclosing_folder(2) should detect folder since its last_child_index
  // (0+2=2) covers position 2.
  QList<structure::tracks::Track *> tracks;
  tracks.append (track.get ());

  track_collection_operator_->moveTracks (tracks, 2);

  // Expected: [folder, child1, track, child2] (track inserted inside folder)
  EXPECT_EQ (track_collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (child1.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (track.id ()), 2);
  EXPECT_EQ (track_collection_->get_track_index (child2.id ()), 3);

  // track should now be a child of folder
  ASSERT_TRUE (track_collection_->get_folder_parent (track.id ()).has_value ());
  EXPECT_EQ (
    track_collection_->get_folder_parent (track.id ()).value (), folder.id ());
}

// Passing an explicit folder as targetFolder should set folder parent.
TEST_F (TrackCollectionOperatorTest, MoveIntoFolderExplicitTarget)
{
  auto folder = create_folder_track ();
  auto other = create_audio_bus_track ();
  auto track = create_audio_bus_track ();

  // Layout: [folder, other, track]
  track_collection_->add_track (folder);
  track_collection_->add_track (other);
  track_collection_->add_track (track);

  track_collection_->set_track_expanded (folder.id (), true);

  // Move 'track' right after folder, explicitly targeting the folder.
  QList<structure::tracks::Track *> tracks;
  tracks.append (track.get ());

  track_collection_operator_->moveTracks (tracks, 1, folder.get ());

  // Expected: [folder, track, other]
  EXPECT_EQ (track_collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (track.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (other.id ()), 2);

  // track should be a child of folder
  ASSERT_TRUE (track_collection_->get_folder_parent (track.id ()).has_value ());
  EXPECT_EQ (
    track_collection_->get_folder_parent (track.id ()).value (), folder.id ());
}

// Moving a track out of a folder (past its child range) should clear the
// folder parent.
TEST_F (TrackCollectionOperatorTest, MoveOutOfFolder)
{
  auto folder = create_folder_track ();
  auto child = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child, other]
  track_collection_->add_track (folder);
  track_collection_->add_track (child);
  track_collection_->add_track (other);

  track_collection_->set_folder_parent (child.id (), folder.id ());
  track_collection_->set_track_expanded (folder.id (), true);

  // Move child past-the-end (pre-removal target 3).
  // get_enclosing_folder(3) returns nullopt because folder's last_child_index
  // (1) < 3.
  QList<structure::tracks::Track *> tracks;
  tracks.append (child.get ());

  track_collection_operator_->moveTracks (tracks, 3);

  // Expected: [folder, other, child]
  EXPECT_EQ (track_collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (other.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (child.id ()), 2);

  // child should no longer have a folder parent
  EXPECT_FALSE (track_collection_->get_folder_parent (child.id ()).has_value ());
}

// Undo after moving a folder with auto-expanded descendants should restore
// original positions and folder parent relationships.
TEST_F (TrackCollectionOperatorTest, UndoFolderMoveWithDescendants)
{
  auto folder = create_folder_track ();
  auto child1 = create_audio_bus_track ();
  auto child2 = create_audio_bus_track ();
  auto other = create_audio_bus_track ();

  // Layout: [folder, child1, child2, other]
  track_collection_->add_track (folder);
  track_collection_->add_track (child1);
  track_collection_->add_track (child2);
  track_collection_->add_track (other);

  track_collection_->set_folder_parent (child1.id (), folder.id ());
  track_collection_->set_folder_parent (child2.id (), folder.id ());
  track_collection_->set_track_expanded (folder.id (), true);

  QList<structure::tracks::Track *> tracks;
  tracks.append (folder.get ());

  track_collection_operator_->moveTracks (tracks, 4);
  undo_stack_->undo ();

  // Should restore original layout
  EXPECT_EQ (track_collection_->get_track_index (folder.id ()), 0);
  EXPECT_EQ (track_collection_->get_track_index (child1.id ()), 1);
  EXPECT_EQ (track_collection_->get_track_index (child2.id ()), 2);
  EXPECT_EQ (track_collection_->get_track_index (other.id ()), 3);

  // Folder parents restored
  ASSERT_TRUE (track_collection_->get_folder_parent (child1.id ()).has_value ());
  EXPECT_EQ (
    track_collection_->get_folder_parent (child1.id ()).value (), folder.id ());
  ASSERT_TRUE (track_collection_->get_folder_parent (child2.id ()).has_value ());
  EXPECT_EQ (
    track_collection_->get_folder_parent (child2.id ()).value (), folder.id ());
}

// ============================================================================
// Delete Tests
// ============================================================================

TEST_F (TrackCollectionOperatorTest, DeleteUndeletableTrackThrows)
{
  // The chord track is one of the non-deletable types
  auto chord_track =
    track_factory_->create_empty_track<structure::tracks::ChordTrack> ();
  track_collection_->add_track (chord_track);

  QList<structure::tracks::Track *> tracks;
  tracks.append (chord_track.get ());

  EXPECT_THROW (
    track_collection_operator_->deleteTracks (tracks), std::invalid_argument);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TrackCollectionOperatorTest, DeleteMixedDeletableUndeletableThrows)
{
  auto audio = create_audio_bus_track ();
  auto chord_track =
    track_factory_->create_empty_track<structure::tracks::ChordTrack> ();

  track_collection_->add_track (audio);
  track_collection_->add_track (chord_track);

  QList<structure::tracks::Track *> tracks;
  tracks.append (audio.get ());
  tracks.append (chord_track.get ());

  EXPECT_THROW (
    track_collection_operator_->deleteTracks (tracks), std::invalid_argument);
  EXPECT_EQ (undo_stack_->count (), 0);
  // Audio track should still be present (no partial delete)
  EXPECT_EQ (track_collection_->track_count (), 2);
}

TEST_F (TrackCollectionOperatorTest, DeleteWithEmptyList)
{
  auto track = create_audio_bus_track ();
  track_collection_->add_track (track);

  QList<structure::tracks::Track *> tracks;

  track_collection_operator_->deleteTracks (tracks);

  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (track_collection_->track_count (), 1);
}

TEST_F (TrackCollectionOperatorTest, DeleteWithNullCollection)
{
  track_collection_operator_->setCollection (nullptr);

  auto track = create_audio_bus_track ();
  track_collection_->add_track (track);

  QList<structure::tracks::Track *> tracks;
  tracks.append (track.get ());

  // Should not crash and no command pushed
  track_collection_operator_->deleteTracks (tracks);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TrackCollectionOperatorTest, DeleteWithNullUndoStack)
{
  track_collection_operator_->setUndoStack (nullptr);

  auto track = create_audio_bus_track ();
  track_collection_->add_track (track);

  QList<structure::tracks::Track *> tracks;
  tracks.append (track.get ());

  // Should not crash and no command pushed
  track_collection_operator_->deleteTracks (tracks);
  EXPECT_EQ (track_collection_->track_count (), 1);
}

TEST_F (TrackCollectionOperatorTest, DeleteLaneWithNullLane)
{
  track_collection_operator_->deleteLane (nullptr);
  EXPECT_EQ (undo_stack_->count (), 0);
}

// A track keeps at least one lane: deleting the only lane is refused
TEST_F (TrackCollectionOperatorTest, DeleteLaneRefusesLastLane)
{
  auto audio_track_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  track_collection_->add_track (audio_track_ref);
  auto * audio_track =
    audio_track_ref.get_object_as<structure::tracks::AudioTrack> ();
  auto * lanes = audio_track->lanes ();
  for (
    const auto idx :
    std::views::iota (size_t{ 1 }, lanes->size ()) | std::views::reverse)
    lanes->removeLane (idx);
  auto * lane = lanes->getFirstLane ();

  track_collection_operator_->deleteLane (lane);
  EXPECT_EQ (undo_stack_->count (), 0);
  EXPECT_EQ (lanes->size (), 1);
}

TEST_F (TrackCollectionOperatorTest, DeleteLaneRemovesLane)
{
  auto audio_track_ref =
    track_factory_->create_empty_track<structure::tracks::AudioTrack> ();
  track_collection_->add_track (audio_track_ref);
  auto * audio_track =
    audio_track_ref.get_object_as<structure::tracks::AudioTrack> ();
  auto * lanes = audio_track->lanes ();
  for (
    const auto idx :
    std::views::iota (size_t{ 1 }, lanes->size ()) | std::views::reverse)
    lanes->removeLane (idx);
  auto * lane_2 = lanes->addLane ();

  track_collection_operator_->deleteLane (lane_2);
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_EQ (lanes->size (), 1);

  undo_stack_->undo ();
  EXPECT_EQ (lanes->size (), 2);
  EXPECT_EQ (lanes->at (1), lane_2);
}

// ============================================================================
// Clipboard Tests
// ============================================================================

class TrackCollectionOperatorClipboardTest
    : public ::testing::Test,
      private test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (registry_);

    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      registry_,
      structure::tracks::SoloedTracksExistGetter{ [] { return false; } },
      {},
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] () -> structure::tracks::FinalTrackDependencies {
        return factory_deps;
      });

    arranger_object_factory_ = std::make_unique<
      structure::arrangement::ArrangerObjectFactory> (
      structure::arrangement::ArrangerObjectFactory::Dependencies{
        .tempo_map_ = tempo_map_wrapper_,
        .registry_ = registry_,
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; },
      },
      [] () { return units::sample_rate (44100); },
      [] () { return units::bpm (120.0); });

    plugin_factory_ = std::make_unique<
      plugins::PluginFactory> (plugins::PluginFactory::CommonFactoryDependencies{
      .registry = registry_,
      .create_plugin_instance_async_func_ =
        [] (
          const juce::PluginDescription &, double, int,
          juce::AudioPluginFormat::PluginCreationCallback callback) {
          callback (nullptr, "No plugin in operator tests");
        },
      .sample_rate_provider_ = [] () { return units::sample_rate (44100); },
      .buffer_size_provider_ = [] () { return units::samples (256u); },
      .top_level_window_provider_ =
        test_helpers::make_mock_plugin_host_window_factory (
          std::make_shared<test_helpers::MockPluginHostWindowState> ()),
      .main_thread_dispatcher_ = main_dispatcher_ });

    registry_.set_deserialization_dependencies (
      { *track_factory_, *arranger_object_factory_, *plugin_factory_ });

    master_track_ =
      track_factory_->create_empty_track<structure::tracks::MasterTrack> ();
    singleton_tracks_.setMasterTrack (
      dynamic_cast<structure::tracks::MasterTrack *> (master_track_->get ()));

    undo_stack_ = create_mock_undo_stack ();
    track_collection_operator_ = std::make_unique<TrackCollectionOperator> (
      *undo_stack_, registry_, clipboard_, *track_collection_, track_routing_,
      singleton_tracks_, [] () { return QString ("test-project-id"); });
  }

  structure::tracks::TrackUuidReference create_audio_bus_track ()
  {
    return track_factory_
      ->create_empty_track<structure::tracks::AudioBusTrack> ();
  }

  structure::tracks::TrackUuidReference create_audio_group_track ()
  {
    return track_factory_
      ->create_empty_track<structure::tracks::AudioGroupTrack> ();
  }

  structure::tracks::TrackUuidReference create_folder_track ()
  {
    return track_factory_->create_empty_track<structure::tracks::FolderTrack> ();
  }

  /** Creates a track, names it and adds it to the collection. */
  structure::tracks::TrackUuidReference add_named_track (std::string_view name)
  {
    auto ref = create_audio_bus_track ();
    ref.get ()->setName (
      utils::Utf8String::from_utf8_encoded_string (name).to_qstring ());
    track_collection_->add_track (ref);
    return ref;
  }

  QList<structure::tracks::Track *>
  track_list (std::initializer_list<structure::tracks::TrackUuidReference> refs)
  {
    QList<structure::tracks::Track *> tracks;
    for (const auto &ref : refs)
      tracks.append (ref.get ());
    return tracks;
  }

  std::string track_name_at (size_t index)
  {
    return track_collection_->get_track_at_index (index)->get_name ().str ();
  }

  // Declared before the operators so destruction (reverse order) destroys
  // them while the registry and clipboard they reference still exist
  structure::project::ProjectRegistry registry_;
  controllers::Clipboard              clipboard_{
    [] () { return QString (); }, [] (const QString &) { }
  };
  structure::tracks::TrackRouting    track_routing_{ registry_ };
  structure::tracks::SingletonTracks singleton_tracks_;

  dsp::TempoMap        tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper tempo_map_wrapper_{ tempo_map_ };

  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>
                                          arranger_object_factory_;
  std::unique_ptr<plugins::PluginFactory> plugin_factory_;
  QObject                                 dispatcher_context_;
  utils::MainThreadClosureDispatcher      main_dispatcher_{
    dispatcher_context_, std::chrono::milliseconds{ 10 }
  };
  std::optional<structure::tracks::TrackUuidReference> master_track_;

  std::unique_ptr<undo::UndoStack>         undo_stack_;
  std::unique_ptr<TrackCollectionOperator> track_collection_operator_;
};

TEST_F (TrackCollectionOperatorClipboardTest, CopyPasteRoundTrip)
{
  auto track_a = add_named_track ("A");
  auto track_b = add_named_track ("B");

  EXPECT_TRUE (
    track_collection_operator_->copyTracks (track_list ({ track_a, track_b })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 2);
  EXPECT_EQ (track_collection_->track_count (), 4);
  EXPECT_EQ (track_name_at (0), "A");
  EXPECT_EQ (track_name_at (1), "B");
  // Pasted tracks are named uniquely against the collection
  EXPECT_EQ (track_name_at (2), "A 1");
  EXPECT_EQ (track_name_at (3), "B 1");

  // The pasted tracks are new objects with new UUID strings
  EXPECT_FALSE (pasted_ids.contains (
    type_safe::get (track_a.id ()).toString (QUuid::WithoutBraces)));

  EXPECT_EQ (undo_stack_->count (), 1);
  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), 2);
  undo_stack_->redo ();
  EXPECT_EQ (track_collection_->track_count (), 4);
}

TEST_F (TrackCollectionOperatorClipboardTest, PasteAtTargetPosition)
{
  auto track_a = add_named_track ("A");
  add_named_track ("B");

  EXPECT_TRUE (
    track_collection_operator_->copyTracks (track_list ({ track_a })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (1);
  ASSERT_EQ (pasted_ids.size (), 1);
  EXPECT_EQ (track_collection_->track_count (), 3);
  EXPECT_EQ (track_name_at (0), "A");
  EXPECT_EQ (track_name_at (1), "A 1");
  EXPECT_EQ (track_name_at (2), "B");
}

TEST_F (TrackCollectionOperatorClipboardTest, CopyRefusesNonCopyableSelection)
{
  auto audio = add_named_track ("A");
  auto chord_track =
    track_factory_->create_empty_track<structure::tracks::ChordTrack> ();
  track_collection_->add_track (chord_track);

  QSignalSpy refused_spy (
    track_collection_operator_.get (),
    &TrackCollectionOperator::operationRefused);

  EXPECT_FALSE (track_collection_operator_->copyTracks (
    track_list ({ audio, chord_track })));
  EXPECT_EQ (refused_spy.count (), 1);
  EXPECT_FALSE (clipboard_.hasTracks ());
}

TEST_F (TrackCollectionOperatorClipboardTest, CutRemovesInOneUndoStep)
{
  auto track_a = add_named_track ("A");
  add_named_track ("B");

  EXPECT_TRUE (track_collection_operator_->cutTracks (track_list ({ track_a })));

  EXPECT_EQ (track_collection_->track_count (), 1);
  EXPECT_EQ (track_name_at (0), "B");
  EXPECT_EQ (undo_stack_->count (), 1);
  EXPECT_TRUE (clipboard_.hasTracks ());

  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), 2);
}

TEST_F (TrackCollectionOperatorClipboardTest, CutThenPaste)
{
  auto track_a = add_named_track ("A");

  EXPECT_TRUE (track_collection_operator_->cutTracks (track_list ({ track_a })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 1);
  EXPECT_EQ (track_collection_->track_count (), 1);
  // The cut removed the original, so the pasted track keeps its name
  EXPECT_EQ (track_name_at (0), "A");
}

TEST_F (TrackCollectionOperatorClipboardTest, DuplicateKeepsClipboard)
{
  auto track_a = add_named_track ("A");
  auto track_b = add_named_track ("B");
  add_named_track ("C");

  EXPECT_TRUE (
    track_collection_operator_->copyTracks (track_list ({ track_a })));

  const auto duplicated_ids =
    track_collection_operator_->duplicateTracks (track_list ({ track_b }));
  ASSERT_EQ (duplicated_ids.size (), 1);
  // Duplicates insert right after their source
  EXPECT_EQ (track_collection_->track_count (), 4);
  EXPECT_EQ (track_name_at (0), "A");
  EXPECT_EQ (track_name_at (1), "B");
  EXPECT_EQ (track_name_at (2), "B 1");
  EXPECT_EQ (track_name_at (3), "C");

  // Duplicating did not touch the clipboard: pasting still yields A
  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 1);
  EXPECT_EQ (track_name_at (4), "A 1");
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  DuplicateFolderPlacesCopyAfterOriginals)
{
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  const auto child_names = { "C1", "C2" };
  for (const auto name : child_names)
    {
      auto child = create_audio_bus_track ();
      child.get ()->setName (name);
      track_collection_->add_track (child);
      track_collection_->set_folder_parent (child.id (), folder.id ());
    }
  add_named_track ("X");

  // Only the folder is selected: the duplicate must land after the
  // folder's children, not inside the folder
  const auto duplicated_ids =
    track_collection_operator_->duplicateTracks (track_list ({ folder }));
  ASSERT_EQ (duplicated_ids.size (), 3);
  EXPECT_EQ (track_collection_->track_count (), 7);

  EXPECT_EQ (track_name_at (0), "F");
  EXPECT_EQ (track_name_at (1), "C1");
  EXPECT_EQ (track_name_at (2), "C2");
  EXPECT_EQ (track_name_at (3), "F 1");
  EXPECT_EQ (track_name_at (4), "C1 1");
  EXPECT_EQ (track_name_at (5), "C2 1");
  EXPECT_EQ (track_name_at (6), "X");

  // The duplicated folder stays top-level and keeps its children
  EXPECT_FALSE (
    track_collection_
      ->get_folder_parent (
        structure::tracks::Track::Uuid (
          QUuid::fromString (duplicated_ids.at (0).toString ())))
      .has_value ());
  const auto duplicated_child_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (1).toString ())));
  ASSERT_TRUE (duplicated_child_parent.has_value ());
  EXPECT_EQ (
    duplicated_child_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  DuplicateFolderInsideFolderStaysInside)
{
  auto outer = create_folder_track ();
  outer.get ()->setName ("G");
  track_collection_->add_track (outer);
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  auto child = create_audio_bus_track ();
  child.get ()->setName ("C1");
  track_collection_->add_track (child);
  track_collection_->set_folder_parent (folder.id (), outer.id ());
  track_collection_->set_folder_parent (child.id (), folder.id ());
  add_named_track ("X");

  const auto duplicated_ids =
    track_collection_operator_->duplicateTracks (track_list ({ folder }));
  ASSERT_EQ (duplicated_ids.size (), 2);
  EXPECT_EQ (track_collection_->track_count (), 6);

  EXPECT_EQ (track_name_at (0), "G");
  EXPECT_EQ (track_name_at (1), "F");
  EXPECT_EQ (track_name_at (2), "C1");
  // The duplicates land after the folder's children, still inside the
  // outer folder
  EXPECT_EQ (track_name_at (3), "F 1");
  EXPECT_EQ (track_name_at (4), "C1 1");
  EXPECT_EQ (track_name_at (5), "X");

  const auto duplicated_folder_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
  ASSERT_TRUE (duplicated_folder_parent.has_value ());
  EXPECT_EQ (duplicated_folder_parent.value (), outer.id ());
  const auto duplicated_child_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (1).toString ())));
  ASSERT_TRUE (duplicated_child_parent.has_value ());
  EXPECT_EQ (
    duplicated_child_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  DuplicateFolderAtCollectionEndInsideFolderKeepsFolder)
{
  auto outer = create_folder_track ();
  outer.get ()->setName ("G");
  track_collection_->add_track (outer);
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  auto child = create_audio_bus_track ();
  child.get ()->setName ("C1");
  track_collection_->add_track (child);
  track_collection_->set_folder_parent (folder.id (), outer.id ());
  track_collection_->set_folder_parent (child.id (), folder.id ());

  // The duplicated folder ends at the collection's end: the duplicates
  // append in place and must still become children of the outer folder
  const auto duplicated_ids =
    track_collection_operator_->duplicateTracks (track_list ({ folder }));
  ASSERT_EQ (duplicated_ids.size (), 2);
  EXPECT_EQ (track_collection_->track_count (), 5);
  EXPECT_EQ (track_name_at (0), "G");
  EXPECT_EQ (track_name_at (1), "F");
  EXPECT_EQ (track_name_at (2), "C1");
  EXPECT_EQ (track_name_at (3), "F 1");
  EXPECT_EQ (track_name_at (4), "C1 1");

  const auto end_duplicated_folder_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
  ASSERT_TRUE (end_duplicated_folder_parent.has_value ());
  EXPECT_EQ (end_duplicated_folder_parent.value (), outer.id ());
  const auto end_duplicated_child_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (1).toString ())));
  ASSERT_TRUE (end_duplicated_child_parent.has_value ());
  EXPECT_EQ (
    end_duplicated_child_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  DuplicateMixedSelectionNestsByDropPosition)
{
  // X sits at the top level, F inside G, and G keeps a child after F's
  // subtree, so the position after the duplicated sources is inside G
  auto loose = add_named_track ("X");
  auto outer = create_folder_track ();
  outer.get ()->setName ("G");
  track_collection_->add_track (outer);
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  auto child = create_audio_bus_track ();
  child.get ()->setName ("C1");
  track_collection_->add_track (child);
  auto sibling = create_audio_bus_track ();
  sibling.get ()->setName ("Z");
  track_collection_->add_track (sibling);
  track_collection_->set_folder_parent (folder.id (), outer.id ());
  track_collection_->set_folder_parent (child.id (), folder.id ());
  track_collection_->set_folder_parent (sibling.id (), outer.id ());

  // The copied roots do not share a folder, so the duplicates land by
  // their position, like a drag of the same selection to just after it:
  // that position is inside G, so the whole block nests there
  const auto duplicated_ids = track_collection_operator_->duplicateTracks (
    track_list ({ loose, folder }));
  ASSERT_EQ (duplicated_ids.size (), 3);
  EXPECT_EQ (track_collection_->track_count (), 8);

  EXPECT_EQ (track_name_at (0), "X");
  EXPECT_EQ (track_name_at (1), "G");
  EXPECT_EQ (track_name_at (2), "F");
  EXPECT_EQ (track_name_at (3), "C1");
  EXPECT_EQ (track_name_at (4), "X 1");
  EXPECT_EQ (track_name_at (5), "F 1");
  EXPECT_EQ (track_name_at (6), "C1 1");
  EXPECT_EQ (track_name_at (7), "Z");

  const auto duplicated_loose_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
  ASSERT_TRUE (duplicated_loose_parent.has_value ());
  EXPECT_EQ (duplicated_loose_parent.value (), outer.id ());
  const auto duplicated_folder_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (1).toString ())));
  EXPECT_EQ (duplicated_folder_parent.value (), outer.id ());
  const auto duplicated_child_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (2).toString ())));
  ASSERT_TRUE (duplicated_child_parent.has_value ());
  EXPECT_EQ (
    duplicated_child_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (1).toString ())));
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  DuplicateMixedSelectionKeepsNoFolderAtCollectionEnd)
{
  // F sits inside G at the start of the collection, the loose track X
  // at the end; the position after the sources is the collection's end
  // and outside every folder
  auto outer = create_folder_track ();
  outer.get ()->setName ("G");
  track_collection_->add_track (outer);
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  auto child = create_audio_bus_track ();
  child.get ()->setName ("C1");
  track_collection_->add_track (child);
  auto loose = add_named_track ("X");
  track_collection_->set_folder_parent (folder.id (), outer.id ());
  track_collection_->set_folder_parent (child.id (), folder.id ());

  // The copied roots do not share a folder: the duplicates append at
  // the end and stay at the top level, like a drag of the same
  // selection to the collection's end
  const auto duplicated_ids = track_collection_operator_->duplicateTracks (
    track_list ({ folder, loose }));
  ASSERT_EQ (duplicated_ids.size (), 3);
  EXPECT_EQ (track_collection_->track_count (), 7);

  EXPECT_EQ (track_name_at (0), "G");
  EXPECT_EQ (track_name_at (1), "F");
  EXPECT_EQ (track_name_at (2), "C1");
  EXPECT_EQ (track_name_at (3), "X");
  EXPECT_EQ (track_name_at (4), "F 1");
  EXPECT_EQ (track_name_at (5), "C1 1");
  EXPECT_EQ (track_name_at (6), "X 1");

  const auto duplicated_folder_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
  EXPECT_FALSE (duplicated_folder_parent.has_value ());
  const auto duplicated_child_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (1).toString ())));
  ASSERT_TRUE (duplicated_child_parent.has_value ());
  EXPECT_EQ (
    duplicated_child_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (0).toString ())));
  const auto duplicated_loose_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (duplicated_ids.at (2).toString ())));
  EXPECT_FALSE (duplicated_loose_parent.has_value ());
}

TEST_F (TrackCollectionOperatorClipboardTest, InSetRoutingRemappedOnPaste)
{
  auto group = create_audio_group_track ();
  group.get ()->setName ("G");
  track_collection_->add_track (group);
  auto source = add_named_track ("A");
  track_routing_.add_or_replace_route (source.id (), group.id ());

  EXPECT_TRUE (
    track_collection_operator_->copyTracks (track_list ({ source, group })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 2);

  // The payload is canonicalized to collection order, so the pasted
  // group (first in the collection) is the first root and the pasted
  // source is the second
  const auto pasted_source_id =
    QUuid::fromString (pasted_ids.at (1).toString ());
  const auto pasted_output = track_routing_.get_output_track (
    structure::tracks::Track::Uuid (pasted_source_id));
  ASSERT_TRUE (pasted_output.has_value ());
  // The pasted source routes to the pasted group (first root), not the
  // original
  EXPECT_EQ (
    type_safe::get (pasted_output->id ()).toString (QUuid::WithoutBraces),
    pasted_ids.at (0).toString ());
}

TEST_F (TrackCollectionOperatorClipboardTest, OutOfSetRoutingKeptInSameProject)
{
  auto group = create_audio_group_track ();
  group.get ()->setName ("G");
  track_collection_->add_track (group);
  auto source = add_named_track ("A");
  track_routing_.add_or_replace_route (source.id (), group.id ());

  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ source })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 1);

  const auto pasted_source_id =
    QUuid::fromString (pasted_ids.at (0).toString ());
  const auto pasted_output = track_routing_.get_output_track (
    structure::tracks::Track::Uuid (pasted_source_id));
  ASSERT_TRUE (pasted_output.has_value ());
  // The pasted source keeps the route to the existing group
  EXPECT_EQ (pasted_output->id (), group.id ());
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  UnresolvableRoutingTargetDefaultsAudioToMaster)
{
  auto source = add_named_track ("A");

  // A routing entry whose target resolves nowhere (the cross-project
  // case) is severed by the paste filter; audio tracks then route to
  // the master track
  nlohmann::json routing_entry = nlohmann::json::array (
    {
      nlohmann::json{
                     { std::string (
            structure::project::ClipboardPayload::kRoutingSourceMetadataKey),
          type_safe::get (source.id ())
            .toString (QUuid::WithoutBraces)
            .toStdString () },
                     { std::string (
            structure::project::ClipboardPayload::kRoutingTargetMetadataKey),
          QUuid::createUuid ().toString (QUuid::WithoutBraces).toStdString () } }
  });
  nlohmann::json metadata = nlohmann::json::object (
    {
      { std::string (structure::project::ClipboardPayload::kRoutingMetadataKey),
       std::move (routing_entry) }
  });

  auto payload = structure::project::ClipboardPayload::create (
    registry_, structure::project::ClipboardPayload::Type::Tracks,
    { type_safe::get (source.id ()) }, QString ("other-project-id"),
    std::move (metadata));
  clipboard_.setPayload (std::move (payload));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 1);

  const auto pasted_source_id =
    QUuid::fromString (pasted_ids.at (0).toString ());
  const auto pasted_output = track_routing_.get_output_track (
    structure::tracks::Track::Uuid (pasted_source_id));
  ASSERT_TRUE (pasted_output.has_value ());
  EXPECT_EQ (pasted_output->id (), master_track_->id ());
}

TEST_F (TrackCollectionOperatorClipboardTest, FolderCopyIncludesDescendants)
{
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  auto child1 = create_audio_bus_track ();
  child1.get ()->setName ("C1");
  auto child2 = create_audio_bus_track ();
  child2.get ()->setName ("C2");
  track_collection_->add_track (child1);
  track_collection_->add_track (child2);
  track_collection_->set_folder_parent (child1.id (), folder.id ());
  track_collection_->set_folder_parent (child2.id (), folder.id ());
  track_collection_->set_track_expanded (folder.id (), true);
  add_named_track ("Other");

  // Copying only the folder pulls its descendants into the payload
  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ folder })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 3);
  EXPECT_EQ (track_collection_->track_count (), 7);

  // The pasted folder nesting is restored: the pasted children point at
  // the pasted folder, not the original
  const auto pasted_folder_id =
    QUuid::fromString (pasted_ids.at (0).toString ());
  const auto pasted_child1_id =
    QUuid::fromString (pasted_ids.at (1).toString ());
  const auto pasted_child1_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (pasted_child1_id));
  ASSERT_TRUE (pasted_child1_parent.has_value ());
  EXPECT_EQ (
    pasted_child1_parent.value (),
    structure::tracks::Track::Uuid (pasted_folder_id));
}

TEST_F (TrackCollectionOperatorClipboardTest, TypeMismatchRefusesPaste)
{
  add_named_track ("A");

  auto payload = structure::project::ClipboardPayload::create (
    registry_, structure::project::ClipboardPayload::Type::ArrangerObjects, {},
    QString ("test-project-id"));
  clipboard_.setPayload (std::move (payload));

  QSignalSpy refused_spy (
    track_collection_operator_.get (),
    &TrackCollectionOperator::operationRefused);

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  EXPECT_TRUE (pasted_ids.isEmpty ());
  EXPECT_EQ (refused_spy.count (), 1);
  EXPECT_EQ (track_collection_->track_count (), 1);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TrackCollectionOperatorClipboardTest, CanPasteTracksReactivity)
{
  EXPECT_FALSE (track_collection_operator_->canPasteTracks ());

  QSignalSpy changed_spy (
    track_collection_operator_.get (),
    &TrackCollectionOperator::canPasteTracksChanged);

  auto track_a = add_named_track ("A");
  EXPECT_TRUE (
    track_collection_operator_->copyTracks (track_list ({ track_a })));

  EXPECT_TRUE (track_collection_operator_->canPasteTracks ());
  EXPECT_GE (changed_spy.count (), 1);
}

TEST_F (TrackCollectionOperatorClipboardTest, FolderPastePreservesSiblingOrder)
{
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  const auto child_names = { "C1", "C2", "C3" };
  for (const auto name : child_names)
    {
      auto child = create_audio_bus_track ();
      child.get ()->setName (name);
      track_collection_->add_track (child);
      track_collection_->set_folder_parent (child.id (), folder.id ());
    }

  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ folder })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 4);
  EXPECT_EQ (track_collection_->track_count (), 8);

  // The pasted children keep their copied order directly after the
  // pasted folder
  EXPECT_EQ (track_name_at (4), "F 1");
  EXPECT_EQ (track_name_at (5), "C1 1");
  EXPECT_EQ (track_name_at (6), "C2 1");
  EXPECT_EQ (track_name_at (7), "C3 1");
}

TEST_F (TrackCollectionOperatorClipboardTest, NestedFolderPasteRestoresNesting)
{
  auto outer = create_folder_track ();
  outer.get ()->setName ("F");
  auto inner = create_folder_track ();
  inner.get ()->setName ("S");
  auto leaf = create_audio_bus_track ();
  leaf.get ()->setName ("T");
  track_collection_->add_track (outer);
  track_collection_->add_track (inner);
  track_collection_->add_track (leaf);
  track_collection_->set_folder_parent (inner.id (), outer.id ());
  track_collection_->set_folder_parent (leaf.id (), inner.id ());

  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ outer })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 3);
  EXPECT_EQ (track_collection_->track_count (), 6);

  const auto pasted_outer_id = QUuid::fromString (pasted_ids.at (0).toString ());
  const auto pasted_inner_id = QUuid::fromString (pasted_ids.at (1).toString ());
  const auto pasted_leaf_id = QUuid::fromString (pasted_ids.at (2).toString ());

  const auto pasted_inner_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (pasted_inner_id));
  ASSERT_TRUE (pasted_inner_parent.has_value ());
  EXPECT_EQ (
    pasted_inner_parent.value (),
    structure::tracks::Track::Uuid (pasted_outer_id));

  const auto pasted_leaf_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (pasted_leaf_id));
  ASSERT_TRUE (pasted_leaf_parent.has_value ());
  EXPECT_EQ (
    pasted_leaf_parent.value (),
    structure::tracks::Track::Uuid (pasted_inner_id));

  // The pasted nesting is contiguous: both pasted tracks are descendants
  // of the pasted outer folder, in list order
  const auto outer_descendants = track_collection_->get_all_descendants (
    structure::tracks::Track::Uuid (pasted_outer_id));
  ASSERT_EQ (outer_descendants.size (), 2u);
  EXPECT_EQ (
    outer_descendants[0], structure::tracks::Track::Uuid (pasted_inner_id));
  EXPECT_EQ (
    outer_descendants[1], structure::tracks::Track::Uuid (pasted_leaf_id));
}

TEST_F (TrackCollectionOperatorClipboardTest, NestedFolderPasteUndoRedo)
{
  auto outer = create_folder_track ();
  auto inner = create_folder_track ();
  auto leaf = create_audio_bus_track ();
  track_collection_->add_track (outer);
  track_collection_->add_track (inner);
  track_collection_->add_track (leaf);
  track_collection_->set_folder_parent (inner.id (), outer.id ());
  track_collection_->set_folder_parent (leaf.id (), inner.id ());

  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ outer })));
  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 3);

  // Undo removes the whole paste in one step
  undo_stack_->undo ();
  EXPECT_EQ (track_collection_->track_count (), 3);

  // Redo re-attaches the tracks with their nesting restored
  undo_stack_->redo ();
  EXPECT_EQ (track_collection_->track_count (), 6);
  const auto redone_inner_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (1).toString ())));
  ASSERT_TRUE (redone_inner_parent.has_value ());
  EXPECT_EQ (
    redone_inner_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (0).toString ())));
  const auto redone_leaf_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (2).toString ())));
  ASSERT_TRUE (redone_leaf_parent.has_value ());
  EXPECT_EQ (
    redone_leaf_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (1).toString ())));
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  NonContiguousSelectionPasteKeepsFolderTogether)
{
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  auto child = create_audio_bus_track ();
  child.get ()->setName ("C");
  track_collection_->add_track (child);
  track_collection_->set_folder_parent (child.id (), folder.id ());
  auto unrelated = add_named_track ("X");

  // The folder and the later unrelated track are selected together,
  // without the folder's child in between
  EXPECT_TRUE (track_collection_operator_->copyTracks (
    track_list ({ folder, unrelated })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 3);
  EXPECT_EQ (track_collection_->track_count (), 6);

  // The payload is canonicalized to collection order, so the pasted
  // folder is directly followed by its child, before the pasted
  // unrelated track
  EXPECT_EQ (track_name_at (3), "F 1");
  EXPECT_EQ (track_name_at (4), "C 1");
  EXPECT_EQ (track_name_at (5), "X 1");
  const auto pasted_child_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (1).toString ())));
  ASSERT_TRUE (pasted_child_parent.has_value ());
  EXPECT_EQ (
    pasted_child_parent.value (),
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (0).toString ())));
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  CraftedNonFoldableFolderParentRefusesPaste)
{
  auto track_a = add_named_track ("A");
  auto track_b = add_named_track ("B");

  const auto uuid_str = [] (const auto &id) {
    return type_safe::get (id).toString (QUuid::WithoutBraces).toStdString ();
  };
  nlohmann::json metadata = nlohmann::json::object (
    {
      { std::string (
          structure::project::ClipboardPayload::kFolderParentsMetadataKey),
       nlohmann::json::object (
          // B's parent would be A, which is not foldable
          { { uuid_str (track_b.id ()), uuid_str (track_a.id ()) } }) }
  });

  auto payload = structure::project::ClipboardPayload::create (
    registry_, structure::project::ClipboardPayload::Type::Tracks,
    { type_safe::get (track_a.id ()), type_safe::get (track_b.id ()) },
    QString ("test-project-id"), std::move (metadata));
  clipboard_.setPayload (std::move (payload));

  QSignalSpy refused_spy (
    track_collection_operator_.get (),
    &TrackCollectionOperator::operationRefused);

  EXPECT_TRUE (track_collection_operator_->pasteTracks (-1).isEmpty ());
  EXPECT_EQ (refused_spy.count (), 1);
  EXPECT_EQ (track_collection_->track_count (), 2);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  CraftedNonTrackRoutingTargetFallsBackToMaster)
{
  // A registered object that is not a track is not a valid routing
  // target
  auto fas_ref = utils::create_object<dsp::FileAudioSource> (
    registry_, utils::audio::AudioBuffer (2, 64),
    dsp::FileAudioSource::BitDepth::BIT_DEPTH_16, units::sample_rate (44100),
    units::bpm (120.0), utils::Utf8String::from_utf8_encoded_string ("test"));

  auto source = add_named_track ("A");

  nlohmann::json routing_entry = nlohmann::json::array (
    {
      nlohmann::json{
                     { std::string (
            structure::project::ClipboardPayload::kRoutingSourceMetadataKey),
          type_safe::get (source.id ())
            .toString (QUuid::WithoutBraces)
            .toStdString () },
                     { std::string (
            structure::project::ClipboardPayload::kRoutingTargetMetadataKey),
          type_safe::get (fas_ref.id ())
            .toString (QUuid::WithoutBraces)
            .toStdString () } }
  });
  nlohmann::json metadata = nlohmann::json::object (
    {
      { std::string (structure::project::ClipboardPayload::kRoutingMetadataKey),
       std::move (routing_entry) }
  });

  auto payload = structure::project::ClipboardPayload::create (
    registry_, structure::project::ClipboardPayload::Type::Tracks,
    { type_safe::get (source.id ()) }, QString ("test-project-id"),
    std::move (metadata));
  clipboard_.setPayload (std::move (payload));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 1);

  const auto pasted_output = track_routing_.get_output_track (
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (0).toString ())));
  ASSERT_TRUE (pasted_output.has_value ());
  EXPECT_EQ (pasted_output->id (), master_track_->id ());
}

TEST_F (TrackCollectionOperatorClipboardTest, PasteIntoCollapsedFolderNests)
{
  auto folder = create_folder_track ();
  folder.get ()->setName ("F");
  track_collection_->add_track (folder);
  auto child = create_audio_bus_track ();
  child.get ()->setName ("C");
  track_collection_->add_track (child);
  track_collection_->set_folder_parent (child.id (), folder.id ());
  track_collection_->set_track_expanded (folder.id (), false);
  auto other = add_named_track ("Other");

  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ other })));

  // The target position is inside the collapsed folder's child range
  const auto pasted_ids = track_collection_operator_->pasteTracks (1);
  ASSERT_EQ (pasted_ids.size (), 1);
  EXPECT_EQ (track_collection_->track_count (), 4);

  // The pasted track lands inside the folder, which expands
  EXPECT_EQ (track_name_at (0), "F");
  EXPECT_EQ (track_name_at (1), "Other 1");
  EXPECT_EQ (track_name_at (2), "C");
  const auto pasted_parent = track_collection_->get_folder_parent (
    structure::tracks::Track::Uuid (
      QUuid::fromString (pasted_ids.at (0).toString ())));
  ASSERT_TRUE (pasted_parent.has_value ());
  EXPECT_EQ (pasted_parent.value (), folder.id ());
  EXPECT_TRUE (track_collection_->get_track_expanded (folder.id ()));
}

TEST_F (TrackCollectionOperatorClipboardTest, PasteReportsSeveredRouting)
{
  auto source = add_named_track ("A");
  {
    auto group = create_audio_group_track ();
    track_routing_.add_or_replace_route (source.id (), group.id ());
    EXPECT_TRUE (
      track_collection_operator_->copyTracks (track_list ({ source })));
    track_routing_.remove_route_for_source (source.id ());
    // The group's last reference is dropped here, purging it from the
    // registry, so the copied route's target no longer resolves
  }

  QSignalSpy modified_spy (
    track_collection_operator_.get (),
    &TrackCollectionOperator::pasteContentModified);

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 1);
  ASSERT_EQ (modified_spy.count (), 1);
  EXPECT_TRUE (modified_spy.takeFirst ().at (0).toString ().contains (
    QStringLiteral ("severed")));
}

TEST_F (
  TrackCollectionOperatorClipboardTest,
  PasteRefusesWhenPluginFailsToInstantiate)
{
  auto track = create_audio_bus_track ();
  track.get ()->setName ("A");
  track_collection_->add_track (track);

  // The track's channel carries a JUCE plugin whose instantiation fails;
  // the pasted copy re-instantiates through the fixture's plugin factory,
  // which also fails, so the paste must be refused
  auto plugin = std::make_unique<plugins::JucePlugin> (
    registry_,
    [] (
      const juce::PluginDescription &, double, int,
      std::function<void (
        std::unique_ptr<juce::AudioPluginInstance>, const juce::String &)>
        callback) {
      callback (nullptr, juce::String ("Test instantiation failure"));
    },
    [] () { return units::sample_rate (44100); },
    [] () { return units::samples (256u); });
  auto descr = std::make_unique<plugins::PluginDescriptor> ();
  descr->name_ = u8"Test Fail Plugin";
  descr->protocol_ = plugins::Protocol::ProtocolType::VST3;
  auto config = std::make_unique<plugins::PluginConfiguration> ();
  config->descr_ = std::move (descr);
  plugin->set_configuration (*config);
  ASSERT_TRUE (QTest::qWaitFor ([&plugin] () {
    return plugin->instantiationStatus ()
           == plugins::Plugin::InstantiationStatus::Failed;
  }));
  registry_.register_object (*plugin);
  auto plugin_ref =
    plugins::PluginUuidReference (plugin->get_uuid (), registry_);
  plugin.release ();
  track.get ()->channel ()->inserts ()->insert_plugin (plugin_ref);

  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ track })));

  QSignalSpy refused_spy (
    track_collection_operator_.get (),
    &TrackCollectionOperator::operationRefused);

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  EXPECT_TRUE (pasted_ids.isEmpty ());
  EXPECT_EQ (refused_spy.count (), 1);
  EXPECT_EQ (track_collection_->track_count (), 1);
  EXPECT_EQ (undo_stack_->count (), 0);
}

TEST_F (TrackCollectionOperatorClipboardTest, PasteSucceedsWithInstantiatedPlugin)
{
  auto track = create_audio_bus_track ();
  track.get ()->setName ("A");
  track_collection_->add_track (track);

  // A plugin whose descriptor matches no bundled Faust plugin acts as a
  // pass-through and finishes instantiation synchronously
  auto plugin_ref =
    utils::create_object<plugins::FaustPlugin> (registry_, registry_);
  auto descr = std::make_unique<plugins::PluginDescriptor> ();
  descr->name_ = u8"Test Pass-Through Plugin";
  descr->protocol_ = plugins::Protocol::ProtocolType::Internal;
  auto config = std::make_unique<plugins::PluginConfiguration> ();
  config->descr_ = std::move (descr);
  plugin_ref.get ()->set_configuration (*config);
  EXPECT_EQ (
    plugin_ref.get ()->instantiationStatus (),
    plugins::Plugin::InstantiationStatus::Successful);
  track.get ()->channel ()->inserts ()->insert_plugin (plugin_ref);

  EXPECT_TRUE (track_collection_operator_->copyTracks (track_list ({ track })));

  const auto pasted_ids = track_collection_operator_->pasteTracks (-1);
  ASSERT_EQ (pasted_ids.size (), 1);
  EXPECT_EQ (track_collection_->track_count (), 2);

  // The pasted track carries a re-instantiated plugin
  const auto pasted_track =
    structure::tracks::TrackUuidReference{
      structure::tracks::Track::Uuid (
        QUuid::fromString (pasted_ids.at (0).toString ())),
      registry_
    }
      .get ();
  std::vector<plugins::PluginUuidReference> pasted_plugins;
  pasted_track->channel ()->get_plugins (pasted_plugins);
  ASSERT_EQ (pasted_plugins.size (), 1u);
  EXPECT_EQ (
    pasted_plugins.front ().get ()->instantiationStatus (),
    plugins::Plugin::InstantiationStatus::Successful);
}

} // namespace zrythm::actions
