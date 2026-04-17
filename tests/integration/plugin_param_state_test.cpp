// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests verifying that parameter changes on the Zrythm side are
 * actually reflected in the plugin's internal state.
 *
 * Strategy: set a Zrythm param to value A, process, save state; then set it to
 * value B, process, save state again; assert the two states differ. This proves
 * the param change reached the plugin without needing to inspect internals.
 */

#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "plugins/plugin_descriptor.h"
#include "undo/undo_stack.h"

#include "helpers/bundled_plugin_finder.h"
#include "helpers/project_fixture.h"

#include <nlohmann/json.hpp>

namespace zrythm::tests
{

class PluginParamStateTest : public test_helpers::ProjectTestFixture
{
protected:
  /**
   * @brief Imports a plugin into a new project and returns the raw pointer.
   */
  plugins::Plugin * import_plugin (
    structure::project::Project     &project,
    undo::UndoStack                 &undo_stack,
    const plugins::PluginDescriptor &descriptor)
  {
    bool instantiation_finished = false;
    auto handler = [&instantiation_finished] (plugins::PluginUuidReference) {
      instantiation_finished = true;
    };

    auto * tracklist = project.tracklist ();
    auto   track_creator = std::make_unique<actions::TrackCreator> (
      undo_stack, *project.track_factory_, *tracklist->collection (),
      *tracklist->trackRouting (), *tracklist->singletonTracks (), &project);

    auto importer = std::make_unique<actions::PluginImporter> (
      undo_stack, *project.plugin_factory_, *track_creator, std::move (handler),
      &project);

    importer->importPluginToNewTrack (&descriptor);

    for (int i = 0; i < 100 && !instantiation_finished; ++i)
      {
        QCoreApplication::processEvents (QEventLoop::AllEvents, 50);
      }
    EXPECT_TRUE (instantiation_finished);

    auto &plugin_registry = project.get_plugin_registry ();
    EXPECT_EQ (plugin_registry.get_hash_map ().size (), 1);

    return std::visit (
      [] (auto * pl) -> plugins::Plugin * { return pl; },
      plugin_registry.get_hash_map ().begin ()->second);
  }
};

/**
 * @brief Tests that changing a Zrythm-facing CLAP param actually changes
 * the plugin's internal state.
 *
 * The Highpass Filter plugin has a Frequency parameter. We set it to two
 * different values, process, and serialize to JSON each time. If the param
 * sync works, the serialized states should differ.
 */
TEST_F (PluginParamStateTest, ClapParameterChangeAffectsState)
{
  static const juce::String kTargetPluginName = "Highpass Filter";
  auto                      juce_desc =
    test_helpers::find_bundled_clap_plugin_by_name (kTargetPluginName);
  ASSERT_NE (juce_desc, nullptr)
    << "Bundled CLAP plugin '" << kTargetPluginName << "' not found";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);
  project->add_default_tracks ();

  auto undo_stack = utils::make_qobject_unique<undo::UndoStack> (
    [proj = project.get ()] (
      const std::function<void ()> &action, bool recalculate_graph) {
      proj->engine ()->execute_function_with_paused_processing_synchronously (
        action, recalculate_graph);
    },
    nullptr);

  auto * plugin = import_plugin (*project, *undo_stack, *descriptor);
  ASSERT_NE (plugin, nullptr);

  // Cast to ClapPlugin via plugin_all.h helper
  auto * clap = dynamic_cast<plugins::ClapPlugin *> (plugin);
  ASSERT_NE (clap, nullptr) << "Expected a ClapPlugin";

  // Find the Frequency parameter (should be beyond bypass+gain)
  dsp::ProcessorParameter * freq_param = nullptr;
  for (const auto &param_ref : clap->get_parameters ())
    {
      auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
      if (p->label ().contains ("Frequency"))
        {
          freq_param = p;
          break;
        }
    }
  ASSERT_NE (freq_param, nullptr) << "Frequency parameter not found";

  // Prepare for processing
  const auto sample_rate = units::sample_rate (48000);
  const auto block_size = units::samples (256);

  clap->prepare_for_processing (nullptr, sample_rate, block_size);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (0),
    .local_offset_ = units::samples (0),
    .nframes_ = units::samples (256)
  };

  // Use project's transport and tempo map
  auto &transport = *project->transport_;
  auto &tempo_map = project->tempo_map ();

  // Set Frequency to a low value, process, and serialize state
  freq_param->setBaseValue (0.1f);
  clap->process_block (time_nfo, transport, tempo_map);

  nlohmann::json json_a;
  to_json (json_a, *clap);
  ASSERT_TRUE (json_a.contains ("state"))
    << "ClapPlugin JSON should contain state key";

  // Set Frequency to a high value, process, and serialize state
  freq_param->setBaseValue (0.9f);
  clap->process_block (time_nfo, transport, tempo_map);

  nlohmann::json json_b;
  to_json (json_b, *clap);

  EXPECT_NE (json_a["state"], json_b["state"])
    << "Plugin state should differ when Frequency param changes from 0.1 to 0.9";
}

/**
 * @brief Same as ClapParameterChangeAffectsState but for VST3 (JUCE) plugins.
 */
TEST_F (PluginParamStateTest, Vst3ParameterChangeAffectsState)
{
  static const juce::String kTargetPluginName = "Highpass Filter";
  auto                      juce_desc =
    test_helpers::find_bundled_vst3_plugin_by_name (kTargetPluginName);
  ASSERT_NE (juce_desc, nullptr)
    << "Bundled VST3 plugin '" << kTargetPluginName << "' not found";

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  auto project = create_minimal_project ();
  ASSERT_NE (project, nullptr);
  project->add_default_tracks ();

  auto undo_stack = utils::make_qobject_unique<undo::UndoStack> (
    [proj = project.get ()] (
      const std::function<void ()> &action, bool recalculate_graph) {
      proj->engine ()->execute_function_with_paused_processing_synchronously (
        action, recalculate_graph);
    },
    nullptr);

  auto * plugin = import_plugin (*project, *undo_stack, *descriptor);
  ASSERT_NE (plugin, nullptr);

  auto * juce_plugin = dynamic_cast<plugins::JucePlugin *> (plugin);
  ASSERT_NE (juce_plugin, nullptr) << "Expected a JucePlugin";

  // Find the Frequency parameter
  dsp::ProcessorParameter * freq_param = nullptr;
  for (const auto &param_ref : juce_plugin->get_parameters ())
    {
      auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
      if (p->label ().contains ("Frequency"))
        {
          freq_param = p;
          break;
        }
    }
  ASSERT_NE (freq_param, nullptr) << "Frequency parameter not found";

  const auto sample_rate = units::sample_rate (48000);
  const auto block_size = units::samples (256);

  juce_plugin->prepare_for_processing (nullptr, sample_rate, block_size);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (0),
    .local_offset_ = units::samples (0),
    .nframes_ = units::samples (256)
  };

  auto &transport = *project->transport_;
  auto &tempo_map = project->tempo_map ();

  // Set Frequency to a low value, process, and serialize state
  freq_param->setBaseValue (0.1f);
  juce_plugin->process_block (time_nfo, transport, tempo_map);

  nlohmann::json json_a;
  to_json (json_a, *juce_plugin);
  ASSERT_TRUE (json_a.contains ("state"))
    << "JucePlugin JSON should contain state key";

  // Set Frequency to a high value, process, and serialize state
  freq_param->setBaseValue (0.9f);
  juce_plugin->process_block (time_nfo, transport, tempo_map);

  nlohmann::json json_b;
  to_json (json_b, *juce_plugin);

  EXPECT_NE (json_a["state"], json_b["state"])
    << "Plugin state should differ when Frequency param changes from 0.1 to 0.9";
}

} // namespace zrythm::tests
