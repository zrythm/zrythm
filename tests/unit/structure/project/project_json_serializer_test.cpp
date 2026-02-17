// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <regex>
#include <set>
#include <string>

#include "dsp/juce_hardware_audio_interface.h"
#include "structure/project/project.h"
#include "structure/project/project_json_serializer.h"
#include "utils/io_utils.h"

#include "helpers/mock_audio_io_device.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace zrythm::structure::project;
using namespace std::string_view_literals;

// ============================================================================
// Constants Tests
// ============================================================================

TEST (ProjectJsonSerializerTest, DocumentTypeConstant)
{
  EXPECT_EQ (ProjectJsonSerializer::DOCUMENT_TYPE, "ZrythmProject"sv);
  EXPECT_FALSE (ProjectJsonSerializer::DOCUMENT_TYPE.empty ());
}

TEST (ProjectJsonSerializerTest, FormatVersionConstants)
{
  // Major version should be >= 1
  EXPECT_GE (ProjectJsonSerializer::FORMAT_MAJOR_VERSION, 1);

  // Minor version should be in valid range
  EXPECT_GE (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 0);
  EXPECT_LE (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 99);
}

TEST (ProjectJsonSerializerTest, JsonKeyConstants)
{
  // All key constants should be non-empty
  EXPECT_FALSE (ProjectJsonSerializer::kProjectData.empty ());
  EXPECT_FALSE (ProjectJsonSerializer::kDatetimeKey.empty ());
  EXPECT_FALSE (ProjectJsonSerializer::kAppVersionKey.empty ());

  // Verify exact values for consistency
  EXPECT_EQ (ProjectJsonSerializer::kProjectData, "projectData"sv);
  EXPECT_EQ (ProjectJsonSerializer::kDatetimeKey, "datetime"sv);
  EXPECT_EQ (ProjectJsonSerializer::kAppVersionKey, "appVersion"sv);
}

// ============================================================================
// JSON Validation Tests
// ============================================================================

namespace
{

/// Creates a minimal valid project JSON matching the current schema
nlohmann::json
create_minimal_valid_project_json ()
{
  nlohmann::json j;

  // Top-level required fields
  j["documentType"] = "ZrythmProject";
  j["formatMajor"] = 2;
  j["formatMinor"] = 1;
  j["appVersion"] = "2.0.0-test";
  j["datetime"] = "2026-02-16T12:00:00Z";
  j["title"] = "Test Project";

  // projectData section
  auto &pd = j["projectData"];

  // tempoMap (required)
  pd["tempoMap"] = nlohmann::json::object ();
  pd["tempoMap"]["timeSignatures"] = nlohmann::json::array ();
  pd["tempoMap"]["tempoChanges"] = nlohmann::json::array ();

  // transport (required)
  pd["transport"] = nlohmann::json::object ();

  // tracklist (required)
  pd["tracklist"] = nlohmann::json::object ();
  pd["tracklist"]["tracks"] = nlohmann::json::array ();
  pd["tracklist"]["pinnedTracksCutoff"] = 0;
  pd["tracklist"]["trackRoutes"] = nlohmann::json::array ();

  // registries (required)
  pd["registries"] = nlohmann::json::object ();
  pd["registries"]["portRegistry"] = nlohmann::json::array ();
  pd["registries"]["paramRegistry"] = nlohmann::json::array ();
  pd["registries"]["pluginRegistry"] = nlohmann::json::array ();
  pd["registries"]["trackRegistry"] = nlohmann::json::array ();
  pd["registries"]["arrangerObjectRegistry"] = nlohmann::json::array ();
  pd["registries"]["fileAudioSourceRegistry"] = nlohmann::json::array ();

  return j;
}

} // namespace

TEST (ProjectJsonSerializerTest, ValidateMinimalValidJson)
{
  auto j = create_minimal_valid_project_json ();
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateJsonWithOptionalFields)
{
  auto  j = create_minimal_valid_project_json ();
  auto &pd = j["projectData"];

  // Add optional fields
  pd["snapGridTimeline"] = nlohmann::json::object ();
  pd["snapGridTimeline"]["snapEnabled"] = true;
  pd["snapGridTimeline"]["snapNoteLength"] = 5; // 1/4 note
  pd["snapGridTimeline"]["snapNoteType"] = 0;   // normal

  pd["snapGridEditor"] = nlohmann::json::object ();
  pd["snapGridEditor"]["snapEnabled"] = false;

  pd["audioPool"] = nlohmann::json::object ();
  pd["audioPool"]["fileHashes"] = nlohmann::json::array ();

  pd["portConnectionsManager"] = nlohmann::json::object ();
  pd["portConnectionsManager"]["connections"] = nlohmann::json::array ();

  pd["tempoObjectManager"] = nlohmann::json::object ();
  pd["tempoObjectManager"]["tempoObjects"] = nlohmann::json::array ();
  pd["tempoObjectManager"]["timeSignatureObjects"] = nlohmann::json::array ();

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateJsonWithValidTrack)
{
  auto j = create_minimal_valid_project_json ();

  // Add a minimal valid track to trackRegistry
  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2; // MIDI track
  track["name"] = "Test MIDI Track";
  track["visible"] = true;
  track["mainHeight"] = 80.0;
  track["enabled"] = true;
  track["color"] = "#ff5588";

  j["projectData"]["registries"]["trackRegistry"].push_back (track);
  j["projectData"]["tracklist"]["tracks"].push_back (
    "550e8400-e29b-41d4-a716-446655440000");

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateJsonWithValidPort)
{
  auto j = create_minimal_valid_project_json ();

  // Add a minimal valid port to portRegistry
  nlohmann::json port;
  port["id"] = "660e8400-e29b-41d4-a716-446655440001";
  port["type"] = 0; // Audio
  port["flow"] = 1; // Output
  port["label"] = "Stereo Out L";

  j["projectData"]["registries"]["portRegistry"].push_back (port);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateJsonWithValidParameter)
{
  auto j = create_minimal_valid_project_json ();

  // Add a minimal valid parameter to paramRegistry
  nlohmann::json param;
  param["id"] = "770e8400-e29b-41d4-a716-446655440002";
  param["uniqueId"] = "fader-volume";
  param["baseValue"] = 0.75;

  j["projectData"]["registries"]["paramRegistry"].push_back (param);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateJsonWithValidMidiNote)
{
  auto j = create_minimal_valid_project_json ();

  // Add a minimal valid MIDI note to arrangerObjectRegistry
  nlohmann::json note;
  note["id"] = "880e8400-e29b-41d4-a716-446655440003";
  note["type"] = 0; // MidiNote
  note["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  note["length"] = {
    { "mode",  0     },
    { "value", 960.0 }
  };                  // 1 beat at 960 PPQN
  note["pitch"] = 60; // Middle C
  note["velocity"] = 100;

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (note);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateJsonWithValidAutomationPoint)
{
  auto j = create_minimal_valid_project_json ();

  // Add a minimal valid automation point
  nlohmann::json ap;
  ap["id"] = "990e8400-e29b-41d4-a716-446655440004";
  ap["type"] = 7; // AutomationPoint
  ap["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ap["normalizedValue"] = 0.5;
  ap["curveOptions"] = {
    { "curviness", 0.0 },
    { "algo",      0   }
  };

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ap);

  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

// ============================================================================
// Invalid JSON Tests
// ============================================================================

TEST (ProjectJsonSerializerTest, InvalidJsonMissingDocumentType)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("documentType");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonWrongDocumentType)
{
  auto j = create_minimal_valid_project_json ();
  j["documentType"] = "WrongDocumentType";

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingFormatMajor)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("formatMajor");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingFormatMinor)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("formatMinor");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingAppVersion)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("appVersion");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingDatetime)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("datetime");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingTitle)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("title");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingProjectData)
{
  auto j = create_minimal_valid_project_json ();
  j.erase ("projectData");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingTempoMap)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"].erase ("tempoMap");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingTransport)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"].erase ("transport");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingTracklist)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"].erase ("tracklist");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingRegistries)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"].erase ("registries");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMissingRequiredRegistry)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"]["registries"].erase ("portRegistry");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonTracklistMissingTracks)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"]["tracklist"].erase ("tracks");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonTracklistMissingPinnedTracksCutoff)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"]["tracklist"].erase ("pinnedTracksCutoff");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonTracklistMissingTrackRoutes)
{
  auto j = create_minimal_valid_project_json ();
  j["projectData"]["tracklist"].erase ("trackRoutes");

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonInvalidUuidFormat)
{
  auto j = create_minimal_valid_project_json ();

  // Add track with invalid UUID format
  nlohmann::json track;
  track["id"] = "not-a-valid-uuid";
  track["type"] = 2;
  track["name"] = "Bad Track";

  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonInvalidColorFormat)
{
  auto j = create_minimal_valid_project_json ();

  // Add track with invalid color format
  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 2;
  track["name"] = "Bad Color Track";
  track["color"] = "red"; // Should be #RRGGBB

  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonInvalidTrackType)
{
  auto j = create_minimal_valid_project_json ();

  // Add track with invalid type (out of range)
  nlohmann::json track;
  track["id"] = "550e8400-e29b-41d4-a716-446655440000";
  track["type"] = 999; // Invalid
  track["name"] = "Invalid Type Track";

  j["projectData"]["registries"]["trackRegistry"].push_back (track);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonMidiNotePitchOutOfRange)
{
  auto j = create_minimal_valid_project_json ();

  // Add MIDI note with pitch > 127
  nlohmann::json note;
  note["id"] = "880e8400-e29b-41d4-a716-446655440003";
  note["type"] = 0;
  note["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  note["pitch"] = 128; // Invalid: max is 127
  note["velocity"] = 100;

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (note);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonAutomationPointValueOutOfRange)
{
  auto j = create_minimal_valid_project_json ();

  // Add automation point with value > 1.0
  nlohmann::json ap;
  ap["id"] = "990e8400-e29b-41d4-a716-446655440004";
  ap["type"] = 7;
  ap["position"] = {
    { "mode",  0   },
    { "value", 0.0 }
  };
  ap["normalizedValue"] = 1.5; // Invalid: max is 1.0
  ap["curveOptions"] = {
    { "curviness", 0.0 },
    { "algo",      0   }
  };

  j["projectData"]["registries"]["arrangerObjectRegistry"].push_back (ap);

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonEmpty)
{
  nlohmann::json j = nlohmann::json::object ();

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

TEST (ProjectJsonSerializerTest, InvalidJsonNotObject)
{
  nlohmann::json j = "not an object";

  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

// ============================================================================
// Project Serialization Integration Tests
// ============================================================================

/// Fixture for testing Project serialization with a minimal Project setup
class ProjectSerializationTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    // Create a temporary directory for the project
    temp_dir_obj = utils::io::make_tmp_dir ();
    project_dir =
      utils::Utf8String::from_qstring (temp_dir_obj->path ()).to_path ();

    // Create audio device manager with dummy device
    audio_device_manager =
      test_helpers::create_audio_device_manager_with_dummy_device ();

    // Create hardware audio interface wrapper
    hw_interface =
      dsp::JuceHardwareAudioInterface::create (audio_device_manager);

    plugin_format_manager = std::make_shared<juce::AudioPluginFormatManager> ();
    plugin_format_manager->addDefaultFormats ();

    // Create a mock settings backend
    auto mock_backend = std::make_unique<test_helpers::MockSettingsBackend> ();
    mock_backend_ptr = mock_backend.get ();

    // Set up default expectations for common settings
    ON_CALL (*mock_backend_ptr, value (testing::_, testing::_))
      .WillByDefault (testing::Return (QVariant ()));

    app_settings =
      std::make_unique<utils::AppSettings> (std::move (mock_backend));

    // Create port registry and monitor fader
    port_registry = std::make_unique<dsp::PortRegistry> (nullptr);
    param_registry = std::make_unique<dsp::ProcessorParameterRegistry> (
      *port_registry, nullptr);
    monitor_fader = utils::make_qobject_unique<dsp::Fader> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry,
        .param_registry_ = *param_registry,
      },
      dsp::PortType::Audio,
      true,  // hard_limit_output
      false, // make_params_automatable
      [] () -> utils::Utf8String { return u8"Test Control Room"; },
      [] (bool fader_solo_status) { return false; });

    // Create metronome with test samples
    juce::AudioSampleBuffer emphasis_sample (2, 512);
    juce::AudioSampleBuffer normal_sample (2, 512);
    metronome = utils::make_qobject_unique<dsp::Metronome> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry,
        .param_registry_ = *param_registry,
      },
      emphasis_sample, normal_sample, true, 1.0f, nullptr);
  }

  void TearDown () override
  {
    metronome.reset ();
    monitor_fader.reset ();
    param_registry.reset ();
    port_registry.reset ();
    app_settings.reset ();
    plugin_format_manager.reset ();
    hw_interface.reset ();
  }

  std::unique_ptr<Project> create_minimal_project ()
  {
    structure::project::Project::ProjectDirectoryPathProvider path_provider =
      [this] (bool for_backup) {
        if (for_backup)
          {
            return project_dir / "backups";
          }
        return project_dir;
      };

    plugins::PluginHostWindowFactory window_factory =
      [] (plugins::Plugin &) -> std::unique_ptr<plugins::IPluginHostWindow> {
      return nullptr;
    };

    auto project = std::make_unique<Project> (
      *app_settings, path_provider, *hw_interface, plugin_format_manager,
      window_factory, *metronome, *monitor_fader);

    return project;
  }

  // Helper to verify a UUID string format
  static void assert_valid_uuid (const std::string &uuid_str)
  {
    // UUID format: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    static const std::regex uuid_regex (
      R"([0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12})",
      std::regex_constants::icase);
    EXPECT_TRUE (std::regex_match (uuid_str, uuid_regex))
      << "Invalid UUID format: " << uuid_str;
  }

  // Helper to verify a hex color format (#RRGGBB)
  static void assert_valid_color (const std::string &color_str)
  {
    static const std::regex color_regex (R"(#[0-9A-Fa-f]{6})");
    EXPECT_TRUE (std::regex_match (color_str, color_regex))
      << "Invalid color format: " << color_str;
  }

  std::unique_ptr<QTemporaryDir>                   temp_dir_obj;
  fs::path                                         project_dir;
  std::shared_ptr<juce::AudioDeviceManager>        audio_device_manager;
  std::unique_ptr<dsp::IHardwareAudioInterface>    hw_interface;
  std::shared_ptr<juce::AudioPluginFormatManager>  plugin_format_manager;
  test_helpers::MockSettingsBackend *              mock_backend_ptr{};
  std::unique_ptr<utils::AppSettings>              app_settings;
  std::unique_ptr<dsp::PortRegistry>               port_registry;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry;
  utils::QObjectUniquePtr<dsp::Fader>              monitor_fader;
  utils::QObjectUniquePtr<dsp::Metronome>          metronome;
};

// ============================================================================
// Top-Level Document Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TopLevelFields)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  const std::string test_version = "2.0.0-test";
  const std::string test_title = "Test Serialization Project";

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, test_version, test_title);

  // Verify document type
  EXPECT_EQ (j["documentType"].get<std::string> (), "ZrythmProject");

  // Verify format version
  EXPECT_EQ (j["formatMajor"].get<int> (), 2);
  EXPECT_EQ (j["formatMinor"].get<int> (), 1);

  // Verify app version
  EXPECT_EQ (j["appVersion"].get<std::string> (), test_version);

  // Verify title
  EXPECT_EQ (j["title"].get<std::string> (), test_title);

  // Verify datetime is present (ISO 8601 format)
  EXPECT_TRUE (j.contains ("datetime"));
  auto datetime = j["datetime"].get<std::string> ();
  EXPECT_FALSE (datetime.empty ());
  EXPECT_TRUE (datetime.find ('T') != std::string::npos)
    << "Datetime should be ISO 8601 format: " << datetime;
}

TEST_F (ProjectSerializationTest, SerializeProject_ProjectDataExists)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");

  // Verify projectData section exists and is an object
  EXPECT_TRUE (j.contains ("projectData"));
  EXPECT_TRUE (j["projectData"].is_object ());
}

// ============================================================================
// Required Fields Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_RequiredFields)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &pd = j["projectData"];

  // Verify all required fields exist
  EXPECT_TRUE (pd.contains ("tempoMap")) << "Missing tempoMap";
  EXPECT_TRUE (pd.contains ("transport")) << "Missing transport";
  EXPECT_TRUE (pd.contains ("tracklist")) << "Missing tracklist";
  EXPECT_TRUE (pd.contains ("registries")) << "Missing registries";
}

// ============================================================================
// TempoMap Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TempoMapStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &tempo_map = j["projectData"]["tempoMap"];

  // TempoMap should contain timeSignatures and tempoChanges arrays
  EXPECT_TRUE (tempo_map.contains ("timeSignatures"));
  EXPECT_TRUE (tempo_map["timeSignatures"].is_array ());

  EXPECT_TRUE (tempo_map.contains ("tempoChanges"));
  EXPECT_TRUE (tempo_map["tempoChanges"].is_array ());
}

// ============================================================================
// Transport Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TransportStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &transport = j["projectData"]["transport"];

  // Transport should contain playhead and position-related fields
  EXPECT_TRUE (transport.contains ("playheadPosition"));
  EXPECT_TRUE (transport.contains ("cuePosition"));
  EXPECT_TRUE (transport.contains ("loopStartPosition"));
  EXPECT_TRUE (transport.contains ("loopEndPosition"));
  EXPECT_TRUE (transport.contains ("punchInPosition"));
  EXPECT_TRUE (transport.contains ("punchOutPosition"));
}

// ============================================================================
// Tracklist Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TracklistStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &tracklist = j["projectData"]["tracklist"];

  // Tracklist required fields
  EXPECT_TRUE (tracklist.contains ("tracks")) << "Missing tracks array";
  EXPECT_TRUE (tracklist["tracks"].is_array ());

  EXPECT_TRUE (tracklist.contains ("pinnedTracksCutoff"))
    << "Missing pinnedTracksCutoff";
  EXPECT_TRUE (tracklist["pinnedTracksCutoff"].is_number ());

  EXPECT_TRUE (tracklist.contains ("trackRoutes")) << "Missing trackRoutes";
  EXPECT_TRUE (tracklist["trackRoutes"].is_array ());
}

TEST_F (ProjectSerializationTest, SerializeProject_TracklistTracksAreUuids)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &tracks = j["projectData"]["tracklist"]["tracks"];

  // Each track in the tracks array should be a UUID string
  for (const auto &track_id : tracks)
    {
      EXPECT_TRUE (track_id.is_string ()) << "Track ID should be a string";
      assert_valid_uuid (track_id.get<std::string> ());
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_TrackRoutesAreUuidPairs)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &routes = j["projectData"]["tracklist"]["trackRoutes"];

  // Each route should be an array of [source_uuid, dest_uuid]
  for (const auto &route : routes)
    {
      EXPECT_TRUE (route.is_array ()) << "Route should be an array";
      EXPECT_EQ (route.size (), 2) << "Route should have 2 elements";

      EXPECT_TRUE (route[0].is_string ()) << "Source should be a UUID string";
      assert_valid_uuid (route[0].get<std::string> ());

      EXPECT_TRUE (route[1].is_string ()) << "Dest should be a UUID string";
      assert_valid_uuid (route[1].get<std::string> ());
    }
}

// ============================================================================
// Registries Structure Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_RegistriesExist)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &registries = j["projectData"]["registries"];

  // All required registries must exist
  EXPECT_TRUE (registries.contains ("portRegistry")) << "Missing portRegistry";
  EXPECT_TRUE (registries.contains ("paramRegistry")) << "Missing paramRegistry";
  EXPECT_TRUE (registries.contains ("pluginRegistry"))
    << "Missing pluginRegistry";
  EXPECT_TRUE (registries.contains ("trackRegistry")) << "Missing trackRegistry";
  EXPECT_TRUE (registries.contains ("arrangerObjectRegistry"))
    << "Missing arrangerObjectRegistry";
  EXPECT_TRUE (registries.contains ("fileAudioSourceRegistry"))
    << "Missing fileAudioSourceRegistry";

  // All registries should be arrays
  EXPECT_TRUE (registries["portRegistry"].is_array ());
  EXPECT_TRUE (registries["paramRegistry"].is_array ());
  EXPECT_TRUE (registries["pluginRegistry"].is_array ());
  EXPECT_TRUE (registries["trackRegistry"].is_array ());
  EXPECT_TRUE (registries["arrangerObjectRegistry"].is_array ());
  EXPECT_TRUE (registries["fileAudioSourceRegistry"].is_array ());
}

TEST_F (ProjectSerializationTest, SerializeProject_PortRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &ports = j["projectData"]["registries"]["portRegistry"];

  for (const auto &port : ports)
    {
      // Each port must have an id (UUID) and type
      EXPECT_TRUE (port.contains ("id")) << "Port missing id";
      assert_valid_uuid (port["id"].get<std::string> ());

      EXPECT_TRUE (port.contains ("type")) << "Port missing type";
      EXPECT_TRUE (port["type"].is_number ());

      EXPECT_TRUE (port.contains ("flow")) << "Port missing flow";
      EXPECT_TRUE (port["flow"].is_number ());
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_ParamRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &params = j["projectData"]["registries"]["paramRegistry"];

  for (const auto &param : params)
    {
      // Each parameter must have id, uniqueId, and baseValue
      EXPECT_TRUE (param.contains ("id")) << "Parameter missing id";
      assert_valid_uuid (param["id"].get<std::string> ());

      EXPECT_TRUE (param.contains ("uniqueId")) << "Parameter missing uniqueId";
      EXPECT_TRUE (param["uniqueId"].is_string ());

      EXPECT_TRUE (param.contains ("baseValue"))
        << "Parameter missing baseValue";
      EXPECT_TRUE (param["baseValue"].is_number ());
      EXPECT_GE (param["baseValue"].get<double> (), 0.0);
      EXPECT_LE (param["baseValue"].get<double> (), 1.0);
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_TrackRegistryStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &tracks = j["projectData"]["registries"]["trackRegistry"];

  for (const auto &track : tracks)
    {
      // Each track must have id, type, and name
      EXPECT_TRUE (track.contains ("id")) << "Track missing id";
      assert_valid_uuid (track["id"].get<std::string> ());

      EXPECT_TRUE (track.contains ("type")) << "Track missing type";
      auto track_type = track["type"].get<int> ();
      EXPECT_GE (track_type, 0);
      EXPECT_LE (track_type, 11);

      EXPECT_TRUE (track.contains ("name")) << "Track missing name";
      EXPECT_TRUE (track["name"].is_string ());

      // Optional fields that should have correct format if present
      if (track.contains ("color"))
        {
          assert_valid_color (track["color"].get<std::string> ());
        }
    }
}

// ============================================================================
// Optional Fields Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_OptionalFields)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &pd = j["projectData"];

  // These optional fields should be present in a typical project
  // (the serializer outputs them)
  EXPECT_TRUE (pd.contains ("snapGridTimeline")) << "Missing snapGridTimeline";
  EXPECT_TRUE (pd.contains ("snapGridEditor")) << "Missing snapGridEditor";
  EXPECT_TRUE (pd.contains ("audioPool")) << "Missing audioPool";
  EXPECT_TRUE (pd.contains ("portConnectionsManager"))
    << "Missing portConnectionsManager";
  EXPECT_TRUE (pd.contains ("tempoObjectManager"))
    << "Missing tempoObjectManager";
}

TEST_F (ProjectSerializationTest, SerializeProject_AudioPoolStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");
  auto &audio_pool = j["projectData"]["audioPool"];

  // AudioPool should have fileHashes array
  EXPECT_TRUE (audio_pool.contains ("fileHashes"));
  EXPECT_TRUE (audio_pool["fileHashes"].is_array ());

  // Each entry in fileHashes should be [uuid, hash] pairs
  for (const auto &entry : audio_pool["fileHashes"])
    {
      EXPECT_TRUE (entry.is_array ()) << "fileHashes entry should be an array";
      EXPECT_EQ (entry.size (), 2) << "fileHashes entry should have 2 elements";

      // First element: UUID reference to fileAudioSourceRegistry
      EXPECT_TRUE (entry[0].is_string ())
        << "First element should be a UUID string";
      assert_valid_uuid (entry[0].get<std::string> ());

      // Second element: file hash (integer)
      EXPECT_TRUE (entry[1].is_number ())
        << "Second element should be a hash integer";
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_SnapGridStructure)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");

  // snapGridTimeline structure
  auto &snap_timeline = j["projectData"]["snapGridTimeline"];
  EXPECT_TRUE (snap_timeline.is_object ());

  // snapGridEditor structure
  auto &snap_editor = j["projectData"]["snapGridEditor"];
  EXPECT_TRUE (snap_editor.is_object ());
}

// ============================================================================
// Schema Validation Test
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_ValidatesAgainstSchema)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0-test", "Schema Test");

  // The serialized JSON should validate against the schema without errors
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

// ============================================================================
// Consistency Tests
// ============================================================================

TEST_F (ProjectSerializationTest, SerializeProject_TrackRegistryMatchesTracklist)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");

  // Collect all track IDs from trackRegistry
  std::set<std::string> registry_ids;
  for (const auto &track : j["projectData"]["registries"]["trackRegistry"])
    {
      registry_ids.insert (track["id"].get<std::string> ());
    }

  // Collect all track IDs referenced in tracklist
  std::set<std::string> tracklist_ids;
  for (const auto &track_id : j["projectData"]["tracklist"]["tracks"])
    {
      tracklist_ids.insert (track_id.get<std::string> ());
    }

  // All tracks in tracklist should exist in registry
  for (const auto &id : tracklist_ids)
    {
      EXPECT_TRUE (registry_ids.contains (id))
        << "Track " << id << " in tracklist but not in trackRegistry";
    }
}

TEST_F (ProjectSerializationTest, SerializeProject_PortReferencesAreValid)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  nlohmann::json j =
    ProjectJsonSerializer::serialize (*project, "2.0.0", "Test");

  // Collect all port IDs
  std::set<std::string> port_ids;
  for (const auto &port : j["projectData"]["registries"]["portRegistry"])
    {
      port_ids.insert (port["id"].get<std::string> ());
    }

  // Check track processors reference valid ports
  for (const auto &track : j["projectData"]["registries"]["trackRegistry"])
    {
      if (track.contains ("processor"))
        {
          auto &processor = track["processor"];
          if (processor.contains ("inputPorts"))
            {
              for (const auto &port_ref : processor["inputPorts"])
                {
                  EXPECT_TRUE (port_ids.contains (port_ref.get<std::string> ()))
                    << "Processor input port not found in portRegistry: "
                    << port_ref.get<std::string> ();
                }
            }
          if (processor.contains ("outputPorts"))
            {
              for (const auto &port_ref : processor["outputPorts"])
                {
                  EXPECT_TRUE (port_ids.contains (port_ref.get<std::string> ()))
                    << "Processor output port not found in portRegistry: "
                    << port_ref.get<std::string> ();
                }
            }
        }
    }
}
