// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_creator.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class ArrangerObjectCreatorTest : public ::testing::Test
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

    // Create mock snap grids with proper constructors
    snap_grid_timeline = std::make_unique<dsp::SnapGrid> (
      tempo_map_, utils::NoteLength::Note_1_4, [] () { return 100.0; });
    snap_grid_editor = std::make_unique<dsp::SnapGrid> (
      tempo_map_, utils::NoteLength::Note_1_4, [] () { return 50.0; });

    // Create arranger object factory with proper dependencies
    structure::arrangement::ArrangerObjectFactory::Dependencies obj_factory_deps{
      .tempo_map_ = tempo_map_,
      .object_registry_ = obj_registry_,
      .file_audio_source_registry_ = file_audio_source_registry_,
      .musical_mode_getter_ = [] () { return true; },
      .last_timeline_obj_len_provider_ = [] () { return 100.0; },
      .last_editor_obj_len_provider_ = [] () { return 50.0; },
      .automation_curve_algorithm_provider_ =
        [] () { return dsp::CurveOptions::Algorithm::Exponent; }
    };

    arranger_object_factory =
      std::make_unique<structure::arrangement::ArrangerObjectFactory> (
        obj_factory_deps, [] () { return 44100.0; }, [] () { return 120.0; });

    // Create the creator
    creator = std::make_unique<ArrangerObjectCreator> (
      *undo_stack_, *arranger_object_factory, *snap_grid_timeline,
      *snap_grid_editor);
  }

  // Create minimal dependencies for track creation
  dsp::TempoMap                   tempo_map_{ 44100.0 };
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
  std::unique_ptr<dsp::SnapGrid>                      snap_grid_timeline;
  std::unique_ptr<dsp::SnapGrid>                      snap_grid_editor;
  std::unique_ptr<structure::arrangement::ArrangerObjectFactory>
                                         arranger_object_factory;
  std::unique_ptr<ArrangerObjectCreator> creator;
};

// Test basic marker creation functionality
TEST_F (ArrangerObjectCreatorTest, AddMarker)
{
  auto * marker_track = singleton_tracks_->markerTrack ();
  ASSERT_NE (marker_track, nullptr);

  const double  start_ticks = 100.0;
  const QString name = "Test Marker";

  auto * marker = creator->addMarker (
    structure::arrangement::Marker::MarkerType::Custom, marker_track, name,
    start_ticks);

  ASSERT_NE (marker, nullptr);
  EXPECT_EQ (
    marker->markerType (), structure::arrangement::Marker::MarkerType::Custom);
  EXPECT_EQ (marker->name ()->name (), name);
  EXPECT_DOUBLE_EQ (marker->position ()->ticks (), start_ticks);

  // Verify marker was added to track
  auto markers = marker_track->get_children_view ();
  EXPECT_TRUE (std::ranges::any_of (markers, [&] (auto * m) {
    return m == marker;
  }));
}

// Test empty chord region creation
TEST_F (ArrangerObjectCreatorTest, AddEmptyChordRegion)
{
  auto * chord_track = singleton_tracks_->chordTrack ();
  ASSERT_NE (chord_track, nullptr);

  const double start_ticks = 300.0;

  auto * region = creator->addEmptyChordRegion (chord_track, start_ticks);

  ASSERT_NE (region, nullptr);
  EXPECT_DOUBLE_EQ (region->position ()->ticks (), start_ticks);

  // Verify region was added to track
  auto regions = chord_track->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::ChordRegion>::get_children_view ();
  EXPECT_TRUE (std::ranges::any_of (regions, [&] (auto * r) {
    return r == region;
  }));
}

// Test undo stack integration
TEST_F (ArrangerObjectCreatorTest, UndoStackIntegration)
{
  auto * marker_track = singleton_tracks_->markerTrack ();
  ASSERT_NE (marker_track, nullptr);

  const double start_ticks = 900.0;
  const int    initial_undo_count = undo_stack_->count ();

  auto * marker = creator->addMarker (
    structure::arrangement::Marker::MarkerType::Custom, marker_track,
    "Undo Test", start_ticks);

  ASSERT_NE (marker, nullptr);

  // Verify a command was pushed to the undo stack
  EXPECT_GT (undo_stack_->count (), initial_undo_count);

  // Test undo functionality
  undo_stack_->undo ();

  // Marker should be removed from track after undo
  auto markers = marker_track->get_children_view ();
  EXPECT_FALSE (std::ranges::any_of (markers, [&] (auto * m) {
    return m == marker;
  }));

  // Test redo functionality
  undo_stack_->redo ();

  // Marker should be back after redo
  markers = marker_track->get_children_view ();
  EXPECT_TRUE (std::ranges::any_of (markers, [&] (auto * m) {
    return m == marker;
  }));
}

} // namespace zrythm::actions
