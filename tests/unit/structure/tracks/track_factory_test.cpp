// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/tracks/track_factory.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class TrackFactoryTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create track registry
    track_registry = std::make_unique<TrackRegistry> ();

    // Create test dependencies
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0 * mp_units::si::hertz);
    tempo_map_wrapper = std::make_unique<dsp::TempoMapWrapper> (*tempo_map);

    // Create factory dependencies
    FinalTrackDependencies deps{
      *tempo_map_wrapper,   file_audio_source_registry,
      plugin_registry,      port_registry,
      param_registry,       obj_registry,
      *track_registry,      transport,
      [] { return false; },
    };

    // Create factory
    factory = std::make_unique<TrackFactory> (std::move (deps));
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper;
  dsp::PortRegistry                     port_registry;
  dsp::ProcessorParameterRegistry       param_registry{ port_registry };
  structure::arrangement::ArrangerObjectRegistry obj_registry;
  dsp::FileAudioSourceRegistry                   file_audio_source_registry;
  plugins::PluginRegistry                        plugin_registry;
  dsp::graph_test::MockTransport                 transport;
  std::unique_ptr<TrackRegistry>                 track_registry;
  std::unique_ptr<TrackFactory>                  factory;
};

// Test basic track creation for different types
TEST_F (TrackFactoryTest, CreateBasicTracks)
{
  // Test AudioTrack creation
  auto   audio_track_builder = factory->get_builder<AudioTrack> ();
  auto   audio_track_ref = audio_track_builder.build ();
  auto * audio_track = audio_track_ref.get_object_as<AudioTrack> ();
  EXPECT_NE (audio_track, nullptr);
  EXPECT_EQ (audio_track->type (), Track::Type::Audio);
  EXPECT_TRUE (track_registry->contains (audio_track->get_uuid ()));

  // Test MidiTrack creation
  auto   midi_track_builder = factory->get_builder<MidiTrack> ();
  auto   midi_track_ref = midi_track_builder.build ();
  auto * midi_track = midi_track_ref.get_object_as<MidiTrack> ();
  EXPECT_NE (midi_track, nullptr);
  EXPECT_EQ (midi_track->type (), Track::Type::Midi);
  EXPECT_TRUE (track_registry->contains (midi_track->get_uuid ()));

  // Test InstrumentTrack creation
  auto   instrument_track_builder = factory->get_builder<InstrumentTrack> ();
  auto   instrument_track_ref = instrument_track_builder.build ();
  auto * instrument_track =
    instrument_track_ref.get_object_as<InstrumentTrack> ();
  EXPECT_NE (instrument_track, nullptr);
  EXPECT_EQ (instrument_track->type (), Track::Type::Instrument);
  EXPECT_TRUE (track_registry->contains (instrument_track->get_uuid ()));

  // Test MasterTrack creation
  auto   master_track_builder = factory->get_builder<MasterTrack> ();
  auto   master_track_ref = master_track_builder.build ();
  auto * master_track = master_track_ref.get_object_as<MasterTrack> ();
  EXPECT_NE (master_track, nullptr);
  EXPECT_EQ (master_track->type (), Track::Type::Master);
  EXPECT_TRUE (track_registry->contains (master_track->get_uuid ()));

  // Test ChordTrack creation
  auto   chord_track_builder = factory->get_builder<ChordTrack> ();
  auto   chord_track_ref = chord_track_builder.build ();
  auto * chord_track = chord_track_ref.get_object_as<ChordTrack> ();
  EXPECT_NE (chord_track, nullptr);
  EXPECT_EQ (chord_track->type (), Track::Type::Chord);
  EXPECT_TRUE (track_registry->contains (chord_track->get_uuid ()));

  // Test MarkerTrack creation
  auto   marker_track_builder = factory->get_builder<MarkerTrack> ();
  auto   marker_track_ref = marker_track_builder.build ();
  auto * marker_track = marker_track_ref.get_object_as<MarkerTrack> ();
  EXPECT_NE (marker_track, nullptr);
  EXPECT_EQ (marker_track->type (), Track::Type::Marker);
  EXPECT_TRUE (track_registry->contains (marker_track->get_uuid ()));

  // Test ModulatorTrack creation
  auto   modulator_track_builder = factory->get_builder<ModulatorTrack> ();
  auto   modulator_track_ref = modulator_track_builder.build ();
  auto * modulator_track = modulator_track_ref.get_object_as<ModulatorTrack> ();
  EXPECT_NE (modulator_track, nullptr);
  EXPECT_EQ (modulator_track->type (), Track::Type::Modulator);
  EXPECT_TRUE (track_registry->contains (modulator_track->get_uuid ()));

  // Test AudioBusTrack creation
  auto   audio_bus_track_builder = factory->get_builder<AudioBusTrack> ();
  auto   audio_bus_track_ref = audio_bus_track_builder.build ();
  auto * audio_bus_track = audio_bus_track_ref.get_object_as<AudioBusTrack> ();
  EXPECT_NE (audio_bus_track, nullptr);
  EXPECT_EQ (audio_bus_track->type (), Track::Type::AudioBus);
  EXPECT_TRUE (track_registry->contains (audio_bus_track->get_uuid ()));

  // Test MidiBusTrack creation
  auto   midi_bus_track_builder = factory->get_builder<MidiBusTrack> ();
  auto   midi_bus_track_ref = midi_bus_track_builder.build ();
  auto * midi_bus_track = midi_bus_track_ref.get_object_as<MidiBusTrack> ();
  EXPECT_NE (midi_bus_track, nullptr);
  EXPECT_EQ (midi_bus_track->type (), Track::Type::MidiBus);
  EXPECT_TRUE (track_registry->contains (midi_bus_track->get_uuid ()));

  // Test AudioGroupTrack creation
  auto   audio_group_track_builder = factory->get_builder<AudioGroupTrack> ();
  auto   audio_group_track_ref = audio_group_track_builder.build ();
  auto * audio_group_track =
    audio_group_track_ref.get_object_as<AudioGroupTrack> ();
  EXPECT_NE (audio_group_track, nullptr);
  EXPECT_EQ (audio_group_track->type (), Track::Type::AudioGroup);
  EXPECT_TRUE (track_registry->contains (audio_group_track->get_uuid ()));

  // Test MidiGroupTrack creation
  auto   midi_group_track_builder = factory->get_builder<MidiGroupTrack> ();
  auto   midi_group_track_ref = midi_group_track_builder.build ();
  auto * midi_group_track =
    midi_group_track_ref.get_object_as<MidiGroupTrack> ();
  EXPECT_NE (midi_group_track, nullptr);
  EXPECT_EQ (midi_group_track->type (), Track::Type::MidiGroup);
  EXPECT_TRUE (track_registry->contains (midi_group_track->get_uuid ()));

  // Test FolderTrack creation
  auto   folder_track_builder = factory->get_builder<FolderTrack> ();
  auto   folder_track_ref = folder_track_builder.build ();
  auto * folder_track = folder_track_ref.get_object_as<FolderTrack> ();
  EXPECT_NE (folder_track, nullptr);
  EXPECT_EQ (folder_track->type (), Track::Type::Folder);
  EXPECT_TRUE (track_registry->contains (folder_track->get_uuid ()));
}

// Test builder pattern with build_for_deserialization
TEST_F (TrackFactoryTest, BuilderPatternDeserialization)
{
  // Test AudioTrack build_for_deserialization
  auto audio_track_builder = factory->get_builder<AudioTrack> ();
  auto audio_track = audio_track_builder.build_for_deserialization ();
  EXPECT_NE (audio_track, nullptr);
  EXPECT_EQ (audio_track->type (), Track::Type::Audio);
  // Should not be registered in registry for deserialization
  EXPECT_FALSE (track_registry->contains (audio_track->get_uuid ()));

  // Test MidiTrack build_for_deserialization
  auto midi_track_builder = factory->get_builder<MidiTrack> ();
  auto midi_track = midi_track_builder.build_for_deserialization ();
  EXPECT_NE (midi_track, nullptr);
  EXPECT_EQ (midi_track->type (), Track::Type::Midi);
  EXPECT_FALSE (track_registry->contains (midi_track->get_uuid ()));

  // Test InstrumentTrack build_for_deserialization
  auto instrument_track_builder = factory->get_builder<InstrumentTrack> ();
  auto instrument_track = instrument_track_builder.build_for_deserialization ();
  EXPECT_NE (instrument_track, nullptr);
  EXPECT_EQ (instrument_track->type (), Track::Type::Instrument);
  EXPECT_FALSE (track_registry->contains (instrument_track->get_uuid ()));
}

// Test factory convenience methods
TEST_F (TrackFactoryTest, ConvenienceMethods)
{
  // Test create_empty_track with template parameter
  auto   audio_track_ref = factory->create_empty_track<AudioTrack> ();
  auto * audio_track = audio_track_ref.get_object_as<AudioTrack> ();
  EXPECT_NE (audio_track, nullptr);
  EXPECT_EQ (audio_track->type (), Track::Type::Audio);
  EXPECT_TRUE (track_registry->contains (audio_track->get_uuid ()));

  auto   midi_track_ref = factory->create_empty_track<MidiTrack> ();
  auto * midi_track = midi_track_ref.get_object_as<MidiTrack> ();
  EXPECT_NE (midi_track, nullptr);
  EXPECT_EQ (midi_track->type (), Track::Type::Midi);
  EXPECT_TRUE (track_registry->contains (midi_track->get_uuid ()));

  // Test create_empty_track with enum type
  auto   audio_track_ref2 = factory->create_empty_track (Track::Type::Audio);
  auto * audio_track2 = audio_track_ref2.get_object_as<AudioTrack> ();
  EXPECT_NE (audio_track2, nullptr);
  EXPECT_EQ (audio_track2->type (), Track::Type::Audio);
  EXPECT_TRUE (track_registry->contains (audio_track2->get_uuid ()));

  auto   midi_track_ref2 = factory->create_empty_track (Track::Type::Midi);
  auto * midi_track2 = midi_track_ref2.get_object_as<MidiTrack> ();
  EXPECT_NE (midi_track2, nullptr);
  EXPECT_EQ (midi_track2->type (), Track::Type::Midi);
  EXPECT_TRUE (track_registry->contains (midi_track2->get_uuid ()));

  auto instrument_track_ref =
    factory->create_empty_track (Track::Type::Instrument);
  auto * instrument_track =
    instrument_track_ref.get_object_as<InstrumentTrack> ();
  EXPECT_NE (instrument_track, nullptr);
  EXPECT_EQ (instrument_track->type (), Track::Type::Instrument);
  EXPECT_TRUE (track_registry->contains (instrument_track->get_uuid ()));

  auto   master_track_ref = factory->create_empty_track (Track::Type::Master);
  auto * master_track = master_track_ref.get_object_as<MasterTrack> ();
  EXPECT_NE (master_track, nullptr);
  EXPECT_EQ (master_track->type (), Track::Type::Master);
  EXPECT_TRUE (track_registry->contains (master_track->get_uuid ()));

  auto   chord_track_ref = factory->create_empty_track (Track::Type::Chord);
  auto * chord_track = chord_track_ref.get_object_as<ChordTrack> ();
  EXPECT_NE (chord_track, nullptr);
  EXPECT_EQ (chord_track->type (), Track::Type::Chord);
  EXPECT_TRUE (track_registry->contains (chord_track->get_uuid ()));

  auto   marker_track_ref = factory->create_empty_track (Track::Type::Marker);
  auto * marker_track = marker_track_ref.get_object_as<MarkerTrack> ();
  EXPECT_NE (marker_track, nullptr);
  EXPECT_EQ (marker_track->type (), Track::Type::Marker);
  EXPECT_TRUE (track_registry->contains (marker_track->get_uuid ()));

  auto modulator_track_ref =
    factory->create_empty_track (Track::Type::Modulator);
  auto * modulator_track = modulator_track_ref.get_object_as<ModulatorTrack> ();
  EXPECT_NE (modulator_track, nullptr);
  EXPECT_EQ (modulator_track->type (), Track::Type::Modulator);
  EXPECT_TRUE (track_registry->contains (modulator_track->get_uuid ()));

  auto audio_bus_track_ref = factory->create_empty_track (Track::Type::AudioBus);
  auto * audio_bus_track = audio_bus_track_ref.get_object_as<AudioBusTrack> ();
  EXPECT_NE (audio_bus_track, nullptr);
  EXPECT_EQ (audio_bus_track->type (), Track::Type::AudioBus);
  EXPECT_TRUE (track_registry->contains (audio_bus_track->get_uuid ()));

  auto midi_bus_track_ref = factory->create_empty_track (Track::Type::MidiBus);
  auto * midi_bus_track = midi_bus_track_ref.get_object_as<MidiBusTrack> ();
  EXPECT_NE (midi_bus_track, nullptr);
  EXPECT_EQ (midi_bus_track->type (), Track::Type::MidiBus);
  EXPECT_TRUE (track_registry->contains (midi_bus_track->get_uuid ()));

  auto audio_group_track_ref =
    factory->create_empty_track (Track::Type::AudioGroup);
  auto * audio_group_track =
    audio_group_track_ref.get_object_as<AudioGroupTrack> ();
  EXPECT_NE (audio_group_track, nullptr);
  EXPECT_EQ (audio_group_track->type (), Track::Type::AudioGroup);
  EXPECT_TRUE (track_registry->contains (audio_group_track->get_uuid ()));

  auto midi_group_track_ref =
    factory->create_empty_track (Track::Type::MidiGroup);
  auto * midi_group_track =
    midi_group_track_ref.get_object_as<MidiGroupTrack> ();
  EXPECT_NE (midi_group_track, nullptr);
  EXPECT_EQ (midi_group_track->type (), Track::Type::MidiGroup);
  EXPECT_TRUE (track_registry->contains (midi_group_track->get_uuid ()));

  auto   folder_track_ref = factory->create_empty_track (Track::Type::Folder);
  auto * folder_track = folder_track_ref.get_object_as<FolderTrack> ();
  EXPECT_NE (folder_track, nullptr);
  EXPECT_EQ (folder_track->type (), Track::Type::Folder);
  EXPECT_TRUE (track_registry->contains (folder_track->get_uuid ()));
}

// Test object registration in registry
TEST_F (TrackFactoryTest, ObjectRegistration)
{
  // Create multiple tracks and verify they're all registered
  const int                       num_tracks = 5;
  std::vector<TrackUuidReference> track_refs;

  for (int i = 0; i < num_tracks; ++i)
    {
      auto ref = factory->create_empty_track<AudioTrack> ();
      track_refs.push_back (ref);
    }

  // Verify all tracks are registered
  EXPECT_EQ (track_registry->size (), num_tracks);

  for (const auto &ref : track_refs)
    {
      EXPECT_TRUE (track_registry->contains (ref.id ()));

      // Verify we can retrieve the track
      auto track_var = track_registry->find_by_id (ref.id ());
      EXPECT_TRUE (track_var.has_value ());

      // Verify it's the correct type
      std::visit (
        [&] (auto * track) {
          EXPECT_NE (track, nullptr);
          EXPECT_EQ (track->type (), Track::Type::Audio);
        },
        track_var.value ());
    }
}

// Test that newly created tracks have non-empty names
TEST_F (TrackFactoryTest, TrackNamesAreNonEmpty)
{
  // Test all track types to ensure they have non-empty names when created
  auto test_track_has_non_empty_name = [&] (Track::Type type) {
    auto track_ref = factory->create_empty_track (type);

    // Use visit to handle the variant and get the track
    std::visit (
      [&] (auto * track) {
        EXPECT_NE (track, nullptr);

        // Track should have a non-empty name
        auto name = track->get_name ();
        EXPECT_FALSE (name.empty ());
        EXPECT_FALSE (name.str ().empty ());
      },
      track_ref.get_object ());
  };

  // Test all track types
  test_track_has_non_empty_name (Track::Type::Audio);
  test_track_has_non_empty_name (Track::Type::Midi);
  test_track_has_non_empty_name (Track::Type::Instrument);
  test_track_has_non_empty_name (Track::Type::Master);
  test_track_has_non_empty_name (Track::Type::Chord);
  test_track_has_non_empty_name (Track::Type::Marker);
  test_track_has_non_empty_name (Track::Type::Modulator);
  test_track_has_non_empty_name (Track::Type::AudioBus);
  test_track_has_non_empty_name (Track::Type::MidiBus);
  test_track_has_non_empty_name (Track::Type::AudioGroup);
  test_track_has_non_empty_name (Track::Type::MidiGroup);
  test_track_has_non_empty_name (Track::Type::Folder);
}

// Test that tracks created via builder have non-empty names
TEST_F (TrackFactoryTest, BuilderTrackNamesAreNonEmpty)
{
  // Test all track types via builder
  auto test_builder_track_has_non_empty_name = [&] (auto builder) {
    auto track_ref = builder.build ();

    // Use visit to handle the variant and get the track
    std::visit (
      [&] (auto * track) {
        EXPECT_NE (track, nullptr);

        // Track should have a non-empty name
        auto name = track->get_name ();
        EXPECT_FALSE (name.empty ());
        EXPECT_FALSE (name.str ().empty ());
      },
      track_ref.get_object ());
  };

  // Test all track types via builder
  test_builder_track_has_non_empty_name (factory->get_builder<AudioTrack> ());
  test_builder_track_has_non_empty_name (factory->get_builder<MidiTrack> ());
  test_builder_track_has_non_empty_name (
    factory->get_builder<InstrumentTrack> ());
  test_builder_track_has_non_empty_name (factory->get_builder<MasterTrack> ());
  test_builder_track_has_non_empty_name (factory->get_builder<ChordTrack> ());
  test_builder_track_has_non_empty_name (factory->get_builder<MarkerTrack> ());
  test_builder_track_has_non_empty_name (
    factory->get_builder<ModulatorTrack> ());
  test_builder_track_has_non_empty_name (factory->get_builder<AudioBusTrack> ());
  test_builder_track_has_non_empty_name (factory->get_builder<MidiBusTrack> ());
  test_builder_track_has_non_empty_name (
    factory->get_builder<AudioGroupTrack> ());
  test_builder_track_has_non_empty_name (
    factory->get_builder<MidiGroupTrack> ());
  test_builder_track_has_non_empty_name (factory->get_builder<FolderTrack> ());
}

} // namespace zrythm::structure::tracks
