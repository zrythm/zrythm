// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for project serialization (save/load roundtrips).
 *
 * These tests verify the full save -> load -> save cycle works correctly.
 */

#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "dsp/juce_hardware_audio_interface.h"
#include "plugins/plugin_descriptor.h"
#include "structure/project/project.h"
#include "structure/project/project_path_provider.h"
#include "structure/project/project_ui_state.h"
#include "undo/undo_stack.h"
#include "utils/io_utils.h"

#include <QSignalSpy>

#include "helpers/mock_audio_io_device.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/qt_helpers.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

namespace zrythm::controllers
{

class ProjectSerializationRoundtripTest
    : public ::testing::Test,
      protected test_helpers::ScopedJuceQApplication
{
protected:
  static constexpr utils::Version TEST_APP_VERSION{ 2, 0, {} };

  void SetUp () override
  {
    // Create a temporary directory for the project
    temp_dir_obj_ = utils::io::make_tmp_dir ();
    project_dir_ =
      utils::Utf8String::from_qstring (temp_dir_obj_->path ()).to_path ();

    // Create audio device manager with dummy device
    audio_device_manager_ =
      test_helpers::create_audio_device_manager_with_dummy_device ();

    // Create hardware audio interface wrapper
    hw_interface_ =
      dsp::JuceHardwareAudioInterface::create (audio_device_manager_);

    plugin_format_manager_ = std::make_shared<juce::AudioPluginFormatManager> ();
    juce::addDefaultFormatsToManager (*plugin_format_manager_);

    // Create a mock settings backend
    auto mock_backend = std::make_unique<test_helpers::MockSettingsBackend> ();
    mock_backend_ptr_ = mock_backend.get ();

    // Set up default expectations for common settings
    ON_CALL (*mock_backend_ptr_, value (testing::_, testing::_))
      .WillByDefault (testing::Return (QVariant ()));

    app_settings_ =
      std::make_unique<utils::AppSettings> (std::move (mock_backend));

    // Create port registry and monitor fader
    port_registry_ = std::make_unique<dsp::PortRegistry> (nullptr);
    param_registry_ = std::make_unique<dsp::ProcessorParameterRegistry> (
      *port_registry_, nullptr);
    monitor_fader_ = utils::make_qobject_unique<dsp::Fader> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_,
        .param_registry_ = *param_registry_,
      },
      dsp::PortType::Audio,
      true,  // hard_limit_output
      false, // make_params_automatable
      [] () -> utils::Utf8String { return u8"Test Control Room"; },
      [] (bool fader_solo_status) { return false; });

    // Create metronome with test samples
    juce::AudioSampleBuffer emphasis_sample (2, 512);
    juce::AudioSampleBuffer normal_sample (2, 512);
    metronome_ = utils::make_qobject_unique<dsp::Metronome> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_,
        .param_registry_ = *param_registry_,
      },
      emphasis_sample, normal_sample, true, 1.0f, nullptr);
  }

  void TearDown () override
  {
    undo_stack_.reset ();
    ui_state_.reset ();
    metronome_.reset ();
    monitor_fader_.reset ();
    param_registry_.reset ();
    port_registry_.reset ();
    app_settings_.reset ();
    plugin_format_manager_.reset ();
    hw_interface_.reset ();
  }

  std::unique_ptr<structure::project::Project> create_minimal_project ()
  {
    structure::project::Project::ProjectDirectoryPathProvider path_provider =
      [this] (bool for_backup) {
        if (for_backup)
          {
            return project_dir_ / "backups";
          }
        return project_dir_;
      };

    plugins::PluginHostWindowFactory window_factory =
      [] (plugins::Plugin &) -> std::unique_ptr<plugins::IPluginHostWindow> {
      return nullptr;
    };

    auto project = std::make_unique<structure::project::Project> (
      *app_settings_, path_provider, *hw_interface_, plugin_format_manager_,
      window_factory, *metronome_, *monitor_fader_);

    return project;
  }

  void create_ui_state_and_undo_stack (structure::project::Project &project)
  {
    ui_state_ = utils::make_qobject_unique<structure::project::ProjectUiState> (
      project, *app_settings_);

    undo_stack_ = utils::make_qobject_unique<undo::UndoStack> (
      [&project] (const std::function<void ()> &action, bool recalculate_graph) {
        project.engine ()->execute_function_with_paused_processing_synchronously (
          action, recalculate_graph);
      },
      nullptr);
  }

  std::unique_ptr<QTemporaryDir>                   temp_dir_obj_;
  fs::path                                         project_dir_;
  std::shared_ptr<juce::AudioDeviceManager>        audio_device_manager_;
  std::unique_ptr<dsp::IHardwareAudioInterface>    hw_interface_;
  std::shared_ptr<juce::AudioPluginFormatManager>  plugin_format_manager_;
  test_helpers::MockSettingsBackend *              mock_backend_ptr_{};
  std::unique_ptr<utils::AppSettings>              app_settings_;
  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  utils::QObjectUniquePtr<dsp::Fader>              monitor_fader_;
  utils::QObjectUniquePtr<dsp::Metronome>          metronome_;
  utils::QObjectUniquePtr<structure::project::ProjectUiState> ui_state_;
  utils::QObjectUniquePtr<undo::UndoStack>                    undo_stack_;
};

/**
 * @brief Tests full save -> load -> save roundtrip with a plugin.
 */
TEST_F (ProjectSerializationRoundtripTest, SaveLoadSaveRoundtripWithPlugin)
{
  // === Step 1: Create original project with a plugin ===
  auto original_project = create_minimal_project ();
  ASSERT_NE (original_project, nullptr);

  auto &plugin_registry = original_project->get_plugin_registry ();
  auto &project_port_registry = original_project->get_port_registry ();
  auto &project_param_registry = original_project->get_param_registry ();

  // Create plugin descriptor
  auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
  descriptor->name_ = u8"Test Roundtrip Plugin";
  descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;

  plugins::PluginConfiguration config;
  config.descr_ = std::move (descriptor);

  // Create the plugin with a state directory provider
  auto plugin_ref = plugin_registry.create_object<plugins::InternalPluginBase> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = project_port_registry,
      .param_registry_ = project_param_registry },
    [project_dir = this->project_dir_] () { return project_dir / "plugins"; },
    nullptr);
  auto * plugin = plugin_ref.get_object_as<plugins::InternalPluginBase> ();
  plugin->set_configuration (config);

  create_ui_state_and_undo_stack (*original_project);

  // === Step 2: Save the project ===

  original_project->engine ()->set_running (true);

  auto save_future = ProjectSaver::save (
    *original_project, *ui_state_, *undo_stack_, TEST_APP_VERSION, project_dir_,
    false);
  test_helpers::waitForFutureWithEvents (save_future);

  ASSERT_TRUE (save_future.isFinished ()) << "First save timed out";
  EXPECT_FALSE (save_future.result ().isEmpty ());

  // Verify project file was created
  auto project_file_path =
    project_dir_
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  EXPECT_TRUE (fs::exists (project_file_path))
    << "Project file not created: " << project_file_path;

  // Stop engine before loading
  original_project->engine ()->set_running (false);

  // === Step 3: Load the project into a new instance ===
  auto load_result = ProjectLoader::load_from_directory (project_dir_);
  load_result.waitForFinished ();
  ASSERT_FALSE (load_result.isCanceled ());

  auto loaded_project = create_minimal_project ();
  auto loaded_ui_state = utils::make_qobject_unique<
    structure::project::ProjectUiState> (*loaded_project, *app_settings_);
  auto loaded_undo_stack = utils::make_qobject_unique<undo::UndoStack> (
    [&] (const std::function<void ()> &action, bool recalculate_graph) {
      loaded_project->engine ()
        ->execute_function_with_paused_processing_synchronously (
          action, recalculate_graph);
    },
    nullptr);

  ProjectLoader::deserialize (
    load_result.result ().json, *loaded_project, *loaded_ui_state,
    *loaded_undo_stack);

  // === Step 4: Verify the plugin was loaded correctly ===
  auto &loaded_plugin_registry = loaded_project->get_plugin_registry ();
  EXPECT_EQ (loaded_plugin_registry.get_hash_map ().size (), 1);

  // Get the loaded plugin and verify get_state_directory() works
  // (this is where the bug manifested - the lambda captured 'this' from
  // a temporary PluginBuilderForDeserialization object)
  auto loaded_plugins =
    loaded_plugin_registry.get_hash_map () | std::views::values;
  auto loaded_plugin_it = loaded_plugins.begin ();
  ASSERT_NE (loaded_plugin_it, loaded_plugins.end ());

  std::visit (
    [this] (auto &&pl) {
      auto state_dir = pl->get_state_directory ();
      EXPECT_FALSE (state_dir.empty ())
        << "get_state_directory() returned empty path";
      EXPECT_TRUE (
        state_dir.string ().find (project_dir_.string ()) != std::string::npos)
        << "State directory " << project_dir_
        << " should be under project dir: " << state_dir;
    },
    *loaded_plugin_it);

  // === Step 5: Save the loaded project again ===
  auto resave_dir = project_dir_ / "resave_test";

  loaded_project->engine ()->set_running (true);

  auto resave_future = ProjectSaver::save (
    *loaded_project, *loaded_ui_state, *loaded_undo_stack, TEST_APP_VERSION,
    resave_dir, false);
  test_helpers::waitForFutureWithEvents (resave_future);

  ASSERT_TRUE (resave_future.isFinished ()) << "Resave timed out";
  EXPECT_FALSE (resave_future.result ().isEmpty ());

  // Verify the resaved project file exists
  auto resaved_project_file_path =
    resave_dir
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  EXPECT_TRUE (fs::exists (resaved_project_file_path))
    << "Resaved project file not created: " << resaved_project_file_path;
}

} // namespace zrythm::controllers
