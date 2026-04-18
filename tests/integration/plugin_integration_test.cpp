// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for plugin hosting: parameter synchronization, audio
 * output, and save/load roundtrip.
 *
 * Tests both CLAP and VST3 (JUCE) variants of each bundled plugin.
 */

#include <ostream>

#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "dsp/midi_event.h"
#include "plugins/plugin.h"
#include "plugins/plugin_descriptor.h"
#include "undo/undo_stack.h"
#include "utils/audio.h"
#include "utils/midi.h"

#include "helpers/bundled_plugin_finder.h"
#include "helpers/project_fixture.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace zrythm::tests
{

// ============================================================================
// Test parameters
// ============================================================================

struct PluginTestParam
{
  std::string_view plugin_name;
  enum class Format
  {
    CLAP,
    VST3,
  } format;
};

inline std::ostream &
operator<< (std::ostream &os, const PluginTestParam &param)
{
  const char * format =
    param.format == PluginTestParam::Format::CLAP ? "CLAP" : "VST3";
  return os << param.plugin_name << " (" << format << ")";
}

// ============================================================================
// Shared fixture base
// ============================================================================

class PluginIntegrationTestBase
    : public test_helpers::ProjectTestFixture,
      public testing::WithParamInterface<PluginTestParam>
{
  void TearDown () override
  {
    if (tracked_plugin_ != nullptr && owned_project_ != nullptr)
      {
        tracked_plugin_->release_resources ();
      }
    tracked_plugin_ = nullptr;
    owned_undo_stack_.reset ();
    owned_project_.reset ();
  }

protected:
  static void inject_midi_note_on (
    dsp::MidiPort * port,
    midi_byte_t     channel,
    midi_byte_t     note,
    midi_byte_t     velocity)
  {
    assert (port != nullptr);
    port->midi_events_.active_events_.add_note_on (
      channel, note, velocity, units::samples (0));
  }

  static dsp::MidiPort * find_midi_input_port (plugins::Plugin &plugin)
  {
    for (const auto &port_ref : plugin.get_input_ports ())
      {
        auto * midi_port = port_ref.get_object_as<dsp::MidiPort> ();
        if (midi_port != nullptr)
          return midi_port;
      }
    return nullptr;
  }

  static std::vector<dsp::AudioPort *>
  find_audio_output_ports (plugins::Plugin &plugin)
  {
    std::vector<dsp::AudioPort *> result;
    for (const auto &port_ref : plugin.get_output_ports ())
      {
        auto * audio_port = port_ref.get_object_as<dsp::AudioPort> ();
        if (audio_port != nullptr)
          result.push_back (audio_port);
      }
    return result;
  }

  static dsp::graph::EngineProcessTimeInfo
  make_time_info (uint32_t num_frames = 256)
  {
    return dsp::graph::EngineProcessTimeInfo{
      .g_start_frame_ = units::samples (0),
      .g_start_frame_w_offset_ = units::samples (0),
      .local_offset_ = units::samples (0),
      .nframes_ = units::samples (num_frames),
    };
  }

  static dsp::ProcessorParameter *
  find_first_plugin_param (plugins::Plugin &plugin)
  {
    auto * bypass = plugin.bypassParameter ();
    auto * gain = plugin.gainParameter ();
    for (const auto &param_ref : plugin.get_parameters ())
      {
        auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
        if (p != bypass && p != gain && p->automatable ())
          return p;
      }
    return nullptr;
  }

  plugins::Plugin * import_and_prepare_plugin (const PluginTestParam &param)
  {
    const auto plugin_name = juce::String::fromUTF8 (param.plugin_name.data ());
    const char * format_str =
      param.format == PluginTestParam::Format::CLAP ? "CLAP" : "VST3";

    const auto juce_desc = [&] () {
      if (param.format == PluginTestParam::Format::CLAP)
        {
          return test_helpers::find_bundled_clap_plugin_by_name (plugin_name);
        }

      return test_helpers::find_bundled_vst3_plugin_by_name (plugin_name);
    }();
    EXPECT_NE (juce_desc, nullptr)
      << "Bundled " << format_str << " plugin '" << param.plugin_name
      << "' not found";

    const auto descriptor =
      plugins::PluginDescriptor::from_juce_description (*juce_desc);
    EXPECT_NE (descriptor, nullptr);

    auto project = create_minimal_project ();
    EXPECT_NE (project, nullptr);
    project->add_default_tracks ();

    auto undo_stack = utils::make_qobject_unique<undo::UndoStack> (
      [proj = project.get ()] (
        const std::function<void ()> &action, bool recalculate_graph) {
        proj->engine ()->execute_function_with_paused_processing_synchronously (
          action, recalculate_graph);
      },
      nullptr);

    bool instantiation_finished = false;
    auto handler = [&instantiation_finished] (plugins::PluginUuidReference) {
      instantiation_finished = true;
    };

    auto * tracklist = project->tracklist ();
    auto   track_creator = std::make_unique<actions::TrackCreator> (
      *undo_stack, *project->track_factory_, *tracklist->collection (),
      *tracklist->trackRouting (), *tracklist->singletonTracks (),
      project.get ());

    auto importer = std::make_unique<actions::PluginImporter> (
      *undo_stack, *project->plugin_factory_, *track_creator,
      std::move (handler), project.get ());

    importer->importPluginToNewTrack (&(*descriptor));

    process_events_until_true ([&] () { return instantiation_finished; });
    EXPECT_TRUE (instantiation_finished);

    auto &plugin_registry = project->get_plugin_registry ();
    EXPECT_EQ (plugin_registry.get_hash_map ().size (), 1);

    auto * plugin = std::visit (
      [] (auto * pl) -> plugins::Plugin * { return pl; },
      plugin_registry.get_hash_map ().begin ()->second);
    EXPECT_NE (plugin, nullptr);
    if (plugin == nullptr)
      return nullptr;

    plugin->prepare_for_processing (
      nullptr, units::sample_rate (48000), units::samples (256));

    owned_project_ = std::move (project);
    owned_undo_stack_ = std::move (undo_stack);
    tracked_plugin_ = plugin;

    return plugin;
  }

  std::unique_ptr<structure::project::Project> owned_project_;
  utils::QObjectUniquePtr<undo::UndoStack>     owned_undo_stack_;
  plugins::Plugin *                            tracked_plugin_ = nullptr;
};

// ============================================================================
// Instrument tests (MIDI in -> audio out)
// ============================================================================

class PluginInstrumentTest : public PluginIntegrationTestBase
{
};

INSTANTIATE_TEST_SUITE_P (
  TripleSynth,
  PluginInstrumentTest,
  testing::Values (
    PluginTestParam{ "Triple Synth", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Triple Synth", PluginTestParam::Format::VST3 }));

TEST_P (PluginInstrumentTest, FreshInstanceProducesSound)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto * midi_in = find_midi_input_port (*plugin);
  ASSERT_NE (midi_in, nullptr);
  inject_midi_note_on (midi_in, 1, 60, 100);

  auto  time_nfo = make_time_info (256);
  auto &transport = *owned_project_->transport_;
  auto &tempo_map = owned_project_->tempo_map ();
  for (int i = 0; i < 5; ++i)
    plugin->process_block (time_nfo, transport, tempo_map);

  auto audio_outs = find_audio_output_ports (*plugin);
  ASSERT_FALSE (audio_outs.empty ());

  bool has_audio = false;
  for (auto * port : audio_outs)
    {
      if (utils::audio::buffer_has_audio (*port->buffers (), 0, 256))
        {
          has_audio = true;
          break;
        }
    }

  EXPECT_TRUE (has_audio)
    << "Plugin produced silent output -- parameters may be zero-initialized";
}

TEST_P (PluginInstrumentTest, SaveLoadRoundtripProducesSound)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto  time_nfo = make_time_info (256);
  auto &transport = *owned_project_->transport_;
  auto &tempo_map = owned_project_->tempo_map ();

  auto * target_param = find_first_plugin_param (*plugin);
  ASSERT_NE (target_param, nullptr);
  target_param->setBaseValue (0.8f);
  for (int i = 0; i < 3; ++i)
    plugin->process_block (time_nfo, transport, tempo_map);

  auto saved_state = plugin->save_state ();
  ASSERT_FALSE (saved_state.empty ());

  // --- Phase 2: fresh project, import same plugin, restore state ---

  auto project2 = create_minimal_project ();
  ASSERT_NE (project2, nullptr);
  project2->add_default_tracks ();

  auto undo_stack2 = utils::make_qobject_unique<undo::UndoStack> (
    [proj = project2.get ()] (
      const std::function<void ()> &action, bool recalculate_graph) {
      proj->engine ()->execute_function_with_paused_processing_synchronously (
        action, recalculate_graph);
    },
    nullptr);

  std::unique_ptr<juce::PluginDescription> juce_desc;
  if (GetParam ().format == PluginTestParam::Format::CLAP)
    {
      juce_desc = test_helpers::find_bundled_clap_plugin_by_name (
        juce::String::fromUTF8 (GetParam ().plugin_name.data ()));
    }
  else
    {
      juce_desc = test_helpers::find_bundled_vst3_plugin_by_name (
        juce::String::fromUTF8 (GetParam ().plugin_name.data ()));
    }
  ASSERT_NE (juce_desc, nullptr);

  auto descriptor =
    plugins::PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (descriptor, nullptr);

  bool instantiation_finished = false;
  auto handler = [&instantiation_finished] (plugins::PluginUuidReference) {
    instantiation_finished = true;
  };

  auto * tracklist = project2->tracklist ();
  auto   track_creator = std::make_unique<actions::TrackCreator> (
    *undo_stack2, *project2->track_factory_, *tracklist->collection (),
    *tracklist->trackRouting (), *tracklist->singletonTracks (),
    project2.get ());

  auto importer = std::make_unique<actions::PluginImporter> (
    *undo_stack2, *project2->plugin_factory_, *track_creator,
    std::move (handler), project2.get ());

  importer->importPluginToNewTrack (&(*descriptor));

  process_events_until_true ([&] () { return instantiation_finished; });
  ASSERT_TRUE (instantiation_finished);

  auto &plugin_registry2 = project2->get_plugin_registry ();
  ASSERT_EQ (plugin_registry2.get_hash_map ().size (), 1);

  auto * plugin2 = std::visit (
    [] (auto * pl) -> plugins::Plugin * { return pl; },
    plugin_registry2.get_hash_map ().begin ()->second);
  ASSERT_NE (plugin2, nullptr);

  plugin2->load_state (saved_state);

  plugin2->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));

  auto * midi_in2 = find_midi_input_port (*plugin2);
  ASSERT_NE (midi_in2, nullptr);
  inject_midi_note_on (midi_in2, 1, 60, 100);

  auto &transport2 = *project2->transport_;
  auto &tempo_map2 = project2->tempo_map ();

  for (int i = 0; i < 5; ++i)
    plugin2->process_block (time_nfo, transport2, tempo_map2);

  auto audio_outs2 = find_audio_output_ports (*plugin2);
  ASSERT_FALSE (audio_outs2.empty ());

  bool has_audio = false;
  for (auto * port : audio_outs2)
    {
      if (utils::audio::buffer_has_audio (*port->buffers (), 0, 256))
        {
          has_audio = true;
          break;
        }
    }

  EXPECT_TRUE (has_audio)
    << "Plugin produced silent output after save/load roundtrip -- "
       "state may not have been restored correctly";
}

// ============================================================================
// Parameter synchronization tests (all plugins)
// ============================================================================

class PluginParamSyncTest : public PluginIntegrationTestBase
{
};

INSTANTIATE_TEST_SUITE_P (
  TripleSynth,
  PluginParamSyncTest,
  testing::Values (
    PluginTestParam{ "Triple Synth", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Triple Synth", PluginTestParam::Format::VST3 }));

INSTANTIATE_TEST_SUITE_P (
  HighpassFilter,
  PluginParamSyncTest,
  testing::Values (
    PluginTestParam{ "Highpass Filter", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Highpass Filter", PluginTestParam::Format::VST3 }));

TEST_P (PluginParamSyncTest, ParameterChangesReachPlugin)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto  time_nfo = make_time_info (256);
  auto &transport = *owned_project_->transport_;
  auto &tempo_map = owned_project_->tempo_map ();

  auto * target_param = find_first_plugin_param (*plugin);
  ASSERT_NE (target_param, nullptr) << "No automatable parameter found";

  for (int i = 0; i < 3; ++i)
    plugin->process_block (time_nfo, transport, tempo_map);

  auto state_default = plugin->save_state ();
  ASSERT_FALSE (state_default.empty ());

  target_param->setBaseValue (0.9f);
  for (int i = 0; i < 3; ++i)
    plugin->process_block (time_nfo, transport, tempo_map);

  auto state_changed = plugin->save_state ();
  EXPECT_NE (state_default, state_changed)
    << "Plugin state unchanged after parameter edit";
}

TEST_P (PluginParamSyncTest, NoFeedbackLoopOnRepeatedCycles)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto  time_nfo = make_time_info (256);
  auto &transport = *owned_project_->transport_;
  auto &tempo_map = owned_project_->tempo_map ();

  auto * bypass = plugin->bypassParameter ();
  auto * gain = plugin->gainParameter ();
  for (const auto &param_ref : plugin->get_parameters ())
    {
      auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
      if (p != bypass && p != gain)
        p->setBaseValue (0.75f);
    }

  for (int i = 0; i < 5; ++i)
    plugin->process_block (time_nfo, transport, tempo_map);

  std::vector<float> values_before;
  for (const auto &param_ref : plugin->get_parameters ())
    {
      auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
      values_before.push_back (p->baseValue ());
    }

  for (int i = 0; i < 20; ++i)
    plugin->process_block (time_nfo, transport, tempo_map);

  std::vector<float> values_after;
  for (const auto &param_ref : plugin->get_parameters ())
    {
      auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
      values_after.push_back (p->baseValue ());
    }

  ASSERT_EQ (values_before.size (), values_after.size ());
  for (size_t i = 0; i < values_before.size (); ++i)
    {
      EXPECT_FLOAT_EQ (values_before[i], values_after[i])
        << "Parameter " << i << " drifted from " << values_before[i] << " to "
        << values_after[i];
    }
}

} // namespace zrythm::tests
