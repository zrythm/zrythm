// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_creator.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class TrackCreatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Set up dependencies
    track_registry_ = std::make_unique<structure::tracks::TrackRegistry> ();

    // Create singleton tracks
    singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();

    // Create track collection
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (*track_registry_);

    // Create track routing
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (*track_registry_);

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

    // Create and register singleton tracks
    auto master_track_ref =
      track_factory_->create_empty_track<structure::tracks::MasterTrack> ();
    auto chord_track_ref =
      track_factory_->create_empty_track<structure::tracks::ChordTrack> ();
    auto modulator_track_ref =
      track_factory_->create_empty_track<structure::tracks::ModulatorTrack> ();
    auto marker_track_ref =
      track_factory_->create_empty_track<structure::tracks::MarkerTrack> ();

    // Set singleton track pointers
    singleton_tracks_->setMasterTrack (
      master_track_ref.get_object_as<structure::tracks::MasterTrack> ());
    singleton_tracks_->setChordTrack (
      chord_track_ref.get_object_as<structure::tracks::ChordTrack> ());
    singleton_tracks_->setModulatorTrack (
      modulator_track_ref.get_object_as<structure::tracks::ModulatorTrack> ());
    singleton_tracks_->setMarkerTrack (
      marker_track_ref.get_object_as<structure::tracks::MarkerTrack> ());

    // Add singleton tracks to collection
    track_collection_->add_track (master_track_ref);
    track_collection_->add_track (chord_track_ref);
    track_collection_->add_track (modulator_track_ref);
    track_collection_->add_track (marker_track_ref);

    // Create track creator
    track_creator_ = std::make_unique<TrackCreator> (
      *undo_stack_, *track_factory_, *track_collection_, *track_routing_,
      *singleton_tracks_);
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
  std::unique_ptr<structure::tracks::SingletonTracks> singleton_tracks_;
  std::unique_ptr<structure::tracks::TrackCollection> track_collection_;
  std::unique_ptr<structure::tracks::TrackRouting>    track_routing_;
  std::unique_ptr<structure::tracks::TrackFactory>    track_factory_;
  std::unique_ptr<undo::UndoStack>                    undo_stack_;
  std::unique_ptr<TrackCreator>                       track_creator_;
};

// Test creating an audio track
TEST_F (TrackCreatorTest, CreateAudioTrack)
{
  auto result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);

  // Should return a valid QVariant containing a track
  EXPECT_TRUE (result.isValid ());
  EXPECT_TRUE (result.canConvert<structure::tracks::Track *> ());

  auto * track = result.value<structure::tracks::Track *> ();
  // Track should be in collection
  EXPECT_TRUE (track_collection_->contains (track->get_uuid ()));

  // Audio track should be routed to master
  auto route = track_routing_->get_output_track (track->get_uuid ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), singleton_tracks_->masterTrack ()->get_uuid ());
}

// Test creating a MIDI track
TEST_F (TrackCreatorTest, CreateMidiTrack)
{
  auto result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);

  EXPECT_TRUE (result.isValid ());
  EXPECT_TRUE (result.canConvert<structure::tracks::Track *> ());

  auto * track = result.value<structure::tracks::Track *> ();
  // Track should be in collection
  EXPECT_TRUE (track_collection_->contains (track->get_uuid ()));

  // MIDI track should not be routed to master (no audio output)
  auto route = track_routing_->get_output_track (track->get_uuid ());
  EXPECT_FALSE (route.has_value ());
}

// Test creating an instrument track
TEST_F (TrackCreatorTest, CreateInstrumentTrack)
{
  auto result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Instrument);

  EXPECT_TRUE (result.isValid ());
  EXPECT_TRUE (result.canConvert<structure::tracks::Track *> ());

  auto * track = result.value<structure::tracks::Track *> ();
  // Track should be in collection
  EXPECT_TRUE (track_collection_->contains (track->get_uuid ()));

  // Instrument track should be routed to master (audio output)
  auto route = track_routing_->get_output_track (track->get_uuid ());
  EXPECT_TRUE (route.has_value ());
  EXPECT_EQ (route->id (), singleton_tracks_->masterTrack ()->get_uuid ());
}

// Test creating multiple tracks
TEST_F (TrackCreatorTest, CreateMultipleTracks)
{
  // Create first track
  auto result1 = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto * track1 = result1.value<structure::tracks::Track *> ();
  structure::tracks::Track::Uuid track1_id = track1->get_uuid ();

  // Create second track
  auto result2 = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Midi);
  auto * track2 = result2.value<structure::tracks::Track *> ();
  structure::tracks::Track::Uuid track2_id = track2->get_uuid ();

  // Both tracks should be in collection
  EXPECT_TRUE (track_collection_->contains (track1_id));
  EXPECT_TRUE (track_collection_->contains (track2_id));
  EXPECT_EQ (
    track_collection_->track_count (), 6); // 4 singletons + 2 new tracks
}

// Test undo/redo functionality
TEST_F (TrackCreatorTest, UndoRedoTrackCreation)
{
  auto result = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto * track = result.value<structure::tracks::Track *> ();
  structure::tracks::Track::Uuid track_id = track->get_uuid ();

  // Track should be present after creation
  EXPECT_TRUE (track_collection_->contains (track_id));
  EXPECT_EQ (track_collection_->track_count (), 5); // 4 singletons + 1 new track

  // Undo should remove the track
  undo_stack_->undo ();
  EXPECT_FALSE (track_collection_->contains (track_id));
  EXPECT_EQ (track_collection_->track_count (), 4); // only singletons remain

  // Redo should add the track back
  undo_stack_->redo ();
  EXPECT_TRUE (track_collection_->contains (track_id));
  EXPECT_EQ (track_collection_->track_count (), 5); // 4 singletons + 1 new track
}

// Test unique name generation
TEST_F (TrackCreatorTest, UniqueNameGeneration)
{
  // Create first track with default name
  auto result1 = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto *            track1 = result1.value<structure::tracks::Track *> ();
  utils::Utf8String name1 = track1->get_name ();

  // Create second track - should get unique name
  auto result2 = track_creator_->addEmptyTrackFromType (
    structure::tracks::Track::Type::Audio);
  auto *            track2 = result2.value<structure::tracks::Track *> ();
  utils::Utf8String name2 = track2->get_name ();

  // Names should be different
  EXPECT_NE (name1, name2);
  EXPECT_TRUE (name2.str ().ends_with ("1") || name2.str ().ends_with ("2"));
}

// Test error case: trying to create singleton track that already exists
TEST_F (TrackCreatorTest, CreateSingletonTrackError)
{
  // Singleton tracks are already created in SetUp, trying to create another
  // should fail
  EXPECT_THROW (
    track_creator_->addEmptyTrackFromType (
      structure::tracks::Track::Type::Master),
    std::invalid_argument);

  EXPECT_THROW (
    track_creator_->addEmptyTrackFromType (structure::tracks::Track::Type::Chord),
    std::invalid_argument);

  EXPECT_THROW (
    track_creator_->addEmptyTrackFromType (
      structure::tracks::Track::Type::Modulator),
    std::invalid_argument);

  EXPECT_THROW (
    track_creator_->addEmptyTrackFromType (
      structure::tracks::Track::Type::Marker),
    std::invalid_argument);
}

} // namespace zrythm::actions
