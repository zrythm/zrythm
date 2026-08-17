// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for project serialization (save/load roundtrips).
 *
 * These tests verify the full save -> load -> save cycle works correctly.
 */

#include <cstring>
#include <optional>

#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "controllers/project_json_serializer.h"
#include "controllers/project_loader.h"
#include "controllers/project_saver.h"
#include "plugins/faust/faust_registry.h"
#include "plugins/plugin_descriptor.h"
#include "structure/project/project_path_provider.h"
#include "structure/project/project_ui_state.h"
#include "utils/qt.h"

#include <QSignalSpy>

#include "helpers/mock_plugin_host_window.h"
#include "helpers/project_fixture.h"
#include "helpers/project_json_comparators.h"
#include "helpers/qt_helpers.h"
#include "helpers/test_plugin_finder.h"

#include <base/source/fstreamer.h>
#include <gtest/gtest.h>
#include <juce_core/juce_core.h>
#include <nlohmann/json.hpp>
#include <public.sdk/source/vst/utility/memoryibstream.h>
#include <public.sdk/source/vst/vstpresetfile.h>

namespace zrythm::controllers
{

/**
 * @brief Decodes a CLAP test plugin state blob.
 *
 * CLAP test plugin state blobs are JSON text written by the fixture's
 * stateSave; the Level parameter value is the "gain" key.
 */
static std::map<std::string, float>
decode_test_clap_state (const QByteArray &raw_state)
{
  const auto j = nlohmann::json::parse (
    std::string_view (
      raw_state.constData (), static_cast<size_t> (raw_state.size ())),
    nullptr, false);
  if (j.is_discarded () || !j.contains ("gain"))
    return {};
  return {
    { "level", j["gain"].get<float> () }
  };
}

/**
 * @brief Decodes a VST3 test plugin state blob.
 *
 * VST3 state blobs use the standard .vstpreset container format
 * (Vst::PresetFile); the controller state chunk holds length-prefixed JSON
 * text written by the test plugin via IBStreamer, with the Level parameter
 * value under the "gain" key.
 */
static std::map<std::string, float>
decode_test_vst3_state (const QByteArray &raw_state)
{
  Steinberg::ResizableMemoryIBStream stream;
  stream.write (
    const_cast<char *> (raw_state.constData ()),
    static_cast<Steinberg::int32> (raw_state.size ()), nullptr);
  stream.rewind ();
  Steinberg::Vst::PresetFile preset (&stream);
  if (!preset.readChunkList () || !preset.seekToControllerState ())
    return {};
  Steinberg::IBStreamer streamer (&stream, kLittleEndian);
  Steinberg::int32      length = 0;
  if (!streamer.readInt32 (length) || length <= 0)
    return {};
  std::string json_text (static_cast<size_t> (length), '\0');
  if (streamer.readRaw (json_text.data (), length) != length)
    return {};
  const auto j = nlohmann::json::parse (json_text, nullptr, false);
  if (j.is_discarded () || !j.contains ("gain"))
    return {};
  return {
    { "level", j["gain"].get<float> () }
  };
}

/**
 * @brief Extracts and decodes the test plugin state from the project JSON.
 */
static std::map<std::string, float>
get_test_plugin_state_from_json (
  const nlohmann::json           &project_json,
  plugins::Protocol::ProtocolType protocol)
{
  const auto &plugin_reg = project_json["projectData"]["registry"]["plugins"];
  if (plugin_reg.empty ())
    return {};
  const auto &plugin_json = plugin_reg[0];
  if (!plugin_json.contains ("state"))
    return {};

  const auto state_str = plugin_json["state"].get<std::string> ();
  const auto raw =
    QByteArray::fromBase64 (QByteArray::fromStdString (state_str));
  if (protocol == plugins::Protocol::ProtocolType::VST3)
    return decode_test_vst3_state (raw);
  return decode_test_clap_state (raw);
}

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

  void process_events_with_condition_or_timeout (
    structure::project::Project          &project,
    std::optional<std::function<bool ()>> condition)
  {
    auto * mock_hw = dynamic_cast<
      test_helpers::ThreadedMockHardwareAudioInterface *> (hw_interface_.get ());
    project.engine ()->activate ();
    project.engine ()->graph_dispatcher ().recalc_graph (false);
    project.engine ()->set_running (true);

    const auto initial_count = mock_hw->process_call_count ();
    if (condition.has_value ())
      {
        process_events_until_true (*condition);
      }
    else
      {
        // Wait for JUCE timers and UI signals to fire (40ms period).
        process_events_until_timeout ();
      }

    // Ensure at least 1 cycle is processed before returning.
    process_events_until_true ([mock_hw, initial_count] () {
      return mock_hw->process_call_count () >= initial_count + 1;
    });

    project.engine ()->set_running (false);
    project.engine ()->deactivate ();
  }

  void process_until_true (
    structure::project::Project  &project,
    const std::function<bool ()> &condition)
  {
    process_events_with_condition_or_timeout (project, condition);
  }

  /**
   * @brief Starts the engine, pumps events for long enough for JUCE timers
   * to fire (40ms period), then stops it.
   *
   * Verifies that at least one processing cycle completed (no crash).
   */
  void process_until_timeout (structure::project::Project &project)
  {
    process_events_with_condition_or_timeout (project, std::nullopt);
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
   * @param param_label Label of the DSP parameter to modify before saving.
   * @param test_value Normalized (0-1) value to set the parameter to.
   * @param post_load_verifier Optional callback invoked at the very end
   *   (after the resave comparison, so it may freely modify the loaded
   *   plugin) with the loaded project and plugin, e.g. to verify that
   *   parameter values reach the DSP after loading.
   */
  void save_load_save_roundtrip_with_plugin (
    const plugins::PluginDescriptor &descriptor,
    std::optional<size_t>            expected_param_count = std::nullopt,
    const utils::Utf8String         &param_label = u8"Level",
    float                            test_value = 0.75f,
    std::function<void (ProjectBundle &, const plugins::PluginPtrVariant &)>
      post_load_verifier = nullptr)
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
    process_events_until_true ([&] () { return instantiation_finished; });
    EXPECT_TRUE (instantiation_finished)
      << "Plugin instantiation did not complete";

    // === Step 1b: Capture original plugin state for later comparison ===
    auto &original_registry = original_bundle->project->get_registry ();
    ASSERT_EQ (original_registry.count_matching<plugins::Plugin> (), 1);

    plugins::PluginPtrVariant original_plugin_var;
    original_registry.for_each_matching<plugins::Plugin> (
      [&] (plugins::Plugin &pl) {
        original_plugin_var =
          utils::convert_to_variant_qobj<plugins::PluginPtrVariant> (&pl);
      });

    // Capture param count and verify it matches expectations
    const size_t original_param_count = std::visit (
      [] (auto &&pl) { return pl->get_parameters ().size (); },
      original_plugin_var);
    if (expected_param_count.has_value ())
      {
        EXPECT_EQ (original_param_count, *expected_param_count)
          << "Plugin param count after import: expected="
          << *expected_param_count << " actual=" << original_param_count;
      }

    // Capture param snapshots for roundtrip verification.
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
    process_until_timeout (*original_bundle->project);

    // Find the target DSP parameter and set it to a known non-default value.
    dsp::ProcessorParameter * gain_param = nullptr;
    dsp::ParameterRange       gain_range{};
    std::visit (
      [&] (auto &&pl) {
        for (const auto &param_ref : pl->get_parameters ())
          {
            auto * param =
              param_ref.template get_object_as<dsp::ProcessorParameter> ();
            if (param->label ().contains (param_label.to_qstring ()))
              {
                gain_param = param;
                gain_range = param->range ();
                param->setBaseValue (test_value);
                original_param_values.push_back (
                  { param->get_uuid (), param->label (), test_value });
              }
          }
      },
      original_plugin_var);

    // Process again so the baseValue changes propagate to the underlying
    // plugin (JUCE/CLAP) before the state blob is captured during save.
    // Wait until the value stabilizes to avoid a race where the plugin
    // reports back its stale default before processing the new value.
    if (gain_param != nullptr)
      {
        process_until_true (*original_bundle->project, [&] () {
          return std::abs (gain_param->baseValue () - test_value) < 0.01f;
        });
      }
    else
      {
        process_until_timeout (*original_bundle->project);
      }

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

    // Capture the original save JSON for later registry comparison
    const auto original_json = nlohmann::json::parse (
      ProjectSaver::get_existing_uncompressed_text (project_dir_));

    // === Step 2b: Verify state data is present and contains correct values ===
    {
      const auto &plugin_reg =
        original_json["projectData"]["registry"]["plugins"];
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

          // Decode the state blob and verify the parameter values. For VST3,
          // the blob is a .vstpreset container; for CLAP, the raw bytes
          // written by the plugin's stateSave.
          if (
            descriptor.protocol_ == plugins::Protocol::ProtocolType::CLAP
            || descriptor.protocol_ == plugins::Protocol::ProtocolType::VST3)
            {
              auto saved_state = get_test_plugin_state_from_json (
                original_json, descriptor.protocol_);
              EXPECT_FALSE (saved_state.empty ())
                << "State blob should contain at least one parameter";
              if (gain_param != nullptr)
                {
                  for (const auto &[path, value] : saved_state)
                    {
                      const float normalized = gain_range.convertTo0To1 (value);
                      EXPECT_NEAR (normalized, test_value, 0.001f)
                        << "State blob parameter '" << path << "' plain value "
                        << value << " (normalized=" << normalized
                        << ") should match the test value " << test_value;
                    }
                }
            }
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

    // === Step 4: Get loaded plugin reference ===
    auto &loaded_registry = loaded_bundle->project->get_registry ();
    EXPECT_GE (loaded_registry.count_matching<plugins::Plugin> (), 1);

    plugins::PluginPtrVariant loaded_plugin_var;
    loaded_registry.for_each_matching<plugins::Plugin> ([&] (plugins::Plugin &pl) {
      loaded_plugin_var =
        utils::convert_to_variant_qobj<plugins::PluginPtrVariant> (&pl);
    });

    // === Step 4a: Process and wait for parameter sync to settle ===
    // All snapshots must have a matching param with the expected value.
    // When there are no plugin DSP params to verify (e.g. internal plugins),
    // just process for a few cycles instead.
    {
      EXPECT_NO_FATAL_FAILURE ({
        if (original_param_values.empty ())
          {
            process_until_timeout (*loaded_bundle->project);
          }
        else
          {
            process_until_true (*loaded_bundle->project, [&] () {
              return std::visit (
                [&] (auto &&pl) {
                  return std::ranges::all_of (
                    original_param_values, [&] (const auto &snap) {
                      const auto param_it = std::ranges::find (
                        pl->get_parameters (), snap.uuid,
                        [] (const auto &param_ref) {
                          return param_ref
                            .template get_object_as<dsp::ProcessorParameter> ()
                            ->get_uuid ();
                        });
                      if (param_it == pl->get_parameters ().end ())
                        return true;
                      const auto * param = param_it->template get_object_as<
                        dsp::ProcessorParameter> ();
                      return utils::math::floats_near (
                        param->baseValue (), snap.value, 0.01f);
                    });
                },
                loaded_plugin_var);
            });
          }
      }) << "Processing a plugin after loading a project should not crash";
    }

    // === Step 4b: Verify param count consistency (no duplication) ===
    const size_t loaded_param_count = std::visit (
      [] (auto &&pl) { return pl->get_parameters ().size (); },
      loaded_plugin_var);
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
                      << utils::to_std_string (original_snap.name);
                    break;
                  }
              }
            EXPECT_TRUE (found)
              << "Parameter from original not found in loaded plugin: "
              << utils::to_std_string (original_snap.name);
          }
      },
      loaded_plugin_var);

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

    // === Step 6: Optional post-load verification ===
    // Runs after the resave comparison so it may freely modify the loaded
    // plugin without affecting the roundtrip JSON.
    if (post_load_verifier != nullptr)
      {
        post_load_verifier (*loaded_bundle, loaded_plugin_var);
      }
  }
};

/**
 * @brief Tests full save -> load -> save roundtrip with an internal plugin.
 *
 * Internal plugins have 2 built-in params (bypass + gain) and no DSP params.
 */
TEST_F (
  ProjectSerializationRoundtripTest,
  SaveLoadSaveRoundtripWithInternalPlugin)
{
  auto descriptor = std::make_unique<plugins::PluginDescriptor> ();
  descriptor->name_ = u8"Test Roundtrip Plugin";
  descriptor->protocol_ = plugins::Protocol::ProtocolType::Internal;

  // 2 built-in (bypass + gain), no DSP params
  save_load_save_roundtrip_with_plugin (*descriptor, 2);
}

/**
 * @brief Tests save -> load -> save with a bundled Faust plugin, verifying
 * that the restored parameter value reaches the DSP after loading.
 *
 * Uses White Noise (a generator, so no audio input is needed) with Amp set
 * to maximum (normalized 1.0 = 10 dB; the default is -10 dB). After loading,
 * output must be loud without touching the parameter (initial push), and
 * lowering Amp afterwards must make it quiet (live propagation).
 */
TEST_F (ProjectSerializationRoundtripTest, SaveLoadSaveRoundtripWithFaustPlugin)
{
  const auto * info =
    plugins::faust::find_faust_plugin_by_key (u8"zrythm.faust.white_noise");
  ASSERT_NE (info, nullptr);
  auto descriptor = plugins::faust::make_faust_plugin_descriptor (*info);
  ASSERT_NE (descriptor, nullptr);

  auto verify_output_follows_param =
    [] (ProjectBundle &loaded_bundle, const plugins::PluginPtrVariant &pl_var) {
      std::visit (
        [&] (auto &&pl) {
          // Find the Amp param and the audio output port
          dsp::ProcessorParameter * amp_param = nullptr;
          dsp::AudioPort *          audio_out = nullptr;
          for (const auto &param_ref : pl->get_parameters ())
            {
              auto * param =
                param_ref.template get_object_as<dsp::ProcessorParameter> ();
              if (param->label ().contains (u8"Amp"))
                amp_param = param;
            }
          for (const auto &port_ref : pl->get_all_output_ports ())
            {
              if (
                auto * port = port_ref.template get_object_as<dsp::AudioPort> ())
                audio_out = port;
            }
          ASSERT_NE (amp_param, nullptr);
          ASSERT_NE (audio_out, nullptr);

          // Process the plugin directly (the engine is deactivated after the
          // resave, so no audio thread races)
          pl->prepare_for_processing (
            nullptr, units::sample_rate (48000), units::samples (256));
          const dsp::graph::ProcessBlockInfo time_nfo{
            .transport_position_ = units::samples (0),
            .buffer_offset_ = units::samples (0),
            .nframes_ = units::samples (256),
          };
          auto &transport = *loaded_bundle.project->transport_;
          auto &tempo_map = loaded_bundle.project->tempo_map ();
          auto  transport_snapshot = transport.get_snapshot ();

          const auto process_blocks = [&] (int count) {
            for (int i = 0; i < count; ++i)
              pl->process_block (time_nfo, transport_snapshot, tempo_map);
          };
          const auto measure_peak = [&] () {
            float peak = 0.f;
            for (int ch = 0; ch < audio_out->buffers ()->getNumChannels (); ++ch)
              {
                peak = std::max (
                  peak, audio_out->buffers ()->getMagnitude (ch, 0, 256));
              }
            return peak;
          };

          // Initial push: the restored 10 dB value must reach the DSP without
          // touching the param (si.smoo needs time to settle)
          process_blocks (100);
          const float loud_peak = measure_peak ();

          // Live propagation: lowering Amp must make the output quiet
          amp_param->setBaseValue (0.f);
          process_blocks (100);
          const float quiet_peak = measure_peak ();

          EXPECT_GT (loud_peak, quiet_peak * 100.f)
            << "Faust param mapping broken after project load (loud="
            << loud_peak << " quiet=" << quiet_peak << ")";
        },
        pl_var);
    };

  // 2 built-in (bypass + gain) + 1 DSP param (Amp) = 3
  save_load_save_roundtrip_with_plugin (
    *descriptor, 3, u8"Amp", 1.0f, verify_output_follows_param);
}

/**
 * @brief Tests full save -> load -> save roundtrip with a bundled VST3 plugin.
 *
 * Uses the Test Gain plugin which has 1 plugin parameter (Gain).
 * Expected params: 2 built-in (bypass + gain) + 1 plugin param (Gain) = 3.
 */
TEST_F (ProjectSerializationRoundtripTest, LoadProjectWithVst3Plugin)
{
  static const juce::String kTargetPluginName = "Test Gain";
  auto                      juce_desc =
    test_helpers::find_test_vst3_plugin_by_name (kTargetPluginName);
  ASSERT_NE (juce_desc, nullptr)
    << "Test VST3 plugin '" << kTargetPluginName
    << "' not found - check TEST_VST3_SEARCH_PATHS";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  // 2 built-in (bypass + gain) + 2 plugin params (Level, CC Assign)
  save_load_save_roundtrip_with_plugin (*descriptor, 4);
}

/**
 * @brief Tests full save -> load -> save roundtrip with a CLAP test plugin.
 *
 * Uses the Test Gain plugin which has 1 plugin parameter (Gain).
 * Expected params: 2 built-in (bypass + gain) + 1 CLAP param (Gain) = 3.
 * This test catches bug 5: CLAP params not exposed as Zrythm
 * ProcessorParameters.
 */
TEST_F (ProjectSerializationRoundtripTest, LoadProjectWithClapPlugin)
{
  static const juce::String kTargetPluginName = "Test Gain";
  auto                      juce_desc =
    test_helpers::find_test_clap_plugin_by_name (kTargetPluginName);
  ASSERT_NE (juce_desc, nullptr)
    << "Test CLAP plugin '" << kTargetPluginName
    << "' not found - check TEST_CLAP_SEARCH_PATHS";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  // 2 built-in (bypass + gain) + 1 plugin param (Gain)
  save_load_save_roundtrip_with_plugin (*descriptor, 3);
}

/**
 * @brief Verifies that the plugin state blob is preserved on reload when it
 * diverges from the host's baseValue.
 *
 * When the host's baseValue for a parameter diverges from the plugin's state
 * blob, a save-reload cycle must preserve the plugin's state blob. The host
 * must apply the state blob as the source of truth rather than overwriting it
 * with its own baseValue.
 *
 * Strategy: save with Level=0.75, modify the JSON baseValue to 0.25,
 * reload, process, then verify the resaved state blob still has 0.75.
 */
TEST_F (
  ProjectSerializationRoundtripTest,
  ClapStatePreservedOnReloadWithDivergedValue)
{
  static const juce::String kTargetPluginName = "Test Gain";
  auto                      juce_desc =
    test_helpers::find_test_clap_plugin_by_name (kTargetPluginName);
  ASSERT_NE (juce_desc, nullptr)
    << "Bundled CLAP plugin '" << kTargetPluginName << "' not found";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  // === Step 1: Create project, set Level to 0.75, save ===
  auto bundle = std::make_unique<ProjectBundle> ();
  bundle->project = create_minimal_project ();
  ASSERT_NE (bundle->project, nullptr);
  bundle->project->add_default_tracks ();
  bundle->ui_state = utils::make_qobject_unique<
    structure::project::ProjectUiState> (*bundle->project, *app_settings_);
  bundle->undo_stack = utils::make_qobject_unique<undo::UndoStack> (
    [proj = bundle->project.get ()] (
      const std::function<void ()> &action, bool recalculate_graph) {
      proj->engine ()->execute_function_with_paused_processing_synchronously (
        action, recalculate_graph);
    },
    nullptr);

  bool instantiation_finished = false;
  auto handler = [&instantiation_finished] (plugins::PluginUuidReference) {
    instantiation_finished = true;
  };

  auto * tracklist = bundle->project->tracklist ();
  auto   track_creator = std::make_unique<actions::TrackCreator> (
    *bundle->undo_stack, *bundle->project->track_factory_,
    *tracklist->collection (), *tracklist->trackRouting (),
    *tracklist->singletonTracks (), bundle->project.get ());

  auto importer = std::make_unique<actions::PluginImporter> (
    *bundle->undo_stack, *bundle->project->plugin_factory_, *track_creator,
    std::move (handler), bundle->project.get ());

  importer->importPluginToNewTrack (&*descriptor);

  process_events_until_true ([&] () { return instantiation_finished; });
  ASSERT_TRUE (instantiation_finished);

  process_until_timeout (*bundle->project);

  constexpr float     correct_value = 0.75f;
  constexpr float     stale_value = 0.25f;
  dsp::ParameterRange gain_range{};

  auto &plugin_reg = bundle->project->get_registry ();
  ASSERT_EQ (plugin_reg.count_matching<plugins::Plugin> (), 1);
  plugins::PluginPtrVariant plugin_var;
  plugin_reg.for_each_matching<plugins::Plugin> ([&] (plugins::Plugin &pl) {
    plugin_var = utils::convert_to_variant_qobj<plugins::PluginPtrVariant> (&pl);
  });
  dsp::ProcessorParameter * gain_param = nullptr;
  std::visit (
    [&] (auto &&pl) {
      for (const auto &param_ref : pl->get_parameters ())
        {
          auto * param =
            param_ref.template get_object_as<dsp::ProcessorParameter> ();
          if (param->label ().contains (u8"Level"))
            {
              gain_range = param->range ();
              gain_param = param;
              param->setBaseValue (correct_value);
            }
        }
    },
    plugin_var);
  ASSERT_NE (gain_param, nullptr);

  process_until_true (*bundle->project, [&] () {
    return std::abs (gain_param->baseValue () - correct_value) < 0.01f;
  });

  auto save_future = ProjectSaver::save (
    *bundle->project, *bundle->ui_state, *bundle->undo_stack, TEST_APP_VERSION,
    project_dir_, false);
  test_helpers::waitForFutureWithEvents (save_future);
  ASSERT_TRUE (save_future.isFinished ());

  bundle->project->engine ()->set_running (false);
  importer.reset ();
  track_creator.reset ();
  bundle.reset ();

  // === Step 2: Verify saved state blob has correct_value ===
  auto project_file_path =
    project_dir_
    / structure::project::ProjectPathProvider::get_path (
      structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
  auto saved_json = nlohmann::json::parse (
    ProjectSaver::get_existing_uncompressed_text (project_dir_));

  auto original_state = get_test_plugin_state_from_json (
    saved_json, plugins::Protocol::ProtocolType::CLAP);
  ASSERT_FALSE (original_state.empty ());
  for (const auto &[path, value] : original_state)
    {
      float normalized = gain_range.convertTo0To1 (value);
      EXPECT_NEAR (normalized, correct_value, 0.01f)
        << "Original state blob should have correct_value";
    }

  // === Step 3: Modify baseValue in JSON to create a divergence ===
  auto &param_reg = saved_json["projectData"]["registry"]["parameters"];
  bool  modified = false;
  for (auto &param_json : param_reg)
    {
      if (
        param_json.contains ("label")
        && param_json["label"].get<std::string> ().find ("Level")
             != std::string::npos)
        {
          float original = param_json["baseValue"].get<float> ();
          ASSERT_NEAR (original, correct_value, 0.01f)
            << "Level baseValue should be correct_value before modification";
          param_json["baseValue"] = stale_value;
          modified = true;
          break;
        }
    }
  ASSERT_TRUE (modified) << "Could not find Level parameter to modify";

  // Write tampered JSON back (compressed)
  {
    const auto       json_str = saved_json.dump (2);
    std::string_view json_sv (json_str);
    QByteArray       src_data (
      json_sv.data (), static_cast<qsizetype> (json_sv.size ()));
    char * compressed_json{};
    size_t compressed_size{};
    ProjectSaver::compress (&compressed_json, &compressed_size, src_data);
    auto tampered_path =
      project_dir_
      / structure::project::ProjectPathProvider::get_path (
        structure::project::ProjectPathProvider::ProjectPath::ProjectFile);
    utils::io::set_file_contents (
      tampered_path, compressed_json, compressed_size);
    free (compressed_json);
  }

  // === Step 4: Reload from tampered JSON ===
  auto load_result = ProjectLoader::load_from_directory (project_dir_);
  load_result.waitForFinished ();
  ASSERT_FALSE (load_result.isCanceled ());

  auto reload_bundle = std::make_unique<ProjectBundle> ();
  reload_bundle->project = create_minimal_project ();
  reload_bundle->ui_state =
    utils::make_qobject_unique<structure::project::ProjectUiState> (
      *reload_bundle->project, *app_settings_);
  reload_bundle->undo_stack = utils::make_qobject_unique<undo::UndoStack> (
    [proj = reload_bundle->project.get ()] (
      const std::function<void ()> &action, bool recalculate_graph) {
      proj->engine ()->execute_function_with_paused_processing_synchronously (
        action, recalculate_graph);
    },
    nullptr);

  ProjectLoader::deserialize (
    load_result.result ().json, *reload_bundle->project,
    *reload_bundle->ui_state, *reload_bundle->undo_stack);

  // Find the Level parameter in the reloaded plugin for condition checking
  auto &reload_registry = reload_bundle->project->get_registry ();
  ASSERT_EQ (reload_registry.count_matching<plugins::Plugin> (), 1);
  dsp::ProcessorParameter * reload_gain_param = nullptr;
  plugins::PluginPtrVariant reload_plugin_var;
  reload_registry.for_each_matching<plugins::Plugin> ([&] (plugins::Plugin &pl) {
    reload_plugin_var =
      utils::convert_to_variant_qobj<plugins::PluginPtrVariant> (&pl);
  });
  std::visit (
    [&] (auto &&pl) {
      for (const auto &param_ref : pl->get_parameters ())
        {
          auto * param =
            param_ref.template get_object_as<dsp::ProcessorParameter> ();
          if (param->label ().contains (u8"Level"))
            reload_gain_param = param;
        }
    },
    reload_plugin_var);
  ASSERT_NE (reload_gain_param, nullptr);

  // Process so the audio thread runs and the host has a chance to apply
  // baseValues from the plugin state blob.
  EXPECT_NO_FATAL_FAILURE ({
    process_until_true (*reload_bundle->project, [&] () {
      return utils::math::floats_near (
        reload_gain_param->baseValue (), correct_value, 0.01f);
    });
  });

  // === Step 5: Save reloaded project and verify state blob ===
  auto verify_dir = project_dir_ / "verify";
  auto verify_future = ProjectSaver::save (
    *reload_bundle->project, *reload_bundle->ui_state,
    *reload_bundle->undo_stack, TEST_APP_VERSION, verify_dir, false);
  test_helpers::waitForFutureWithEvents (verify_future);
  ASSERT_TRUE (verify_future.isFinished ());

  auto verify_json = nlohmann::json::parse (
    ProjectSaver::get_existing_uncompressed_text (verify_dir));
  auto final_state = get_test_plugin_state_from_json (
    verify_json, plugins::Protocol::ProtocolType::CLAP);
  ASSERT_FALSE (final_state.empty ());

  for (const auto &[path, value] : final_state)
    {
      float normalized = gain_range.convertTo0To1 (value);
      EXPECT_NEAR (normalized, correct_value, 0.001f)
        << "State blob should preserve correct_value (" << correct_value
        << "), not stale_value (" << stale_value
        << "). Plugin state blob should be the source of truth over the host's baseValue.";
    }
}

/**
 * @brief Parametrized over the test GUI plugins of each native protocol.
 */
class ProjectPluginUiRestoreTest
    : public ProjectSerializationRoundtripTest,
      public testing::WithParamInterface<std::string_view>
{
};

/**
 * @brief A plugin saved with a visible editor gets its editor restored on
 * project load, but only after loading finishes.
 *
 * Native window creation requires a fully loaded project, so the editor
 * restore triggered by a plugin deserialized with visible UI is delivered
 * via a queued invocation rather than synchronously during deserialization.
 */
TEST_P (ProjectPluginUiRestoreTest, LoadWithVisibleUiRestoresEditorDeferred)
{
  const auto plugin_name = juce::String::fromUTF8 (
    GetParam ().data (), static_cast<int> (GetParam ().size ()));

  auto juce_desc = test_helpers::find_test_clap_plugin_by_name (plugin_name);
  if (juce_desc == nullptr)
    juce_desc = test_helpers::find_test_vst3_plugin_by_name (plugin_name);
  ASSERT_NE (juce_desc, nullptr)
    << "Test GUI plugin '" << plugin_name << "' not found";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  // === Step 1: Create a project with the plugin's editor shown ===
  auto original_window_state =
    std::make_shared<test_helpers::MockPluginHostWindowState> ();
  auto bundle = std::make_unique<ProjectBundle> ();
  bundle->project = create_minimal_project (
    test_helpers::make_mock_plugin_host_window_factory (original_window_state));
  ASSERT_NE (bundle->project, nullptr);
  bundle->project->add_default_tracks ();
  bundle->ui_state = utils::make_qobject_unique<
    structure::project::ProjectUiState> (*bundle->project, *app_settings_);
  bundle->undo_stack = utils::make_qobject_unique<undo::UndoStack> (
    [proj = bundle->project.get ()] (
      const std::function<void ()> &action, bool recalculate_graph) {
      proj->engine ()->execute_function_with_paused_processing_synchronously (
        action, recalculate_graph);
    },
    nullptr);

  bool instantiation_finished = false;
  auto handler = [&instantiation_finished] (plugins::PluginUuidReference) {
    instantiation_finished = true;
  };

  auto * tracklist = bundle->project->tracklist ();
  auto   track_creator = std::make_unique<actions::TrackCreator> (
    *bundle->undo_stack, *bundle->project->track_factory_,
    *tracklist->collection (), *tracklist->trackRouting (),
    *tracklist->singletonTracks (), bundle->project.get ());
  auto importer = std::make_unique<actions::PluginImporter> (
    *bundle->undo_stack, *bundle->project->plugin_factory_, *track_creator,
    std::move (handler), bundle->project.get ());

  importer->importPluginToNewTrack (&*descriptor);
  process_events_until_true ([&] () { return instantiation_finished; });
  ASSERT_TRUE (instantiation_finished);

  auto &registry = bundle->project->get_registry ();
  ASSERT_EQ (registry.count_matching<plugins::Plugin> (), 1);
  plugins::Plugin * plugin = nullptr;
  registry.for_each_matching<plugins::Plugin> ([&] (plugins::Plugin &pl) {
    plugin = &pl;
  });
  ASSERT_NE (plugin, nullptr);

  plugin->setUiVisible (true);
  ASSERT_TRUE (original_window_state->visible)
    << "Editor did not appear after requesting visible UI";

  // === Step 2: Serialize, then deserialize into a fresh project ===
  const auto json = ProjectJsonSerializer::serialize (
    *bundle->project, *bundle->ui_state, *bundle->undo_stack, TEST_APP_VERSION,
    "test");

  importer.reset ();
  track_creator.reset ();
  bundle.reset ();

  auto loaded_window_state =
    std::make_shared<test_helpers::MockPluginHostWindowState> ();
  auto loaded_bundle = std::make_unique<ProjectBundle> ();
  loaded_bundle->project = create_minimal_project (
    test_helpers::make_mock_plugin_host_window_factory (loaded_window_state));
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

  ProjectLoader::deserialize (
    json, *loaded_bundle->project, *loaded_bundle->ui_state,
    *loaded_bundle->undo_stack);

  // Native windows must not be created during deserialization: the editor
  // restore is queued so that project loading finishes first
  EXPECT_EQ (loaded_window_state->window, nullptr);
  EXPECT_FALSE (loaded_window_state->visible);
  process_events_until_true ([&] { return loaded_window_state->visible; });
}

INSTANTIATE_TEST_SUITE_P (
  GuiPlugins,
  ProjectPluginUiRestoreTest,
  testing::
    Values (std::string_view ("Test GUI CLAP"), std::string_view ("Test GUI")),
  [] (const testing::TestParamInfo<std::string_view> &param_info) {
    return param_info.param.find ("CLAP") != std::string_view::npos
             ? std::string ("Clap")
             : std::string ("Vst3");
  });

} // namespace zrythm::controllers
