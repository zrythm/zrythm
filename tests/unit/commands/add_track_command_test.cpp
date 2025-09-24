// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_track_command.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{

class AddEmptyTrackCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a test track
    auto track_ref = track_registry_.create_object<
      structure::tracks::AudioTrack> (dependencies_);
    test_track_ref_ = track_ref;

    // Initialize collection
    collection_ =
      std::make_unique<structure::tracks::TrackCollection> (track_registry_);
  }

  // Create minimal dependencies for track creation
  dsp::TempoMap                   tempo_map_{ 44100.0 * mp_units::si::hertz };
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

  structure::tracks::TrackRegistry track_registry_;

  structure::tracks::FinalTrackDependencies dependencies_{
    tempo_map_wrapper_,
    file_audio_source_registry_,
    plugin_registry_,
    port_registry_,
    param_registry_,
    obj_registry_,
    track_registry_,
    transport_,
    soloed_tracks_exist_getter_,
  };
  structure::tracks::TrackUuidReference test_track_ref_{ track_registry_ };
  std::unique_ptr<structure::tracks::TrackCollection> collection_;
};

// Test initial state after construction
TEST_F (AddEmptyTrackCommandTest, InitialState)
{
  AddEmptyTrackCommand command (*collection_, test_track_ref_);

  // Initially, the track should not be in the collection
  EXPECT_FALSE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 0);
}

// Test redo operation adds track to collection
TEST_F (AddEmptyTrackCommandTest, RedoAddsTrack)
{
  AddEmptyTrackCommand command (*collection_, test_track_ref_);

  command.redo ();

  // After redo, the track should be in the collection
  EXPECT_TRUE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 1);
}

// Test undo operation removes track from collection
TEST_F (AddEmptyTrackCommandTest, UndoRemovesTrack)
{
  AddEmptyTrackCommand command (*collection_, test_track_ref_);

  // First add the track
  command.redo ();
  EXPECT_TRUE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  // Then undo should remove it
  command.undo ();
  EXPECT_FALSE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 0);
}

// Test multiple undo/redo cycles
TEST_F (AddEmptyTrackCommandTest, MultipleUndoRedoCycles)
{
  AddEmptyTrackCommand command (*collection_, test_track_ref_);

  // First cycle
  command.redo ();
  EXPECT_TRUE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  command.undo ();
  EXPECT_FALSE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 0);

  // Second cycle
  command.redo ();
  EXPECT_TRUE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  command.undo ();
  EXPECT_FALSE (collection_->contains (test_track_ref_.id ()));
  EXPECT_EQ (collection_->track_count (), 0);
}

// Test command text
TEST_F (AddEmptyTrackCommandTest, CommandText)
{
  AddEmptyTrackCommand command (*collection_, test_track_ref_);

  // The command should have the default text "Add Track"
  EXPECT_EQ (command.text (), QString ("Add Track"));
}

// Test with different track types
TEST_F (AddEmptyTrackCommandTest, DifferentTrackTypes)
{
  // Test with MIDI track
  auto midi_track_ref =
    track_registry_.create_object<structure::tracks::MidiTrack> (dependencies_);

  AddEmptyTrackCommand midi_command (*collection_, midi_track_ref);

  midi_command.redo ();
  EXPECT_TRUE (collection_->contains (midi_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  midi_command.undo ();
  EXPECT_FALSE (collection_->contains (midi_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 0);

  // Test with Instrument track
  auto instrument_track_ref = track_registry_.create_object<
    structure::tracks::InstrumentTrack> (dependencies_);

  AddEmptyTrackCommand instrument_command (*collection_, instrument_track_ref);

  instrument_command.redo ();
  EXPECT_TRUE (collection_->contains (instrument_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  instrument_command.undo ();
  EXPECT_FALSE (collection_->contains (instrument_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 0);
}

// Test adding multiple tracks to same collection
TEST_F (AddEmptyTrackCommandTest, MultipleTracksSameCollection)
{
  // Create second track
  auto second_track_ref =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);

  AddEmptyTrackCommand command1 (*collection_, test_track_ref_);
  AddEmptyTrackCommand command2 (*collection_, second_track_ref);

  // Add first track
  command1.redo ();
  EXPECT_TRUE (collection_->contains (test_track_ref_.id ()));
  EXPECT_FALSE (collection_->contains (second_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  // Add second track
  command2.redo ();
  EXPECT_TRUE (collection_->contains (test_track_ref_.id ()));
  EXPECT_TRUE (collection_->contains (second_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 2);

  // Undo second track
  command2.undo ();
  EXPECT_TRUE (collection_->contains (test_track_ref_.id ()));
  EXPECT_FALSE (collection_->contains (second_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  // Undo first track
  command1.undo ();
  EXPECT_FALSE (collection_->contains (test_track_ref_.id ()));
  EXPECT_FALSE (collection_->contains (second_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 0);
}

} // namespace zrythm::commands
