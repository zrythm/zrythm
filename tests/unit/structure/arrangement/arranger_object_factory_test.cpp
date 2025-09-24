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
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0 * mp_units::si::hertz);

    // Setup providers
    sample_rate_provider = [] () { return 44100.0; };
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
    *sample_buffer, utils::audio::BitDepth::BIT_DEPTH_32, 44100, 120.0,
    u8"TestSource");

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

} // namespace zrythm::structure::arrangement
