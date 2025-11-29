// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/arranger_object_factory.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class ArrangerObjectFactoryTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));

    // Setup providers
    sample_rate_provider = [] () { return units::sample_rate (44100); };
    bpm_provider = [] () { return 120.0; };

    // Create factory
    factory = std::make_unique<ArrangerObjectFactory> (
      ArrangerObjectFactory::Dependencies{
        .tempo_map_ = *tempo_map,
        .object_registry_ = object_registry,
        .file_audio_source_registry_ = file_audio_source_registry,
        .musical_mode_getter_ = [] () { return true; },
        .last_timeline_obj_len_provider_ = [] () { return 100.0; },
        .last_editor_obj_len_provider_ = [] () { return 50.0; },
        .automation_curve_algorithm_provider_ =
          [] () { return dsp::CurveOptions::Algorithm::Exponent; } },
      sample_rate_provider, bpm_provider);
  }

  std::unique_ptr<dsp::TempoMap>            tempo_map;
  ArrangerObjectFactory::SampleRateProvider sample_rate_provider;
  ArrangerObjectFactory::BpmProvider        bpm_provider;
  ArrangerObjectRegistry                    object_registry;
  dsp::FileAudioSourceRegistry              file_audio_source_registry;
  std::unique_ptr<ArrangerObjectFactory>    factory;
};

// Test basic object creation for different types
TEST_F (ArrangerObjectFactoryTest, CreateBasicObjects)
{
  // Test MidiNote creation
  auto   midi_note_builder = factory->get_builder<MidiNote> ();
  auto   midi_note_ref = midi_note_builder.build_in_registry ();
  auto * midi_note = midi_note_ref.get_object_as<MidiNote> ();
  EXPECT_NE (midi_note, nullptr);
  EXPECT_EQ (midi_note->type (), ArrangerObject::Type::MidiNote);
  EXPECT_TRUE (object_registry.contains (midi_note->get_uuid ()));

  // Test Marker creation
  auto   marker_builder = factory->get_builder<Marker> ();
  auto   marker_ref = marker_builder.build_in_registry ();
  auto * marker = marker_ref.get_object_as<Marker> ();
  EXPECT_NE (marker, nullptr);
  EXPECT_EQ (marker->type (), ArrangerObject::Type::Marker);
  EXPECT_TRUE (object_registry.contains (marker->get_uuid ()));

  // Test ChordObject creation
  auto   chord_object_builder = factory->get_builder<ChordObject> ();
  auto   chord_object_ref = chord_object_builder.build_in_registry ();
  auto * chord_object = chord_object_ref.get_object_as<ChordObject> ();
  EXPECT_NE (chord_object, nullptr);
  EXPECT_EQ (chord_object->type (), ArrangerObject::Type::ChordObject);
  EXPECT_TRUE (object_registry.contains (chord_object->get_uuid ()));

  // Test AutomationPoint creation
  auto   automation_point_builder = factory->get_builder<AutomationPoint> ();
  auto   automation_point_ref = automation_point_builder.build_in_registry ();
  auto * automation_point =
    automation_point_ref.get_object_as<AutomationPoint> ();
  EXPECT_NE (automation_point, nullptr);
  EXPECT_EQ (automation_point->type (), ArrangerObject::Type::AutomationPoint);
  EXPECT_TRUE (object_registry.contains (automation_point->get_uuid ()));
}

// Test builder pattern with various configurations
TEST_F (ArrangerObjectFactoryTest, BuilderPatternConfiguration)
{
  // Test MidiNote with pitch and velocity
  const int    test_pitch = 60;
  const int    test_velocity = 90;
  const double start_ticks = 100.0;
  const double end_ticks = 200.0;

  auto midi_note_ref =
    factory->get_builder<MidiNote> ()
      .with_start_ticks (start_ticks)
      .with_end_ticks (end_ticks)
      .with_pitch (test_pitch)
      .with_velocity (test_velocity)
      .build_in_registry ();

  auto * midi_note = midi_note_ref.get_object_as<MidiNote> ();
  EXPECT_EQ (midi_note->pitch (), test_pitch);
  EXPECT_EQ (midi_note->velocity (), test_velocity);
  EXPECT_EQ (midi_note->position ()->ticks (), start_ticks);
  EXPECT_EQ (midi_note->bounds ()->length ()->ticks (), end_ticks - start_ticks);

  // Test Marker with name and type
  const QString            marker_name = "Test Marker";
  const Marker::MarkerType marker_type = Marker::MarkerType::Custom;
  const double             marker_start_ticks = 50.0;

  auto marker_ref =
    factory->get_builder<Marker> ()
      .with_start_ticks (marker_start_ticks)
      .with_name (marker_name)
      .with_marker_type (marker_type)
      .build_in_registry ();

  auto * marker = marker_ref.get_object_as<Marker> ();
  EXPECT_EQ (marker->name ()->name (), marker_name);
  EXPECT_EQ (marker->markerType (), marker_type);
  EXPECT_EQ (marker->position ()->ticks (), marker_start_ticks);

  // Test AutomationPoint with value
  const double automation_value = 0.75;
  const double automation_start_ticks = 75.0;

  auto automation_point_ref =
    factory->get_builder<AutomationPoint> ()
      .with_start_ticks (automation_start_ticks)
      .with_automatable_value (automation_value)
      .build_in_registry ();

  auto * automation_point =
    automation_point_ref.get_object_as<AutomationPoint> ();
  EXPECT_DOUBLE_EQ (automation_point->value (), automation_value);
  EXPECT_EQ (automation_point->position ()->ticks (), automation_start_ticks);
}

// Test object registration in registry
TEST_F (ArrangerObjectFactoryTest, ObjectRegistration)
{
  // Create multiple objects and verify they're all registered
  const int                                num_objects = 5;
  std::vector<ArrangerObjectUuidReference> object_refs;

  for (int i = 0; i < num_objects; ++i)
    {
      auto ref =
        factory->get_builder<MidiNote> ()
          .with_start_ticks (i * 100.0)
          .with_pitch (60 + i)
          .build_in_registry ();
      object_refs.push_back (ref);
    }

  // Verify all objects are registered
  EXPECT_EQ (object_registry.size (), num_objects);

  for (const auto &ref : object_refs)
    {
      EXPECT_TRUE (object_registry.contains (ref.id ()));

      // Verify we can retrieve the object
      auto obj_var = object_registry.find_by_id (ref.id ());
      EXPECT_TRUE (obj_var.has_value ());

      // Verify it's the correct type
      std::visit (
        [&] (auto * obj) {
          EXPECT_NE (obj, nullptr);
          EXPECT_EQ (obj->type (), ArrangerObject::Type::MidiNote);
        },
        obj_var.value ());
    }
}

// Test factory convenience methods
TEST_F (ArrangerObjectFactoryTest, ConvenienceMethods)
{
  // Test create_audio_region_with_clip
  // First create a dummy audio source
  auto sample_buffer = std::make_unique<utils::audio::AudioBuffer> (2, 1024);
  auto source_ref = file_audio_source_registry.create_object<
    dsp::FileAudioSource> (
    *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
    units::sample_rate (44100), 120.0, u8"TestSource");

  const double start_ticks = 100.0;
  auto         audio_region_ref =
    factory->create_audio_region_with_clip (source_ref, start_ticks);

  auto * audio_region = audio_region_ref.get_object_as<AudioRegion> ();
  EXPECT_NE (audio_region, nullptr);
  EXPECT_EQ (audio_region->type (), ArrangerObject::Type::AudioRegion);
  EXPECT_EQ (audio_region->position ()->ticks (), start_ticks);
  EXPECT_TRUE (object_registry.contains (audio_region->get_uuid ()));

  // Test create_editor_object for MidiNote
  const double editor_start_ticks = 150.0;
  const int    editor_pitch = 64;

  auto midi_note_ref = factory->create_editor_object<MidiRegion> (
    editor_start_ticks, editor_pitch);

  auto * midi_note = midi_note_ref.get_object_as<MidiNote> ();
  EXPECT_NE (midi_note, nullptr);
  EXPECT_EQ (midi_note->pitch (), editor_pitch);
  EXPECT_EQ (midi_note->position ()->ticks (), editor_start_ticks);
  EXPECT_TRUE (object_registry.contains (midi_note->get_uuid ()));

  // Test create_editor_object for AutomationPoint
  const double automation_start_ticks = 200.0;
  const double automation_value = 0.5;

  auto automation_point_ref = factory->create_editor_object<AutomationRegion> (
    automation_start_ticks, automation_value);

  auto * automation_point =
    automation_point_ref.get_object_as<AutomationPoint> ();
  EXPECT_NE (automation_point, nullptr);
  EXPECT_DOUBLE_EQ (automation_point->value (), automation_value);
  EXPECT_EQ (automation_point->position ()->ticks (), automation_start_ticks);
  EXPECT_TRUE (object_registry.contains (automation_point->get_uuid ()));
}

// Test build_empty method
TEST_F (ArrangerObjectFactoryTest, BuildEmptyObjects)
{
  // Test building empty objects without registration
  auto empty_midi_note = factory->get_builder<MidiNote> ().build_empty ();
  EXPECT_NE (empty_midi_note, nullptr);
  EXPECT_EQ (empty_midi_note->type (), ArrangerObject::Type::MidiNote);
  EXPECT_FALSE (object_registry.contains (empty_midi_note->get_uuid ()));

  auto empty_marker = factory->get_builder<Marker> ().build_empty ();
  EXPECT_NE (empty_marker, nullptr);
  EXPECT_EQ (empty_marker->type (), ArrangerObject::Type::Marker);
  EXPECT_FALSE (object_registry.contains (empty_marker->get_uuid ()));

  auto empty_audio_region = factory->get_builder<AudioRegion> ().build_empty ();
  EXPECT_NE (empty_audio_region, nullptr);
  EXPECT_EQ (empty_audio_region->type (), ArrangerObject::Type::AudioRegion);
  EXPECT_FALSE (object_registry.contains (empty_audio_region->get_uuid ()));
}

// Test clone_new_object_identity for different object types
TEST_F (ArrangerObjectFactoryTest, CloneNewObjectIdentity)
{
  // Test cloning MidiNote
  auto original_midi_note_ref =
    factory->get_builder<MidiNote> ()
      .with_start_ticks (100.0)
      .with_end_ticks (500.0)
      .with_pitch (60)
      .with_velocity (80)
      .build_in_registry ();

  auto * original_midi_note = original_midi_note_ref.get_object_as<MidiNote> ();
  EXPECT_NE (original_midi_note, nullptr);
  const auto original_midi_note_uuid = original_midi_note->get_uuid ();

  // Clone the midi note
  auto cloned_midi_note_ref =
    factory->clone_new_object_identity (*original_midi_note);
  auto * cloned_midi_note = cloned_midi_note_ref.get_object_as<MidiNote> ();
  EXPECT_NE (cloned_midi_note, nullptr);
  const auto cloned_midi_note_uuid = cloned_midi_note->get_uuid ();

  // Verify cloned object has same properties but different UUID
  EXPECT_NE (original_midi_note_uuid, cloned_midi_note_uuid);
  EXPECT_DOUBLE_EQ (
    cloned_midi_note->position ()->ticks (),
    original_midi_note->position ()->ticks ());
  EXPECT_DOUBLE_EQ (
    cloned_midi_note->bounds ()->length ()->ticks (),
    original_midi_note->bounds ()->length ()->ticks ());
  EXPECT_EQ (cloned_midi_note->pitch (), original_midi_note->pitch ());
  EXPECT_EQ (cloned_midi_note->velocity (), original_midi_note->velocity ());
  EXPECT_TRUE (object_registry.contains (cloned_midi_note_uuid));

  // Test cloning Marker
  auto original_marker_ref =
    factory->get_builder<Marker> ()
      .with_start_ticks (200.0)
      .with_name ("Test Marker")
      .with_marker_type (Marker::MarkerType::Custom)
      .build_in_registry ();

  auto * original_marker = original_marker_ref.get_object_as<Marker> ();
  EXPECT_NE (original_marker, nullptr);
  const auto original_marker_uuid = original_marker->get_uuid ();

  // Clone the marker
  auto cloned_marker_ref = factory->clone_new_object_identity (*original_marker);
  auto * cloned_marker = cloned_marker_ref.get_object_as<Marker> ();
  EXPECT_NE (cloned_marker, nullptr);
  const auto cloned_marker_uuid = cloned_marker->get_uuid ();

  // Verify cloned object has same properties but different UUID
  EXPECT_NE (original_marker_uuid, cloned_marker_uuid);
  EXPECT_DOUBLE_EQ (
    cloned_marker->position ()->ticks (),
    original_marker->position ()->ticks ());
  EXPECT_EQ (cloned_marker->name ()->name (), original_marker->name ()->name ());
  EXPECT_EQ (cloned_marker->markerType (), original_marker->markerType ());
  EXPECT_TRUE (object_registry.contains (cloned_marker_uuid));

  // Test cloning AutomationPoint
  auto original_automation_point_ref =
    factory->get_builder<AutomationPoint> ()
      .with_start_ticks (300.0)
      .with_automatable_value (0.75)
      .build_in_registry ();

  auto * original_automation_point =
    original_automation_point_ref.get_object_as<AutomationPoint> ();
  EXPECT_NE (original_automation_point, nullptr);
  const auto original_automation_point_uuid =
    original_automation_point->get_uuid ();

  // Clone the automation point
  auto cloned_automation_point_ref =
    factory->clone_new_object_identity (*original_automation_point);
  auto * cloned_automation_point =
    cloned_automation_point_ref.get_object_as<AutomationPoint> ();
  EXPECT_NE (cloned_automation_point, nullptr);
  const auto cloned_automation_point_uuid = cloned_automation_point->get_uuid ();

  // Verify cloned object has same properties but different UUID
  EXPECT_NE (original_automation_point_uuid, cloned_automation_point_uuid);
  EXPECT_DOUBLE_EQ (
    cloned_automation_point->position ()->ticks (),
    original_automation_point->position ()->ticks ());
  EXPECT_FLOAT_EQ (
    cloned_automation_point->value (), original_automation_point->value ());
  EXPECT_TRUE (object_registry.contains (cloned_automation_point_uuid));
}

// Test clone_new_object_identity for AudioRegion using public API
TEST_F (ArrangerObjectFactoryTest, CloneNewObjectIdentityAudioRegion)
{
  // First create a dummy audio source
  auto sample_buffer = std::make_unique<utils::audio::AudioBuffer> (2, 1024);
  auto source_ref = file_audio_source_registry.create_object<
    dsp::FileAudioSource> (
    *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
    units::sample_rate (44100), 120.0, u8"TestSource");

  // Create audio region
  auto original_audio_region_ref =
    factory->create_audio_region_with_clip (source_ref, 1000.0);
  auto * original_audio_region =
    original_audio_region_ref.get_object_as<AudioRegion> ();
  EXPECT_NE (original_audio_region, nullptr);
  const auto original_audio_region_uuid = original_audio_region->get_uuid ();

  // Clone the audio region
  auto cloned_audio_region_ref =
    factory->clone_new_object_identity (*original_audio_region);
  auto * cloned_audio_region =
    cloned_audio_region_ref.get_object_as<AudioRegion> ();
  EXPECT_NE (cloned_audio_region, nullptr);
  const auto cloned_audio_region_uuid = cloned_audio_region->get_uuid ();

  // Verify cloned object has same properties but different UUID
  EXPECT_NE (original_audio_region_uuid, cloned_audio_region_uuid);
  EXPECT_DOUBLE_EQ (
    cloned_audio_region->position ()->ticks (),
    original_audio_region->position ()->ticks ());
  EXPECT_DOUBLE_EQ (
    cloned_audio_region->bounds ()->length ()->ticks (),
    original_audio_region->bounds ()->length ()->ticks ());
  EXPECT_TRUE (object_registry.contains (cloned_audio_region_uuid));
}

// Test clone_new_object_identity for MidiRegion
TEST_F (ArrangerObjectFactoryTest, CloneNewObjectIdentityMidiRegion)
{
  auto original_midi_region_ref =
    factory->get_builder<MidiRegion> ()
      .with_start_ticks (500.0)
      .with_end_ticks (1500.0)
      .build_in_registry ();

  auto * original_midi_region =
    original_midi_region_ref.get_object_as<MidiRegion> ();
  EXPECT_NE (original_midi_region, nullptr);
  const auto original_midi_region_uuid = original_midi_region->get_uuid ();

  // Clone the midi region
  auto cloned_midi_region_ref =
    factory->clone_new_object_identity (*original_midi_region);
  auto * cloned_midi_region =
    cloned_midi_region_ref.get_object_as<MidiRegion> ();
  EXPECT_NE (cloned_midi_region, nullptr);
  const auto cloned_midi_region_uuid = cloned_midi_region->get_uuid ();

  // Verify cloned object has same properties but different UUID
  EXPECT_NE (original_midi_region_uuid, cloned_midi_region_uuid);
  EXPECT_DOUBLE_EQ (
    cloned_midi_region->position ()->ticks (),
    original_midi_region->position ()->ticks ());
  EXPECT_DOUBLE_EQ (
    cloned_midi_region->bounds ()->length ()->ticks (),
    original_midi_region->bounds ()->length ()->ticks ());
  EXPECT_DOUBLE_EQ (
    cloned_midi_region->loopRange ()->clipStartPosition ()->ticks (),
    original_midi_region->loopRange ()->clipStartPosition ()->ticks ());
  EXPECT_DOUBLE_EQ (
    cloned_midi_region->loopRange ()->loopStartPosition ()->ticks (),
    original_midi_region->loopRange ()->loopStartPosition ()->ticks ());
  EXPECT_DOUBLE_EQ (
    cloned_midi_region->loopRange ()->loopEndPosition ()->ticks (),
    original_midi_region->loopRange ()->loopEndPosition ()->ticks ());
  EXPECT_TRUE (object_registry.contains (cloned_midi_region_uuid));
}

// Test clone_new_object_identity for AudioSourceObject
TEST_F (ArrangerObjectFactoryTest, CloneNewObjectIdentityAudioSourceObject)
{
  // First create a dummy audio source
  auto sample_buffer = std::make_unique<utils::audio::AudioBuffer> (2, 1024);
  auto source_ref = file_audio_source_registry.create_object<
    dsp::FileAudioSource> (
    *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32,
    units::sample_rate (44100), 120.0, u8"TestSource");

  auto original_audio_source_obj_ref =
    factory->get_builder<AudioSourceObject> ().build_in_registry ();

  auto * original_audio_source_obj =
    original_audio_source_obj_ref.get_object_as<AudioSourceObject> ();
  EXPECT_NE (original_audio_source_obj, nullptr);
  const auto original_audio_source_obj_uuid =
    original_audio_source_obj->get_uuid ();

  // Clone the audio source object
  auto cloned_audio_source_obj_ref =
    factory->clone_new_object_identity (*original_audio_source_obj);
  auto * cloned_audio_source_obj =
    cloned_audio_source_obj_ref.get_object_as<AudioSourceObject> ();
  EXPECT_NE (cloned_audio_source_obj, nullptr);
  const auto cloned_audio_source_obj_uuid = cloned_audio_source_obj->get_uuid ();

  // Verify cloned object has different UUID
  EXPECT_NE (original_audio_source_obj_uuid, cloned_audio_source_obj_uuid);
  EXPECT_TRUE (object_registry.contains (cloned_audio_source_obj_uuid));

  // Verify that the audio source reference is preserved (should have same
  // reference)
  const auto original_source_ref =
    original_audio_source_obj->audio_source_ref ();
  const auto cloned_source_ref = cloned_audio_source_obj->audio_source_ref ();
  EXPECT_EQ (original_source_ref.id (), cloned_source_ref.id ());
}

// Test clone_new_object_identity for TempoObject
TEST_F (ArrangerObjectFactoryTest, CloneNewObjectIdentityTempoObject)
{
  auto original_tempo_obj_ref =
    factory->get_builder<TempoObject> ().build_in_registry ();

  auto * original_tempo_obj =
    original_tempo_obj_ref.get_object_as<TempoObject> ();
  EXPECT_NE (original_tempo_obj, nullptr);
  const auto original_tempo_obj_uuid = original_tempo_obj->get_uuid ();

  // Clone the tempo object
  auto cloned_tempo_obj_ref =
    factory->clone_new_object_identity (*original_tempo_obj);
  auto * cloned_tempo_obj = cloned_tempo_obj_ref.get_object_as<TempoObject> ();
  EXPECT_NE (cloned_tempo_obj, nullptr);
  const auto cloned_tempo_obj_uuid = cloned_tempo_obj->get_uuid ();

  // Verify cloned object has different UUID
  EXPECT_NE (original_tempo_obj_uuid, cloned_tempo_obj_uuid);
  EXPECT_TRUE (object_registry.contains (cloned_tempo_obj_uuid));
}

// Test clone_new_object_identity for TimeSignatureObject
TEST_F (ArrangerObjectFactoryTest, CloneNewObjectIdentityTimeSignatureObject)
{
  auto original_time_signature_obj_ref =
    factory->get_builder<TimeSignatureObject> ().build_in_registry ();

  auto * original_time_signature_obj =
    original_time_signature_obj_ref.get_object_as<TimeSignatureObject> ();
  EXPECT_NE (original_time_signature_obj, nullptr);
  const auto original_time_signature_obj_uuid =
    original_time_signature_obj->get_uuid ();

  // Clone the time signature object
  auto cloned_time_signature_obj_ref =
    factory->clone_new_object_identity (*original_time_signature_obj);
  auto * cloned_time_signature_obj =
    cloned_time_signature_obj_ref.get_object_as<TimeSignatureObject> ();
  EXPECT_NE (cloned_time_signature_obj, nullptr);
  const auto cloned_time_signature_obj_uuid =
    cloned_time_signature_obj->get_uuid ();

  // Verify cloned object has different UUID
  EXPECT_NE (original_time_signature_obj_uuid, cloned_time_signature_obj_uuid);
  EXPECT_TRUE (object_registry.contains (cloned_time_signature_obj_uuid));
}

} // namespace zrythm::structure::arrangement
