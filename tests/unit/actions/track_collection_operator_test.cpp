// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_collection_operator.h"
#include "structure/tracks/track_collection.h"
#include "structure/tracks/track_factory.h"
#include "undo/undo_stack.h"

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
    // Set up dependencies for TrackFactory
    track_registry_ = std::make_unique<structure::tracks::TrackRegistry> ();
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (*track_registry_);

    // Create track factory with dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,
      file_audio_source_registry_,
      plugin_registry_,
      port_registry_,
      param_registry_,
      obj_registry_,
      *track_registry_,
      transport_,
      soloed_tracks_exist_getter_,
    };
    track_factory_ =
      std::make_unique<structure::tracks::TrackFactory> (factory_deps);

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

  // Create minimal dependencies for track creation
  dsp::TempoMap                   tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper            tempo_map_wrapper_{ tempo_map_ };
  dsp::FileAudioSourceRegistry    file_audio_source_registry_;
  plugins::PluginRegistry         plugin_registry_;
  dsp::PortRegistry               port_registry_;
  dsp::ProcessorParameterRegistry param_registry_{ port_registry_ };
  structure::arrangement::ArrangerObjectRegistry obj_registry_;
  dsp::graph_test::MockTransport                 transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  std::unique_ptr<structure::tracks::TrackRegistry>   track_registry_;
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
  tracks.append (track.get_object_base ());

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
  tracks.append (track.get_object_base ());

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
  tracks.append (track1.get_object_base ());
  tracks.append (nullptr); // Add null track
  tracks.append (track2.get_object_base ());

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

  // Move track1 to position 2
  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get_object_base ());

  track_collection_operator_->moveTracks (tracks, 2);

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
  tracks.append (track3.get_object_base ());

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
  tracks.append (track2.get_object_base ());

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

  // Move track2 and track3 so track2 ends at position 2
  QList<structure::tracks::Track *> tracks;
  tracks.append (track2.get_object_base ());
  tracks.append (track3.get_object_base ());

  track_collection_operator_->moveTracks (tracks, 2);

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

  // Move track2 and track4 so track2 ends at position 3
  QList<structure::tracks::Track *> tracks;
  tracks.append (track2.get_object_base ());
  tracks.append (track4.get_object_base ());

  track_collection_operator_->moveTracks (tracks, 3);

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

  // Move track1 to position 2
  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get_object_base ());

  track_collection_operator_->moveTracks (tracks, 2);

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

  // Move track1 to position 2
  QList<structure::tracks::Track *> tracks;
  tracks.append (track1.get_object_base ());

  track_collection_operator_->moveTracks (tracks, 2);
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
  tracks.append (track1.get_object_base ());

  track_collection_operator_->moveTracks (tracks, 2);

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
  tracks.append (track1.get_object_base ());

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

  // Move track2 and track3
  QList<structure::tracks::Track *> tracks;
  tracks.append (track2.get_object_base ());
  tracks.append (track3.get_object_base ());

  track_collection_operator_->moveTracks (tracks, 2);

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

} // namespace zrythm::actions
