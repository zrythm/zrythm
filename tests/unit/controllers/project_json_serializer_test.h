// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <memory>
#include <regex>
#include <set>
#include <string>

#include "controllers/project_json_serializer.h"
#include "dsp/juce_hardware_audio_interface.h"
#include "structure/project/project.h"
#include "structure/project/project_ui_state.h"
#include "undo/undo_stack.h"
#include "utils/io_utils.h"

#include "helpers/mock_audio_io_device.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::controllers
{

// ============================================================================
// Helper Functions
// ============================================================================

/// Creates a minimal valid project JSON matching the current schema
inline nlohmann::json
create_minimal_valid_project_json ()
{
  nlohmann::json j;

  // Top-level required fields
  j["documentType"] = "ZrythmProject";
  j["schemaVersion"] = nlohmann::json::object ();
  j["schemaVersion"]["major"] = 2;
  j["schemaVersion"]["minor"] = 1;
  j["appVersion"] = nlohmann::json::object ();
  j["appVersion"]["major"] = 2;
  j["appVersion"]["minor"] = 0;
  j["appVersion"]["patch"] = 0;
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

/// Recursively extracts all UUID values from "id" fields in a JSON structure
inline std::set<std::string>
extract_all_uuids (const nlohmann::json &j)
{
  std::set<std::string> uuids;

  const auto extract = [&] (auto &&self, const nlohmann::json &obj) -> void {
    if (obj.is_object ())
      {
        if (obj.contains ("id") && obj["id"].is_string ())
          {
            uuids.insert (obj["id"].get<std::string> ());
          }
        for (const auto &[key, val] : obj.items ())
          {
            self (self, val);
          }
      }
    else if (obj.is_array ())
      {
        for (const auto &elem : obj)
          {
            self (self, elem);
          }
      }
  };

  extract (extract, j);
  return uuids;
}

/// Counts the total number of objects in a JSON structure
inline size_t
count_objects (const nlohmann::json &j)
{
  size_t count = 0;

  const auto count_fn = [&] (auto &&self, const nlohmann::json &obj) -> void {
    if (obj.is_object ())
      {
        ++count;
        for (const auto &[key, val] : obj.items ())
          {
            self (self, val);
          }
      }
    else if (obj.is_array ())
      {
        for (const auto &elem : obj)
          {
            self (self, elem);
          }
      }
  };

  count_fn (count_fn, j);
  return count;
}

// ============================================================================
// Test Fixture
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
    undo_stack.reset ();
    ui_state.reset ();
    metronome.reset ();
    monitor_fader.reset ();
    param_registry.reset ();
    port_registry.reset ();
    app_settings.reset ();
    plugin_format_manager.reset ();
    hw_interface.reset ();
  }

  std::unique_ptr<structure::project::Project> create_minimal_project ()
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

    auto project = std::make_unique<structure::project::Project> (
      *app_settings, path_provider, *hw_interface, plugin_format_manager,
      window_factory, *metronome, *monitor_fader);

    return project;
  }

  void create_ui_state_and_undo_stack (structure::project::Project &project)
  {
    ui_state = utils::make_qobject_unique<structure::project::ProjectUiState> (
      project, *app_settings);

    undo_stack = utils::make_qobject_unique<undo::UndoStack> (
      [&project] (const std::function<void ()> &action, bool recalculate_graph) {
        project.engine ()->execute_function_with_paused_processing_synchronously (
          action, recalculate_graph);
      },
      nullptr);
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

  static constexpr utils::Version TEST_APP_VERSION{ 2, 0, {} };
  static constexpr utils::Version TEST_APP_VERSION_WITH_PATCH{ 2, 0, 1 };

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
  utils::QObjectUniquePtr<structure::project::ProjectUiState> ui_state;
  utils::QObjectUniquePtr<undo::UndoStack>                    undo_stack;
};

} // namespace zrythm::controllers
