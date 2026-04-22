// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for project serialization (save/load roundtrips).
 *
 * These tests verify the full save -> load -> save cycle works correctly.
 */

#include <optional>

#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/plugin_factory.h"
#include "structure/project/project_path_provider.h"
#include "structure/project/project_ui_state.h"

#include <QSignalSpy>

#include "helpers/bundled_plugin_finder.h"
#include "helpers/project_fixture.h"
#include "helpers/project_json_comparators.h"
#include "helpers/qt_helpers.h"

#include <gtest/gtest.h>

namespace zrythm::controllers
{

/**
 * @brief Owning wrapper that holds a project together with its UI state and
 * undo stack, ensuring correct destruction order (undo stack -> ui state ->
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

class ProjectSerializationRoundtripTest : public test_helpers::ProjectTestFixture
{
protected:
  static constexpr utils::Version TEST_APP_VERSION{ 2, 0, {} };

  /**
   * @brief Starts the engine, waits for a few processing cycles, then stops it.
   */
  void process_a_few_cycles (structure::project::Project &project)
  {
    auto * mock_hw = dynamic_cast<
      test_helpers::ThreadedMockHardwareAudioInterface *> (hw_interface_.get ());
    project.engine ()->activate ();
    project.engine ()->graph_dispatcher ().recalc_graph (false);
    project.engine ()->set_running (true);
    const auto initial_count = mock_hw->process_call_count ();
    while (mock_hw->process_call_count () - initial_count < 10)
      QCoreApplication::processEvents (QEventLoop::AllEvents, 50);
    project.engine ()->set_running (false);
    project.engine ()->deactivate ();
  }

  /**
   * @brief Runs the full save -> load -> resave roundtrip cycle with a plugin.
   *
   * Creates a project, imports a plugin via PluginImporter, saves, loads into a
   * new instance, verifies the plugin runs correctly, and resaves.
   *
   * Verifications performed (for all plugin types):
   * 1. Registry sizes and IDs match between original save and resave
   * 2. Plugin param counts are consistent (no duplication)
   * 3. Parameter values are preserved through roundtrip
   * 4. Plugin state data is present in the serialized JSON
   *
   * @param descriptor The plugin descriptor to import.
   * @param expected_param_count Expected number of parameters after import
   *   (including bypass and gain). Use std::nullopt to skip this check.
   */
  void save_load_save_roundtrip_with_plugin (
    const plugins::PluginDescriptor &descriptor,
    std::optional<size_t>            expected_param_count = std::nullopt)
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

    // === Step 1b: Capture original plugin state for later comparison ===
    auto &original_plugin_registry =
      original_bundle->project->get_plugin_registry ();
    ASSERT_EQ (original_plugin_registry.get_hash_map ().size (), 1);

    auto original_plugin_var =
      original_plugin_registry.get_hash_map () | std::views::values
      | std::views::take (1);
    auto original_plugin_it = original_plugin_var.begin ();
    ASSERT_NE (original_plugin_it, original_plugin_var.end ());

    // Capture param count and verify it matches expectations
    const size_t original_param_count = std::visit (
      [] (auto &&pl) { return pl->get_parameters ().size (); },
      *original_plugin_it);
    if (expected_param_count.has_value ())
      {
        EXPECT_EQ (original_param_count, *expected_param_count)
          << "Plugin param count after import: expected="
          << *expected_param_count << " actual=" << original_param_count;
      }

    // Set non-default values on all params and capture snapshots
    struct ParamSnapshot
    {
      dsp::ProcessorParameter::Uuid uuid;
      QString                       name;
      float                         value;
    };
    std::vector<ParamSnapshot> original_param_values;

    // Start the engine and wait for a few processing cycles so the
    // plugin settles (some plugins reflect parameter values back via timers
    // on first load) before we set our custom values.
    process_a_few_cycles (*original_bundle->project);

    std::visit (
      [&] (auto &&pl) {
        for (const auto &param_ref : pl->get_parameters ())
          {
            auto * param =
              param_ref.template get_object_as<dsp::ProcessorParameter> ();
            param->setBaseValue (0.75f);
            original_param_values.push_back (
              { param->get_uuid (), param->label (), 0.75f });
          }
      },
      *original_plugin_it);

    // Process again so the baseValue changes propagate to the underlying
    // plugin (JUCE/CLAP) before the state blob is captured during save.
    process_a_few_cycles (*original_bundle->project);

    // === Step 2: Save the project ===
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
    EXPECT_TRUE (std::filesystem::exists (project_file_path))
      << "Project file not created: " << project_file_path;

    original_bundle->project->engine ()->set_running (false);

    // Capture the original save JSON for later registry comparison
    const auto original_json = nlohmann::json::parse (
      ProjectSaver::get_existing_uncompressed_text (project_dir_));

    // === Step 2b: Verify state data is present in the serialized JSON ===
    {
      const auto &plugin_reg =
        original_json["projectData"]["registries"]["pluginRegistry"];
      ASSERT_EQ (plugin_reg.size (), 1)
        << "Expected exactly 1 plugin in registry";
      const auto &plugin_json = plugin_reg[0];

      // All real plugin formats (VST3, CLAP) should persist state in JSON.
      // Internal plugins may not have a "state" key (no binary to persist).
      if (descriptor.protocol_ != plugins::Protocol::ProtocolType::Internal)
        {
          EXPECT_TRUE (plugin_json.contains ("state"))
            << "Plugin JSON should contain 'state' key for format: "
            << static_cast<int> (descriptor.protocol_);
        }
    }

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
      // Start processing to verify the loaded plugin works correctly
      EXPECT_NO_FATAL_FAILURE ({
        process_a_few_cycles (*loaded_bundle->project);
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

    // === Step 4b: Verify param count consistency (no duplication) ===
    const size_t loaded_param_count = std::visit (
      [] (auto &&pl) { return pl->get_parameters ().size (); },
      *loaded_plugin_it);
    EXPECT_EQ (loaded_param_count, original_param_count)
      << "Param count mismatch: original=" << original_param_count
      << " loaded=" << loaded_param_count;

    // === Step 4c: Verify parameter values are preserved ===
    std::visit (
      [&] (auto &&pl) {
        for (const auto &original_snap : original_param_values)
          {
            bool found = false;
            for (const auto &param_ref : pl->get_parameters ())
              {
                const auto * param =
                  param_ref.template get_object_as<dsp::ProcessorParameter> ();
                if (param->get_uuid () == original_snap.uuid)
                  {
                    found = true;
                    EXPECT_FLOAT_EQ (param->baseValue (), original_snap.value)
                      << "Parameter value not preserved: "
                      << original_snap.name.toStdString ();
                    break;
                  }
              }
            EXPECT_TRUE (found)
              << "Parameter from original not found in loaded plugin: "
              << original_snap.name.toStdString ();
          }
      },
      *loaded_plugin_it);

    // === Step 5: Save the loaded project again and compare registries ===
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
    EXPECT_TRUE (std::filesystem::exists (resaved_project_file_path))
      << "Resaved project file not created: " << resaved_project_file_path;

    // Compare original and resave JSON registries
    const auto resave_json = nlohmann::json::parse (
      ProjectSaver::get_existing_uncompressed_text (resave_dir));
    test_helpers::expect_registries_match (original_json, resave_json);
  }
};

/**
 * @brief Tests full save -> load -> save roundtrip with an internal plugin.
 *
 * Internal plugins have 2 built-in params (bypass + gain) and no FAUST params.
 */
TEST_F (
  ProjectSerializationRoundtripTest,
  SaveLoadSaveRoundtripWithInternalPlugin)
{
  auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
  descriptor->name_ = u8"Test Roundtrip Plugin";
  descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;

  // 2 built-in (bypass + gain), no FAUST params
  save_load_save_roundtrip_with_plugin (*descriptor, 2);
}

/**
 * @brief Tests full save -> load -> save roundtrip with a bundled VST3 plugin.
 *
 * Uses the Highpass Filter plugin which has 1 FAUST parameter (Frequency).
 * Expected params: 2 built-in (bypass + gain) + 1 JUCE bypass + 1 FAUST = 4.
 */
TEST_F (ProjectSerializationRoundtripTest, LoadProjectWithVst3Plugin)
{
  static const juce::String kTargetPluginName = "Highpass Filter";
  auto                      juce_desc =
    test_helpers::find_bundled_vst3_plugin_by_name (kTargetPluginName);
  ASSERT_NE (juce_desc, nullptr)
    << "Bundled VST3 plugin '" << kTargetPluginName
    << "' not found - check BUNDLED_VST3_SEARCH_PATHS";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  // 2 built-in (bypass + gain) + 1 JUCE bypass param + 1 FAUST param
  // (Frequency)
  save_load_save_roundtrip_with_plugin (*descriptor, 4);
}

/**
 * @brief Tests full save -> load -> save roundtrip with a bundled CLAP plugin.
 *
 * Uses the Highpass Filter plugin which has 1 FAUST parameter (Frequency).
 * Expected params: 2 built-in (bypass + gain) + 1 CLAP param (Frequency) = 3.
 * This test catches bug 5: CLAP params not exposed as Zrythm
 * ProcessorParameters.
 */
TEST_F (ProjectSerializationRoundtripTest, LoadProjectWithClapPlugin)
{
  static const juce::String kTargetPluginName = "Highpass Filter";
  auto                      juce_desc =
    test_helpers::find_bundled_clap_plugin_by_name (kTargetPluginName);
  ASSERT_NE (juce_desc, nullptr)
    << "Bundled CLAP plugin '" << kTargetPluginName
    << "' not found - check BUNDLED_CLAP_SEARCH_PATHS";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  // The CLAP plugin wraps the same JUCE AudioProcessor but does not expose
  // the JUCE bypass parameter as a separate CLAP param, so we get 3 params
  // (bypass + gain + Frequency) instead of 4 like VST3.
  save_load_save_roundtrip_with_plugin (*descriptor, 3);
}

} // namespace zrythm::controllers
