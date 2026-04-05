// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for project serialization (save/load roundtrips).
 *
 * These tests verify the full save -> load -> save cycle works correctly.
 */

#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_factory.h"
#include "structure/project/project.h"
#include "structure/project/project_path_provider.h"
#include "structure/project/project_ui_state.h"
#include "undo/undo_stack.h"
#include "utils/file_path_list.h"
#include "utils/io_utils.h"

#include <QSignalSpy>

#include "helpers/mock_hardware_audio_interface_threaded.h"
#include "helpers/mock_settings_backend.h"
#include "helpers/qt_helpers.h"
#include "helpers/scoped_juce_qapplication.h"

#include <gtest/gtest.h>

namespace zrythm::controllers
{

/**
 * @brief Owning wrapper that holds a project together with its UI state and
 * undo stack, ensuring correct destruction order (undo stack → ui state →
 * project).
 */
struct ProjectBundle
{
  std::unique_ptr<structure::project::Project>                project;
  utils::QObjectUniquePtr<structure::project::ProjectUiState> ui_state;
  utils::QObjectUniquePtr<undo::UndoStack>                    undo_stack;

  ~ProjectBundle ()
  {
    undo_stack.reset ();
    ui_state.reset ();
    project.reset ();
  }
};

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

    hw_interface_ =
      std::make_unique<test_helpers::ThreadedMockHardwareAudioInterface> ();

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
        return for_backup ? project_dir_ / "backups" : project_dir_;
      };

    plugins::PluginHostWindowFactory window_factory =
      [] (plugins::Plugin &) -> std::unique_ptr<plugins::IPluginHostWindow> {
      return nullptr;
    };

    return std::make_unique<structure::project::Project> (
      *app_settings_, path_provider, *hw_interface_, plugin_format_manager_,
      window_factory, *metronome_, *monitor_fader_);
  }

  /**
   * @brief Finds the first bundled VST3 plugin description.
   */
  std::unique_ptr<juce::PluginDescription> findFirstBundledVst3Plugin ()
  {
    QString paths_str = QStringLiteral (BUNDLED_VST3_SEARCH_PATHS);
    if (paths_str.isEmpty ())
      return nullptr;

    auto        paths = std::make_unique<utils::FilePathList> ();
    QStringList path_list = paths_str.split (":::", Qt::SkipEmptyParts);
    for (const auto &path : path_list)
      paths->add_path (fs::path (path.toStdString ()));

    if (paths->empty ())
      return nullptr;

    juce::VST3PluginFormat vst3_format;
    auto juce_search_paths = paths->get_as_juce_file_search_path ();
    auto identifiers =
      vst3_format.searchPathsForPlugins (juce_search_paths, false, false);
    if (identifiers.isEmpty ())
      return nullptr;

    juce::OwnedArray<juce::PluginDescription> descriptions;
    vst3_format.findAllTypesForFile (descriptions, identifiers[0]);
    if (descriptions.isEmpty ())
      return nullptr;

    return std::make_unique<juce::PluginDescription> (*descriptions[0]);
  }

  /**
   * @brief Runs the full save -> load -> resave roundtrip cycle with a plugin.
   *
   * Creates a project, imports a plugin via PluginImporter, saves, loads into a
   * new instance, verifies the plugin runs correctly, and resaves.
   *
   * @param descriptor The plugin descriptor to import.
   */
  void
  saveLoadSaveRoundtripWithPlugin (const plugins::PluginDescriptor &descriptor)
  {
    // === Step 1: Create project and import plugin via PluginImporter ===
    auto original_bundle = std::make_unique<ProjectBundle> ();
    original_bundle->project = create_minimal_project ();
    ASSERT_NE (original_bundle->project, nullptr);

    original_bundle->project->add_default_tracks ();

    original_bundle->ui_state =
      utils::make_qobject_unique<structure::project::ProjectUiState> (
        *original_bundle->project, *app_settings_);
    original_bundle->undo_stack = utils::make_qobject_unique<undo::UndoStack> (
      [proj = original_bundle->project.get ()] (
        const std::function<void ()> &action, bool recalculate_graph) {
        proj->engine ()->execute_function_with_paused_processing_synchronously (
          action, recalculate_graph);
      },
      nullptr);

    bool instantiation_finished = false;
    auto handler = [&instantiation_finished] (plugins::PluginUuidReference) {
      instantiation_finished = true;
    };

    auto * tracklist = original_bundle->project->tracklist ();
    auto   track_creator = std::make_unique<actions::TrackCreator> (
      *original_bundle->undo_stack, *original_bundle->project->track_factory_,
      *tracklist->collection (), *tracklist->trackRouting (),
      *tracklist->singletonTracks (), original_bundle->project.get ());

    auto importer = std::make_unique<actions::PluginImporter> (
      *original_bundle->undo_stack, *original_bundle->project->plugin_factory_,
      *track_creator, std::move (handler), original_bundle->project.get ());

    importer->importPluginToNewTrack (&descriptor);

    // Wait for instantiation to complete
    for (int i = 0; i < 100 && !instantiation_finished; ++i)
      {
        QCoreApplication::processEvents (QEventLoop::AllEvents, 50);
      }
    EXPECT_TRUE (instantiation_finished)
      << "Plugin instantiation did not complete";

    // === Step 2: Save the project ===
    original_bundle->project->engine ()->set_running (true);

    auto save_future = ProjectSaver::save (
      *original_bundle->project, *original_bundle->ui_state,
      *original_bundle->undo_stack, TEST_APP_VERSION, project_dir_, false);
    test_helpers::waitForFutureWithEvents (save_future);

    ASSERT_TRUE (save_future.isFinished ()) << "First save timed out";
    EXPECT_FALSE (save_future.result ().isEmpty ());

    auto project_file_path =
      project_dir_
      / structure::project::ProjectPathProvider::get_path (
        structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
    EXPECT_TRUE (fs::exists (project_file_path))
      << "Project file not created: " << project_file_path;

    original_bundle->project->engine ()->set_running (false);

    // Release objects that reference the original project before destroying it
    importer.reset ();
    track_creator.reset ();
    original_bundle.reset ();

    // === Step 3: Load into a new instance ===
    auto load_result = ProjectLoader::load_from_directory (project_dir_);
    load_result.waitForFinished ();
    ASSERT_FALSE (load_result.isCanceled ());

    auto loaded_bundle = std::make_unique<ProjectBundle> ();
    loaded_bundle->project = create_minimal_project ();
    loaded_bundle->ui_state =
      utils::make_qobject_unique<structure::project::ProjectUiState> (
        *loaded_bundle->project, *app_settings_);
    loaded_bundle->undo_stack = utils::make_qobject_unique<undo::UndoStack> (
      [proj = loaded_bundle->project.get ()] (
        const std::function<void ()> &action, bool recalculate_graph) {
        proj->engine ()->execute_function_with_paused_processing_synchronously (
          action, recalculate_graph);
      },
      nullptr);

    // Deserialization triggers async JUCE plugin instantiation
    ProjectLoader::deserialize (
      load_result.result ().json, *loaded_bundle->project,
      *loaded_bundle->ui_state, *loaded_bundle->undo_stack);

    {
      // Start processing immediately
      EXPECT_NO_FATAL_FAILURE ({
        loaded_bundle->project->engine ()->activate ();
        loaded_bundle->project->engine ()->graph_dispatcher ().recalc_graph (
          false);
        loaded_bundle->project->engine ()->set_running (true);

        // Wait for the mock audio device to process a few cycles so the engine
        // processes the plugin
        auto * mock_hw =
          dynamic_cast<test_helpers::ThreadedMockHardwareAudioInterface *> (
            hw_interface_.get ());
        const auto initial_count = mock_hw->process_call_count ();
        while (mock_hw->process_call_count () - initial_count < 3)
          QCoreApplication::processEvents ();

        loaded_bundle->project->engine ()->set_running (false);
        loaded_bundle->project->engine ()->deactivate ();
      }) << "Processing a plugin after loading a project should not crash";
    }

    // === Step 4: Verify the plugin was loaded correctly ===
    auto &loaded_plugin_registry =
      loaded_bundle->project->get_plugin_registry ();
    EXPECT_GE (loaded_plugin_registry.get_hash_map ().size (), 1);

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
          << "State directory should be under project dir: " << state_dir;
      },
      *loaded_plugin_it);

    // === Step 5: Save the loaded project again ===
    auto resave_dir = project_dir_ / "resave_test";

    loaded_bundle->project->engine ()->set_running (true);

    auto resave_future = ProjectSaver::save (
      *loaded_bundle->project, *loaded_bundle->ui_state,
      *loaded_bundle->undo_stack, TEST_APP_VERSION, resave_dir, false);
    test_helpers::waitForFutureWithEvents (resave_future);

    ASSERT_TRUE (resave_future.isFinished ()) << "Resave timed out";
    EXPECT_FALSE (resave_future.result ().isEmpty ());

    auto resaved_project_file_path =
      resave_dir
      / structure::project::ProjectPathProvider::get_path (
        structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
    EXPECT_TRUE (fs::exists (resaved_project_file_path))
      << "Resaved project file not created: " << resaved_project_file_path;
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
};

/**
 * @brief Tests full save -> load -> save roundtrip with an internal plugin.
 */
TEST_F (
  ProjectSerializationRoundtripTest,
  SaveLoadSaveRoundtripWithInternalPlugin)
{
  auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
  descriptor->name_ = u8"Test Roundtrip Plugin";
  descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;

  saveLoadSaveRoundtripWithPlugin (*descriptor);
}

/**
 * @brief Tests full save -> load -> save roundtrip with a bundled VST3 plugin.
 */
TEST_F (ProjectSerializationRoundtripTest, LoadProjectWithVst3Plugin)
{
  auto juce_desc = findFirstBundledVst3Plugin ();
  ASSERT_NE (juce_desc, nullptr)
    << "No bundled VST3 plugins found - check BUNDLED_VST3_SEARCH_PATHS";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  saveLoadSaveRoundtripWithPlugin (*descriptor);
}

} // namespace zrythm::controllers
