// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

TEST (ProjectJsonSerializerTest, Constants)
{
  // Test static constants
  EXPECT_EQ (ProjectJsonSerializer::FORMAT_MAJOR_VERSION, 2);
  EXPECT_EQ (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 1);
  EXPECT_EQ (ProjectJsonSerializer::DOCUMENT_TYPE, "ZrythmProject"sv);
  EXPECT_EQ (ProjectJsonSerializer::kProjectData, "projectData"sv);
  EXPECT_EQ (ProjectJsonSerializer::kDatetimeKey, "datetime"sv);
  EXPECT_EQ (ProjectJsonSerializer::kAppVersionKey, "appVersion"sv);
}

TEST (ProjectJsonSerializerTest, ConstantValidation)
{
  // Verify that all required fields are present in the serializer constants

  // Document type should be non-empty and match expected value
  EXPECT_FALSE (ProjectJsonSerializer::DOCUMENT_TYPE.empty ());
  EXPECT_EQ (ProjectJsonSerializer::DOCUMENT_TYPE, "ZrythmProject"sv);

  // Format versions should be reasonable
  EXPECT_GE (ProjectJsonSerializer::FORMAT_MAJOR_VERSION, 1);
  EXPECT_GE (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 0);
  EXPECT_LE (ProjectJsonSerializer::FORMAT_MINOR_VERSION, 99);

  // All keys should be non-empty
  EXPECT_FALSE (ProjectJsonSerializer::kProjectData.empty ());
  EXPECT_FALSE (ProjectJsonSerializer::kDatetimeKey.empty ());
  EXPECT_FALSE (ProjectJsonSerializer::kAppVersionKey.empty ());
}

TEST (ProjectJsonSerializerTest, ValidateValidJson)
{
  // Create a minimal valid project JSON
  nlohmann::json j;
  j["documentType"] = "ZrythmProject";
  j["formatMajor"] = 2;
  j["formatMinor"] = 1;
  j["appVersion"] = "2.0.0";
  j["datetime"] = "2026-01-28T12:00:00Z";
  j["title"] = "Test Project";
  j[ProjectJsonSerializer::kProjectData] = nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tempoMap"] = nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tempoMap"]["timeSignatures"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["tempoMap"]["tempoChanges"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["transport"] =
    nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tracklist"] =
    nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["tracklist"]["tracks"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["tracklist"]["pinnedTracksCutoff"] = 0;
  j[ProjectJsonSerializer::kProjectData]["tracklist"]["trackRoutes"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"] =
    nlohmann::json::object ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["portRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["paramRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["pluginRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]["trackRegistry"] =
    nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]
   ["arrangerObjectRegistry"] = nlohmann::json::array ();
  j[ProjectJsonSerializer::kProjectData]["registries"]
   ["fileAudioSourceRegistry"] = nlohmann::json::array ();

  // Should not throw
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}

TEST (ProjectJsonSerializerTest, ValidateInvalidJson)
{
  // Create an invalid project JSON (missing required fields)
  nlohmann::json j;
  j["documentType"] = "ZrythmProject";
  // Missing required fields like formatMajor, formatMinor, etc.

  // Should throw
  EXPECT_THROW (
    { ProjectJsonSerializer::validate_json (j); },
    utils::exceptions::ZrythmException);
}

// Fixture for testing Project serialization with a minimal Project setup
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

TEST_F (ProjectSerializationTest, SerializeProject)
{
  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);

  // Serialize the project
  nlohmann::json j = ProjectJsonSerializer::serialize (
    *project, "2.0.0-test", "Test Serialization Project");

  // Verify top-level structure
  EXPECT_EQ (j["documentType"].get<std::string> (), "ZrythmProject");
  EXPECT_EQ (j["formatMajor"], 2);
  EXPECT_EQ (j["formatMinor"], 1);
  EXPECT_EQ (j["appVersion"].get<std::string> (), "2.0.0-test");
  EXPECT_EQ (j["title"].get<std::string> (), "Test Serialization Project");

  // Verify datetime is present and valid (ISO 8601 format)
  EXPECT_TRUE (j.contains ("datetime"));
  EXPECT_FALSE (j["datetime"].get<std::string> ().empty ());

  // Verify project data section exists
  EXPECT_TRUE (j.contains ("projectData"));
  auto &projectData = j["projectData"];
  EXPECT_TRUE (projectData.is_object ());

  // Verify expected top-level project data keys exist
  EXPECT_TRUE (projectData.contains ("tempoMap"));
  EXPECT_TRUE (projectData.contains ("transport"));
  EXPECT_TRUE (projectData.contains ("tracklist"));
  EXPECT_TRUE (projectData.contains ("registries"));

  // Verify tracklist structure
  EXPECT_TRUE (projectData.contains ("tracklist"));
  auto &tracklist = projectData["tracklist"];
  EXPECT_TRUE (tracklist.contains ("tracks"));
  EXPECT_TRUE (tracklist.contains ("pinnedTracksCutoff"));
  EXPECT_TRUE (tracklist.contains ("trackRoutes"));

  // Verify registries structure
  auto &registries = projectData["registries"];
  EXPECT_TRUE (registries.contains ("portRegistry"));
  EXPECT_TRUE (registries.contains ("paramRegistry"));
  EXPECT_TRUE (registries.contains ("pluginRegistry"));
  EXPECT_TRUE (registries.contains ("trackRegistry"));
  EXPECT_TRUE (registries.contains ("arrangerObjectRegistry"));
  EXPECT_TRUE (registries.contains ("fileAudioSourceRegistry"));

  // Verify the JSON validates against the schema
  EXPECT_NO_THROW ({ ProjectJsonSerializer::validate_json (j); });
}
