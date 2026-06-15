// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/arranger_object_creator.h"
#include "structure/tracks/track_factory.h"
#include "structure/tracks/track_routing.h"
#include "structure/tracks/tracklist.h"
#include "undo/undo_stack.h"
#include "utils/object_registry.h"

#include "helpers/scoped_qcoreapplication.h"

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
    // Must be created first and destroyed last to keep the event dispatcher
    // alive during teardown of timer-bearing objects (ChordTrack, UndoStack).
    app_ = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    // Create singleton tracks
    singleton_tracks_ = std::make_unique<structure::tracks::SingletonTracks> ();

    // Create track collection
    track_collection_ =
      std::make_unique<structure::tracks::TrackCollection> (registry_);

    // Create track routing
    track_routing_ =
      std::make_unique<structure::tracks::TrackRouting> (registry_);

    // Create track factory with dependencies
    structure::tracks::FinalTrackDependencies factory_deps{
      tempo_map_wrapper_,          registry_, transport_,
      soloed_tracks_exist_getter_, {},
    };
    track_factory_ = std::make_unique<structure::tracks::TrackFactory> (
      [factory_deps] () -> structure::tracks::FinalTrackDependencies {
        return factory_deps;
      });

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
      tempo_map_, dsp::notes::NoteLength::Note_1_4, [] () { return 100.0; });
    snap_grid_editor = std::make_unique<dsp::SnapGrid> (
      tempo_map_, dsp::notes::NoteLength::Note_1_4, [] () { return 50.0; });

    // Create arranger object factory with proper dependencies
    structure::arrangement::ArrangerObjectFactory::Dependencies obj_factory_deps{
      .tempo_map_ = tempo_map_,
      .registry_ = registry_,
      .musical_mode_getter_ = [] () { return true; },
      .last_timeline_obj_len_provider_ = [] () { return 100.0; },
      .last_editor_obj_len_provider_ = [] () { return 50.0; },
      .automation_curve_algorithm_provider_ =
        [] () { return dsp::CurveOptions::Algorithm::Exponent; }
    };

    arranger_object_factory = std::make_unique<
      structure::arrangement::ArrangerObjectFactory> (
      obj_factory_deps, [] () { return units::sample_rate (44100); },
      [] () { return 120.0; });

    // Create the creator
    creator = std::make_unique<ArrangerObjectCreator> (
      *undo_stack_, *arranger_object_factory, *snap_grid_timeline,
      *snap_grid_editor);
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app_;

  dsp::TempoMap                  tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper           tempo_map_wrapper_{ tempo_map_ };
  utils::ObjectRegistry          registry_;
  dsp::graph_test::MockTransport transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

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

TEST_F (ArrangerObjectCreatorTest, AddMidiRegionForRecording)
{
  auto track_ref =
    track_factory_->create_empty_track<structure::tracks::MidiTrack> ();
  auto * midi_track = track_ref.get_object_as<structure::tracks::MidiTrack> ();
  ASSERT_NE (midi_track, nullptr);
  midi_track->setName (u8"MIDI Track");
  track_collection_->add_track (track_ref);

  auto * lanes = midi_track->lanes ();
  ASSERT_NE (lanes, nullptr);
  lanes->create_missing_lanes (0);
  auto * lane = lanes->at (0);
  ASSERT_NE (lane, nullptr);

  const double start_ticks = 480.0;
  auto         region_ref = creator->add_midi_region_for_recording (
    *midi_track, *lane, units::ticks (start_ticks));

  auto * region =
    region_ref.get_object_as<structure::arrangement::MidiRegion> ();
  ASSERT_NE (region, nullptr);
  EXPECT_DOUBLE_EQ (region->position ()->ticks (), start_ticks);

  auto midi_regions = lane->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiRegion>::get_children_view ();
  EXPECT_TRUE (std::ranges::any_of (midi_regions, [&] (auto * r) {
    return r == region;
  }));
}

TEST_F (ArrangerObjectCreatorTest, AddMidiRegionForRecordingUndoRedo)
{
  auto track_ref =
    track_factory_->create_empty_track<structure::tracks::MidiTrack> ();
  auto * midi_track = track_ref.get_object_as<structure::tracks::MidiTrack> ();
  track_collection_->add_track (track_ref);

  auto * lanes = midi_track->lanes ();
  lanes->create_missing_lanes (0);
  auto * lane = lanes->at (0);

  const int initial_count = undo_stack_->count ();
  auto      region_ref = creator->add_midi_region_for_recording (
    *midi_track, *lane, units::ticks (100.0));
  EXPECT_GT (undo_stack_->count (), initial_count);

  auto * region =
    region_ref.get_object_as<structure::arrangement::MidiRegion> ();

  undo_stack_->undo ();
  auto midi_regions = lane->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiRegion>::get_children_view ();
  EXPECT_FALSE (std::ranges::any_of (midi_regions, [&] (auto * r) {
    return r == region;
  }));

  undo_stack_->redo ();
  midi_regions = lane->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiRegion>::get_children_view ();
  EXPECT_TRUE (std::ranges::any_of (midi_regions, [&] (auto * r) {
    return r == region;
  }));
}

TEST_F (ArrangerObjectCreatorTest, AddMidiControlEvent)
{
  auto track_ref =
    track_factory_->create_empty_track<structure::tracks::MidiTrack> ();
  auto * midi_track = track_ref.get_object_as<structure::tracks::MidiTrack> ();
  track_collection_->add_track (track_ref);

  auto * lanes = midi_track->lanes ();
  lanes->create_missing_lanes (0);
  auto * lane = lanes->at (0);

  auto region_ref = creator->add_midi_region_for_recording (
    *midi_track, *lane, units::ticks (0.0));
  auto * region =
    region_ref.get_object_as<structure::arrangement::MidiRegion> ();
  ASSERT_NE (region, nullptr);

  using EventType = structure::arrangement::MidiControlEvent::EventType;
  auto * ev = creator->add_midi_control_event (
    *region, units::ticks (200.0), EventType::ControlChange, 0, 1, 64);
  ASSERT_NE (ev, nullptr);
  EXPECT_EQ (ev->controlType (), EventType::ControlChange);
  EXPECT_EQ (ev->channel (), 0);
  EXPECT_EQ (ev->controller (), 1);
  EXPECT_EQ (ev->value (), 64);
  EXPECT_DOUBLE_EQ (ev->position ()->ticks (), 200.0);

  auto control_events = region->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiControlEvent>::get_children_view ();
  EXPECT_TRUE (std::ranges::any_of (control_events, [&] (auto * e) {
    return e == ev;
  }));
}

TEST_F (ArrangerObjectCreatorTest, AddMidiControlEventPitchBend)
{
  auto track_ref =
    track_factory_->create_empty_track<structure::tracks::MidiTrack> ();
  auto * midi_track = track_ref.get_object_as<structure::tracks::MidiTrack> ();
  track_collection_->add_track (track_ref);

  auto * lanes = midi_track->lanes ();
  lanes->create_missing_lanes (0);
  auto * lane = lanes->at (0);

  auto region_ref = creator->add_midi_region_for_recording (
    *midi_track, *lane, units::ticks (0.0));
  auto * region =
    region_ref.get_object_as<structure::arrangement::MidiRegion> ();

  using EventType = structure::arrangement::MidiControlEvent::EventType;
  auto * ev = creator->add_midi_control_event (
    *region, units::ticks (500.0), EventType::PitchBend, 2, 0, 8192);
  ASSERT_NE (ev, nullptr);
  EXPECT_EQ (ev->controlType (), EventType::PitchBend);
  EXPECT_EQ (ev->channel (), 2);
  EXPECT_EQ (ev->value (), 8192);
}

TEST_F (ArrangerObjectCreatorTest, AddMidiControlEventUndoRedo)
{
  auto track_ref =
    track_factory_->create_empty_track<structure::tracks::MidiTrack> ();
  auto * midi_track = track_ref.get_object_as<structure::tracks::MidiTrack> ();
  track_collection_->add_track (track_ref);

  auto * lanes = midi_track->lanes ();
  lanes->create_missing_lanes (0);
  auto * lane = lanes->at (0);

  auto region_ref = creator->add_midi_region_for_recording (
    *midi_track, *lane, units::ticks (0.0));
  auto * region =
    region_ref.get_object_as<structure::arrangement::MidiRegion> ();

  using EventType = structure::arrangement::MidiControlEvent::EventType;
  const int initial_count = undo_stack_->count ();
  creator->add_midi_control_event (
    *region, units::ticks (100.0), EventType::ControlChange, 0, 7, 127);
  EXPECT_GT (undo_stack_->count (), initial_count);

  auto control_events = region->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiControlEvent>::get_children_view ();
  EXPECT_FALSE (control_events.empty ());

  undo_stack_->undo ();
  control_events = region->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiControlEvent>::get_children_view ();
  EXPECT_TRUE (control_events.empty ());

  undo_stack_->redo ();
  control_events = region->structure::arrangement::ArrangerObjectOwner<
    structure::arrangement::MidiControlEvent>::get_children_view ();
  EXPECT_FALSE (control_events.empty ());
}

// === Chord object tests ===

TEST_F (ArrangerObjectCreatorTest, AddChordObjectFromFields)
{
  auto * chord_track = singleton_tracks_->chordTrack ();
  ASSERT_NE (chord_track, nullptr);

  // Create a chord region.
  auto * region = creator->addEmptyChordRegion (chord_track, 0.0);
  ASSERT_NE (region, nullptr);

  // Add a D Minor 7 chord with bass note A.
  auto * co = creator->addChordObjectFromFields (
    region, 0.0, dsp::MusicalNote::D, dsp::ChordType::Minor,
    dsp::ChordAccent::Seventh, true, dsp::MusicalNote::A, 1);
  ASSERT_NE (co, nullptr);

  auto * desc = co->chordDescriptor ();
  EXPECT_EQ (desc->rootNote (), dsp::MusicalNote::D);
  EXPECT_EQ (desc->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (desc->chordAccent (), dsp::ChordAccent::Seventh);
  EXPECT_TRUE (desc->hasBass ());
  EXPECT_EQ (desc->bassNote (), dsp::MusicalNote::A);
  EXPECT_EQ (desc->inversion (), 1);

  // Undo removes the chord object.
  undo_stack_->undo ();
  EXPECT_EQ (region->chordObjects ()->rowCount (), 0);

  // Redo re-adds it.
  undo_stack_->redo ();
  EXPECT_EQ (region->chordObjects ()->rowCount (), 1);
}

TEST_F (ArrangerObjectCreatorTest, AddChordObjectFromDescriptor)
{
  auto * chord_track = singleton_tracks_->chordTrack ();
  ASSERT_NE (chord_track, nullptr);

  auto * region = creator->addEmptyChordRegion (chord_track, 0.0);

  // Build a source descriptor.
  dsp::ChordDescriptor src (
    dsp::MusicalNote::E, dsp::ChordType::Diminished, dsp::ChordAccent::Ninth, 2,
    dsp::MusicalNote::B);

  auto * co = creator->addChordObjectFromDescriptor (region, 480.0, &src);
  ASSERT_NE (co, nullptr);
  EXPECT_EQ (co->chordDescriptor ()->rootNote (), dsp::MusicalNote::E);
  EXPECT_EQ (co->chordDescriptor ()->chordType (), dsp::ChordType::Diminished);
  EXPECT_EQ (co->chordDescriptor ()->chordAccent (), dsp::ChordAccent::Ninth);
  EXPECT_EQ (co->chordDescriptor ()->inversion (), 2);
  EXPECT_TRUE (co->chordDescriptor ()->hasBass ());
  EXPECT_EQ (co->chordDescriptor ()->bassNote (), dsp::MusicalNote::B);
}

TEST_F (ArrangerObjectCreatorTest, AddChordObjectFromDescriptorNullReturnsNull)
{
  auto * chord_track = singleton_tracks_->chordTrack ();
  ASSERT_NE (chord_track, nullptr);

  auto * region = creator->addEmptyChordRegion (chord_track, 0.0);

  auto * co = creator->addChordObjectFromDescriptor (region, 0.0, nullptr);
  EXPECT_EQ (co, nullptr);
}

TEST_F (ArrangerObjectCreatorTest, EditChordObjectsDescriptor)
{
  auto * chord_track = singleton_tracks_->chordTrack ();
  ASSERT_NE (chord_track, nullptr);

  auto * region = creator->addEmptyChordRegion (chord_track, 0.0);

  // Add two C Major chord objects.
  auto * co1 = creator->addChordObjectFromFields (
    region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major,
    dsp::ChordAccent::None, false, dsp::MusicalNote::C, 0);
  auto * co2 = creator->addChordObjectFromFields (
    region, 480.0, dsp::MusicalNote::C, dsp::ChordType::Major,
    dsp::ChordAccent::None, false, dsp::MusicalNote::C, 0);

  // Batch-edit both to D Minor 7.
  creator->editChordObjectsDescriptor (
    QVariantList{ QVariant::fromValue (co1), QVariant::fromValue (co2) },
    dsp::MusicalNote::D, dsp::ChordType::Minor, dsp::ChordAccent::Seventh,
    false, dsp::MusicalNote::C, 0);

  // Both should now be D Minor 7.
  EXPECT_EQ (co1->chordDescriptor ()->rootNote (), dsp::MusicalNote::D);
  EXPECT_EQ (co1->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (co1->chordDescriptor ()->chordAccent (), dsp::ChordAccent::Seventh);
  EXPECT_EQ (co2->chordDescriptor ()->rootNote (), dsp::MusicalNote::D);
  EXPECT_EQ (co2->chordDescriptor ()->chordType (), dsp::ChordType::Minor);

  // Undo restores both to C Major (one undo step for the batch).
  undo_stack_->undo ();
  EXPECT_EQ (co1->chordDescriptor ()->rootNote (), dsp::MusicalNote::C);
  EXPECT_EQ (co1->chordDescriptor ()->chordType (), dsp::ChordType::Major);
  EXPECT_EQ (co1->chordDescriptor ()->chordAccent (), dsp::ChordAccent::None);
  EXPECT_EQ (co2->chordDescriptor ()->rootNote (), dsp::MusicalNote::C);
  EXPECT_EQ (co2->chordDescriptor ()->chordType (), dsp::ChordType::Major);

  // Redo re-applies.
  undo_stack_->redo ();
  EXPECT_EQ (co1->chordDescriptor ()->rootNote (), dsp::MusicalNote::D);
  EXPECT_EQ (co1->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
}

TEST_F (ArrangerObjectCreatorTest, EditChordObjectsDescriptorIsOneUndoStep)
{
  auto * chord_track = singleton_tracks_->chordTrack ();
  ASSERT_NE (chord_track, nullptr);

  auto * region = creator->addEmptyChordRegion (chord_track, 0.0);

  auto * co1 = creator->addChordObjectFromFields (
    region, 0.0, dsp::MusicalNote::C, dsp::ChordType::Major,
    dsp::ChordAccent::None, false, dsp::MusicalNote::C, 0);
  auto * co2 = creator->addChordObjectFromFields (
    region, 480.0, dsp::MusicalNote::G, dsp::ChordType::Major,
    dsp::ChordAccent::None, false, dsp::MusicalNote::C, 0);

  const int count_before = undo_stack_->count ();

  creator->editChordObjectsDescriptor (
    QVariantList{ QVariant::fromValue (co1), QVariant::fromValue (co2) },
    dsp::MusicalNote::A, dsp::ChordType::Minor, dsp::ChordAccent::None, false,
    dsp::MusicalNote::C, 0);

  // The edit actually applied to both objects.
  EXPECT_EQ (co1->chordDescriptor ()->rootNote (), dsp::MusicalNote::A);
  EXPECT_EQ (co1->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (co2->chordDescriptor ()->rootNote (), dsp::MusicalNote::A);
  EXPECT_EQ (co2->chordDescriptor ()->chordType (), dsp::ChordType::Minor);

  // The batch edit should add exactly one entry to the undo stack (the macro).
  EXPECT_EQ (undo_stack_->count (), count_before + 1);

  // Undo restores the originals as one step.
  undo_stack_->undo ();
  EXPECT_EQ (co1->chordDescriptor ()->rootNote (), dsp::MusicalNote::C);
  EXPECT_EQ (co1->chordDescriptor ()->chordType (), dsp::ChordType::Major);
  EXPECT_EQ (co2->chordDescriptor ()->rootNote (), dsp::MusicalNote::G);
  EXPECT_EQ (co2->chordDescriptor ()->chordType (), dsp::ChordType::Major);

  // Redo re-applies the batch to both objects.
  undo_stack_->redo ();
  EXPECT_EQ (co1->chordDescriptor ()->rootNote (), dsp::MusicalNote::A);
  EXPECT_EQ (co1->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
  EXPECT_EQ (co1->chordDescriptor ()->chordAccent (), dsp::ChordAccent::None);
  EXPECT_EQ (co2->chordDescriptor ()->rootNote (), dsp::MusicalNote::A);
  EXPECT_EQ (co2->chordDescriptor ()->chordType (), dsp::ChordType::Minor);
}

} // namespace zrythm::actions
