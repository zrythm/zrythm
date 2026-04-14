// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/delete_tracks_command.h"
#include "structure/tracks/folder_track.h"
#include "structure/tracks/track.h"
#include "structure/tracks/track_collection.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{

class DeleteTracksCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    collection_ =
      std::make_unique<structure::tracks::TrackCollection> (track_registry_);
  }

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
  std::unique_ptr<structure::tracks::TrackCollection> collection_;
};

TEST_F (DeleteTracksCommandTest, DeleteSingleTrack)
{
  auto track_ref =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);
  collection_->add_track (track_ref);
  ASSERT_EQ (collection_->track_count (), 1);

  DeleteTracksCommand cmd (*collection_, { track_ref });
  cmd.redo ();

  EXPECT_EQ (collection_->track_count (), 0);
  EXPECT_FALSE (collection_->contains (track_ref.id ()));

  cmd.undo ();

  EXPECT_EQ (collection_->track_count (), 1);
  EXPECT_TRUE (collection_->contains (track_ref.id ()));
}

TEST_F (DeleteTracksCommandTest, DeleteMultipleTracks)
{
  auto track1 = track_registry_.create_object<structure::tracks::AudioTrack> (
    dependencies_);
  auto track2 =
    track_registry_.create_object<structure::tracks::MidiTrack> (dependencies_);
  auto track3 = track_registry_.create_object<structure::tracks::AudioTrack> (
    dependencies_);

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);
  ASSERT_EQ (collection_->track_count (), 3);

  DeleteTracksCommand cmd (*collection_, { track1, track3 });
  cmd.redo ();

  EXPECT_EQ (collection_->track_count (), 1);
  EXPECT_FALSE (collection_->contains (track1.id ()));
  EXPECT_TRUE (collection_->contains (track2.id ()));
  EXPECT_FALSE (collection_->contains (track3.id ()));

  cmd.undo ();

  EXPECT_EQ (collection_->track_count (), 3);
  EXPECT_TRUE (collection_->contains (track1.id ()));
  EXPECT_TRUE (collection_->contains (track2.id ()));
  EXPECT_TRUE (collection_->contains (track3.id ()));
}

TEST_F (DeleteTracksCommandTest, RestoresOriginalPositions)
{
  auto track1 = track_registry_.create_object<structure::tracks::AudioTrack> (
    dependencies_);
  auto track2 =
    track_registry_.create_object<structure::tracks::MidiTrack> (dependencies_);
  auto track3 = track_registry_.create_object<structure::tracks::AudioTrack> (
    dependencies_);

  collection_->add_track (track1);
  collection_->add_track (track2);
  collection_->add_track (track3);

  DeleteTracksCommand cmd (*collection_, { track2 });
  cmd.redo ();
  ASSERT_EQ (collection_->track_count (), 2);

  cmd.undo ();
  ASSERT_EQ (collection_->track_count (), 3);

  auto idx1 = collection_->get_track_index (track1.id ());
  auto idx2 = collection_->get_track_index (track2.id ());
  auto idx3 = collection_->get_track_index (track3.id ());
  EXPECT_LT (idx1, idx2);
  EXPECT_LT (idx2, idx3);
}

TEST_F (DeleteTracksCommandTest, MultipleUndoRedoCycles)
{
  auto track_ref =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);
  collection_->add_track (track_ref);

  DeleteTracksCommand cmd (*collection_, { track_ref });

  for (int i = 0; i < 3; ++i)
    {
      cmd.redo ();
      EXPECT_EQ (collection_->track_count (), 0);

      cmd.undo ();
      EXPECT_EQ (collection_->track_count (), 1);
      EXPECT_TRUE (collection_->contains (track_ref.id ()));
    }
}

TEST_F (DeleteTracksCommandTest, CommandText)
{
  auto track_ref =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);
  DeleteTracksCommand cmd (*collection_, { track_ref });
  EXPECT_EQ (cmd.text (), QString ("Delete Track"));
}

TEST_F (DeleteTracksCommandTest, CommandTextPlural)
{
  auto track1 = track_registry_.create_object<structure::tracks::AudioTrack> (
    dependencies_);
  auto track2 =
    track_registry_.create_object<structure::tracks::MidiTrack> (dependencies_);
  collection_->add_track (track1);
  collection_->add_track (track2);

  DeleteTracksCommand cmd (*collection_, { track1, track2 });
  EXPECT_EQ (cmd.text (), QString ("Delete Tracks"));
}

TEST_F (DeleteTracksCommandTest, DeleteFolderWithChildren)
{
  // Set up: folder -> [audio_child, midi_child]
  auto folder = track_registry_.create_object<structure::tracks::FolderTrack> (
    dependencies_);
  auto audio_child =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);
  auto midi_child =
    track_registry_.create_object<structure::tracks::MidiTrack> (dependencies_);
  auto unrelated =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);

  collection_->add_track (folder);
  collection_->add_track (audio_child);
  collection_->add_track (midi_child);
  collection_->add_track (unrelated);

  collection_->set_folder_parent (audio_child.id (), folder.id ());
  collection_->set_folder_parent (midi_child.id (), folder.id ());

  ASSERT_EQ (collection_->track_count (), 4);

  // Simulate the operator's expansion: pass folder + its descendants
  auto descendants = collection_->get_all_descendants (folder.id ());
  ASSERT_EQ (descendants.size (), 2u);

  std::vector<structure::tracks::TrackUuidReference> expanded_refs;
  expanded_refs.push_back (folder);
  for (const auto &desc_id : descendants)
    expanded_refs.emplace_back (desc_id, track_registry_);

  DeleteTracksCommand cmd (*collection_, std::move (expanded_refs));
  cmd.redo ();

  // Only the unrelated track remains
  EXPECT_EQ (collection_->track_count (), 1);
  EXPECT_TRUE (collection_->contains (unrelated.id ()));
  EXPECT_FALSE (collection_->contains (folder.id ()));
  EXPECT_FALSE (collection_->contains (audio_child.id ()));
  EXPECT_FALSE (collection_->contains (midi_child.id ()));

  cmd.undo ();

  // All tracks restored with original positions and folder parents
  EXPECT_EQ (collection_->track_count (), 4);
  EXPECT_TRUE (collection_->contains (folder.id ()));
  EXPECT_TRUE (collection_->contains (audio_child.id ()));
  EXPECT_TRUE (collection_->contains (midi_child.id ()));
  EXPECT_TRUE (collection_->contains (unrelated.id ()));

  // Verify positions are restored: folder at 0, children after it
  auto folder_idx = collection_->get_track_index (folder.id ());
  auto audio_idx = collection_->get_track_index (audio_child.id ());
  auto midi_idx = collection_->get_track_index (midi_child.id ());
  auto unrelated_idx = collection_->get_track_index (unrelated.id ());
  EXPECT_LT (folder_idx, audio_idx);
  EXPECT_LT (audio_idx, midi_idx);
  EXPECT_LT (midi_idx, unrelated_idx);

  // Verify folder parent relationships are restored
  EXPECT_EQ (collection_->get_folder_parent (audio_child.id ()), folder.id ());
  EXPECT_EQ (collection_->get_folder_parent (midi_child.id ()), folder.id ());
}

TEST_F (DeleteTracksCommandTest, DeleteNestedFolders)
{
  // Set up: outer_folder -> inner_folder -> [audio_child], plus unrelated
  auto outer_folder = track_registry_.create_object<
    structure::tracks::FolderTrack> (dependencies_);
  auto inner_folder = track_registry_.create_object<
    structure::tracks::FolderTrack> (dependencies_);
  auto audio_child =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);
  auto unrelated =
    track_registry_.create_object<structure::tracks::AudioTrack> (dependencies_);

  collection_->add_track (outer_folder);
  collection_->add_track (inner_folder);
  collection_->add_track (audio_child);
  collection_->add_track (unrelated);

  collection_->set_folder_parent (inner_folder.id (), outer_folder.id ());
  collection_->set_folder_parent (audio_child.id (), inner_folder.id ());

  ASSERT_EQ (collection_->track_count (), 4);

  // Expand: outer_folder -> inner_folder -> audio_child
  std::vector<structure::tracks::TrackUuidReference> expanded_refs;
  expanded_refs.push_back (outer_folder);
  for (
    const auto &desc_id : collection_->get_all_descendants (outer_folder.id ()))
    expanded_refs.emplace_back (desc_id, track_registry_);

  ASSERT_EQ (expanded_refs.size (), 3u);

  DeleteTracksCommand cmd (*collection_, std::move (expanded_refs));
  cmd.redo ();

  EXPECT_EQ (collection_->track_count (), 1);
  EXPECT_TRUE (collection_->contains (unrelated.id ()));
  EXPECT_FALSE (collection_->contains (outer_folder.id ()));
  EXPECT_FALSE (collection_->contains (inner_folder.id ()));
  EXPECT_FALSE (collection_->contains (audio_child.id ()));

  cmd.undo ();

  EXPECT_EQ (collection_->track_count (), 4);
  EXPECT_TRUE (collection_->contains (outer_folder.id ()));
  EXPECT_TRUE (collection_->contains (inner_folder.id ()));
  EXPECT_TRUE (collection_->contains (audio_child.id ()));
  EXPECT_TRUE (collection_->contains (unrelated.id ()));

  // Verify positions restored
  EXPECT_LT (
    collection_->get_track_index (outer_folder.id ()),
    collection_->get_track_index (inner_folder.id ()));
  EXPECT_LT (
    collection_->get_track_index (inner_folder.id ()),
    collection_->get_track_index (audio_child.id ()));
  EXPECT_LT (
    collection_->get_track_index (audio_child.id ()),
    collection_->get_track_index (unrelated.id ()));

  // Verify folder parent relationships restored
  EXPECT_EQ (
    collection_->get_folder_parent (inner_folder.id ()), outer_folder.id ());
  EXPECT_EQ (
    collection_->get_folder_parent (audio_child.id ()), inner_folder.id ());
}

TEST_F (DeleteTracksCommandTest, DeleteEmptyRefsIsNoop)
{
  DeleteTracksCommand cmd (*collection_, {});
  EXPECT_EQ (cmd.text (), QString ("Delete Tracks"));

  cmd.redo ();
  EXPECT_EQ (collection_->track_count (), 0);

  cmd.undo ();
  EXPECT_EQ (collection_->track_count (), 0);
}

} // namespace zrythm::commands
