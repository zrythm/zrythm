// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/add_track_command.h"
#include "plugins/internal_plugin_base.h"
#include "structure/tracks/track_all.h"
#include "structure/tracks/track_collection.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{

class AddEmptyTrackCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    auto track_ref = utils::create_object<structure::tracks::AudioTrack> (
      registry_, dependencies_);
    test_track_ref_ = track_ref;

    collection_ =
      std::make_unique<structure::tracks::TrackCollection> (registry_);
  }

  dsp::TempoMap                  tempo_map_{ units::sample_rate (44100.0) };
  dsp::TempoMapWrapper           tempo_map_wrapper_{ tempo_map_ };
  dsp::graph_test::MockTransport transport_;
  structure::tracks::SoloedTracksExistGetter soloed_tracks_exist_getter_{ [] {
    return false;
  } };

  utils::ObjectRegistry registry_;

  structure::tracks::FinalTrackDependencies dependencies_{
    tempo_map_wrapper_,
    registry_,
    soloed_tracks_exist_getter_,
    {},
  };
  structure::tracks::TrackUuidReference test_track_ref_{ registry_ };
  std::unique_ptr<structure::tracks::TrackCollection> collection_;

  plugins::PluginUuidReference
  create_and_append_plugin_to_channel (structure::tracks::Track &track)
  {
    auto * channel = track.channel ();
    auto   ref = utils::create_object<plugins::InternalPluginBase> (
      registry_, registry_, nullptr);
    channel->inserts ()->append_plugin (ref);
    return ref;
  }
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
  auto midi_track_ref = utils::create_object<structure::tracks::MidiTrack> (
    registry_, dependencies_);

  AddEmptyTrackCommand midi_command (*collection_, midi_track_ref);

  midi_command.redo ();
  EXPECT_TRUE (collection_->contains (midi_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 1);

  midi_command.undo ();
  EXPECT_FALSE (collection_->contains (midi_track_ref.id ()));
  EXPECT_EQ (collection_->track_count (), 0);

  // Test with Instrument track
  auto instrument_track_ref = utils::create_object<
    structure::tracks::InstrumentTrack> (registry_, dependencies_);

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
  auto second_track_ref = utils::create_object<structure::tracks::AudioTrack> (
    registry_, dependencies_);

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

TEST_F (AddEmptyTrackCommandTest, UndoClosesPluginWindows)
{
  auto * track = test_track_ref_.get ();
  auto   plugin_ref = create_and_append_plugin_to_channel (*track);
  auto * plugin = plugin_ref.get ();
  plugin->setUiVisible (true);

  AddEmptyTrackCommand command (*collection_, test_track_ref_);
  command.redo ();
  ASSERT_TRUE (plugin->uiVisible ());

  command.undo ();
  EXPECT_FALSE (plugin->uiVisible ());
}

TEST_F (AddEmptyTrackCommandTest, UndoAfterReopenClosesPluginWindows)
{
  auto * track = test_track_ref_.get ();
  auto   plugin_ref = create_and_append_plugin_to_channel (*track);
  auto * plugin = plugin_ref.get ();
  plugin->setUiVisible (true);

  AddEmptyTrackCommand command (*collection_, test_track_ref_);
  command.redo ();
  ASSERT_TRUE (plugin->uiVisible ());

  command.undo ();
  EXPECT_FALSE (plugin->uiVisible ());

  command.redo ();
  plugin->setUiVisible (true);
  ASSERT_TRUE (plugin->uiVisible ());

  command.undo ();
  EXPECT_FALSE (plugin->uiVisible ());
}

} // namespace zrythm::commands
