// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "project_json_serializer_test.h"
#include "utils/registry_utils.h"

#include <fmt/format.h>

namespace zrythm::controllers
{

// ============================================================================
// Parameterized Tests for Missing Required Fields
// ============================================================================
// These tests verify that the schema correctly rejects JSON with missing
// required fields. This is a schema behavior test, not a C++ serialization test.
// ============================================================================

namespace
{

class MissingRequiredFieldTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingRequiredFieldTest, TopLevelFieldMissing_Throws)
{
  j_.erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingRequiredFieldTest,
  testing::Values (
    "documentType",
    "schemaVersion",
    "appVersion",
    "datetime",
    "title",
    "projectData"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

class MissingProjectDataFieldTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingProjectDataFieldTest, ProjectDataFieldMissing_Throws)
{
  j_["projectData"].erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingProjectDataFieldTest,
  testing::Values ("tempoMap", "transport", "tracklist", "registry"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

class MissingRegistryTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingRegistryTest, RegistryMissing_Throws)
{
  j_["projectData"]["registry"].erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingRegistryTest,
  testing::Values (
    "ports",
    "parameters",
    "plugins",
    "tracks",
    "arrangerObjects",
    "fileAudioSources"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

class MissingTracklistFieldTest : public testing::TestWithParam<std::string>
{
protected:
  void SetUp () override { j_ = create_minimal_valid_project_json (); }
  nlohmann::json j_;
};

TEST_P (MissingTracklistFieldTest, TracklistFieldMissing_Throws)
{
  j_["projectData"]["tracklist"].erase (GetParam ());
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j_); },
    utils::exceptions::ZrythmException);
}

INSTANTIATE_TEST_SUITE_P (
  ProjectJsonSerializerValidationTest,
  MissingTracklistFieldTest,
  testing::Values ("tracks", "pinnedTracksCutoff", "trackRoutes"),
  [] (const testing::TestParamInfo<std::string> &param_info) {
    return "Missing_" + param_info.param;
  });

} // namespace

// ============================================================================
// Schema Validation Tests - Invalid Data Rejection
// ============================================================================
// These tests verify that the schema correctly rejects invalid data values.
// This is a schema behavior test (constraints, ranges, formats).
// ============================================================================

TEST (ProjectJsonSerializerValidationTest, InvalidJsonWrongDocumentType)
{
  auto j = create_minimal_valid_project_json ();
  j["documentType"] = "WrongDocumentType";

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonInvalidUuidFormat)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "not-a-valid-uuid";
  track["type"] = 2;
  track["name"] = "Bad Track";

  j["projectData"]["registry"]["tracks"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonInvalidColorFormat)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2;
  track["name"] = "Bad Color Track";
  track["color"] = "red";

  j["projectData"]["registry"]["tracks"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonInvalidTrackType)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 999;
  track["name"] = "Invalid Type Track";

  j["projectData"]["registry"]["tracks"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonEmpty)
{
  nlohmann::json j = nlohmann::json::object ();

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerValidationTest, InvalidJsonNotObject)
{
  nlohmann::json j = "not an object";

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

// ============================================================================
// Edge Case Tests
// ============================================================================
// These tests verify edge cases that are hard to create via C++ objects
// or test schema behavior with unusual but valid inputs.
// ============================================================================

TEST (ProjectJsonSerializerValidationTest, ValidateJson_EmptyTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_UnicodeInTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "测试项目 🎵";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_UnicodeInTrackName)
{
  auto j = create_minimal_valid_project_json ();

  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2;
  track["name"] = "钢琴轨道 🎹";
  j["projectData"]["registry"]["tracks"].push_back (track);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_WhitespaceInStrings)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "  Project with spaces  ";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_SpecialCharactersInTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = "Project \"with quotes\" and \\backslash\\";
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerValidationTest, ValidateJson_VeryLongTitle)
{
  auto j = create_minimal_valid_project_json ();
  j["title"] = std::string (1000, 'a');
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

// ============================================================================
// Comprehensive Schema Validation Test
// ============================================================================
// Creates a project with all major serializable content types and validates
// the serialized JSON against the schema. This catches drift between the
// C++ serializers and the JSON schema.
// ============================================================================

TEST_F (
  ProjectSerializationTest,
  ValidateJson_AllContentTypes_ProducesSchemaValidJson)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  auto &factory = *project->arrangerObjectFactory ();

  // ---- MIDI track + MidiClip + MidiNote ----
  structure::tracks::MidiTrack * midi_track_ptr = nullptr;
  utils::TypedUuidReference<structure::arrangement::MidiClip> midi_clip_ref{
    project->get_registry ()
  };
  {
    auto track_ref = project->track_factory_->create_empty_track<
      structure::tracks::MidiTrack> ();
    auto * track = track_ref.get_object_as<structure::tracks::MidiTrack> ();
    track->setName (u8"MIDI Track");
    project->tracklist ()->collection ()->add_track (track_ref);
    midi_track_ptr = track;

    midi_clip_ref =
      factory.get_builder<structure::arrangement::MidiClip> ()
        .with_start_ticks (units::ticks (0))
        .with_end_ticks (units::ticks (960))
        .with_name (u8"MIDI Clip")
        .build_in_registry ();
    auto * clip =
      midi_clip_ref.get_object_as<structure::arrangement::MidiClip> ();
    track->lanes ()->at (0)->structure::arrangement::
      ArrangerObjectOwner<structure::arrangement::MidiClip>::add_object (
        midi_clip_ref);

    auto note_ref =
      factory.get_builder<structure::arrangement::MidiNote> ()
        .with_start_ticks (units::ticks (120))
        .with_pitch (60)
        .with_velocity (100)
        .build_in_registry ();
    clip->ArrangerObjectOwner<structure::arrangement::MidiNote>::add_object (
      note_ref);
  }

  // ---- Audio track + AudioClip ----
  {
    auto track_ref = project->track_factory_->create_empty_track<
      structure::tracks::AudioTrack> ();
    auto * track = track_ref.get_object_as<structure::tracks::AudioTrack> ();
    track->setName (u8"Audio Track");
    project->tracklist ()->collection ()->add_track (track_ref);

    utils::audio::AudioBuffer dummy_buf (2, 1);
    dummy_buf.clear ();
    auto &file_registry = project->get_registry ();
    auto  audio_source_ref = utils::create_object<dsp::FileAudioSource> (
      file_registry, dummy_buf, utils::audio::BitDepth::BIT_DEPTH_32,
      units::sample_rate (44100), units::bpm (120.0), u8"test_audio.wav");

    auto clip_ref =
      factory.create_audio_clip_with_clip (audio_source_ref, units::ticks (0));
    auto * clip = clip_ref.get_object_as<structure::arrangement::AudioClip> ();
    clip->name ()->setName (u8"Audio Clip");
    track->lanes ()->at (0)->structure::arrangement::
      ArrangerObjectOwner<structure::arrangement::AudioClip>::add_object (
        clip_ref);
  }

  // ---- Chord track + ChordClip + ChordObject ----
  {
    auto track_ref = project->track_factory_->create_empty_track<
      structure::tracks::ChordTrack> ();
    project->tracklist ()->collection ()->add_track (track_ref);
    project->tracklist ()->singletonTracks ()->setChordTrack (
      track_ref.get_object_as<structure::tracks::ChordTrack> ());
    auto * chord_track =
      project->tracklist ()->singletonTracks ()->chordTrack ();
    ASSERT_NE (chord_track, nullptr);

    auto clip_ref =
      factory.get_builder<structure::arrangement::ChordClip> ()
        .with_start_ticks (units::ticks (0))
        .with_end_ticks (units::ticks (960))
        .with_name (u8"Chord Clip")
        .build_in_registry ();
    auto * clip = clip_ref.get_object_as<structure::arrangement::ChordClip> ();
    chord_track->structure::arrangement::ArrangerObjectOwner<
      structure::arrangement::ChordClip>::add_object (clip_ref);

    auto chord_ref =
      factory.get_builder<structure::arrangement::ChordObject> ()
        .with_start_ticks (units::ticks (0))
        .with_chord_descriptor (dsp::MusicalNote::C, dsp::ChordType::Major)
        .build_in_registry ();
    clip->add_object (chord_ref);
  }

  // ---- Automation clip + Automation point on AudioTrack ----
  {
    auto track_ref = project->track_factory_->create_empty_track<
      structure::tracks::AudioTrack> ();
    auto * track = track_ref.get_object_as<structure::tracks::AudioTrack> ();
    track->setName (u8"Automation Track");
    project->tracklist ()->collection ()->add_track (track_ref);

    auto clip_ref =
      factory.get_builder<structure::arrangement::AutomationClip> ()
        .with_start_ticks (units::ticks (0))
        .with_end_ticks (units::ticks (960))
        .with_name (u8"Automation Clip")
        .build_in_registry ();
    track->automationTracklist ()->automation_track_at (0)->add_object (
      clip_ref);

    auto point_ref =
      factory.get_builder<structure::arrangement::AutomationPoint> ()
        .with_start_ticks (units::ticks (480))
        .with_automatable_value (0.75)
        .build_in_registry ();
    clip_ref.get_object_as<structure::arrangement::AutomationClip> ()
      ->add_object (point_ref);
  }

  // ---- Scene with a ClipSlot referencing the MIDI clip ----
  // This exercises clipId serialization in the clip launcher.
  {
    auto * scene = project->clipLauncher ()->addScene ();
    scene->setName ("Scene 1");

    ASSERT_NE (midi_track_ptr, nullptr);
    auto * clip_slot = scene->clipSlots ()->clipSlotForTrack (midi_track_ptr);
    ASSERT_NE (clip_slot, nullptr);
    clip_slot->setClip (midi_clip_ref.get ());
  }

  create_ui_state_and_undo_stack (*project);

  nlohmann::json j = ProjectJsonSerializer::serialize (
    *project, *ui_state, *undo_stack, TEST_APP_VERSION,
    "All Content Types Test");

  try
    {
      ProjectJsonSerializer::validate_json (j);
    }
  catch (const utils::exceptions::ZrythmException &e)
    {
      FAIL ()
        << "Full project with all content types should produce "
           "schema-valid JSON. Check for property name mismatches "
           "between C++ serializers and "
           "data/schemas/project.schema.json. Error: "
        << e.what ();
    }
}
}
