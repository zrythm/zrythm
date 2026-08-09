// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 * Integration tests for plugin hosting: parameter synchronization, audio
 * output, and save/load roundtrip.
 *
 * Tests both CLAP and VST3 (JUCE) variants of each test plugin.
 */

#include <ostream>
#include <ranges>

#include "utils/format_qt.h"

#include "actions/plugin_importer.h"
#include "actions/track_creator.h"
#include "dsp/midi_event.h"
#include "plugins/plugin.h"
#include "plugins/plugin_descriptor.h"
#include "undo/undo_stack.h"
#include "utils/audio.h"
#include "utils/midi.h"

#include "helpers/project_fixture.h"
#include "helpers/test_plugin_finder.h"

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
    {
      const auto _ev = dsp::midi_event::make_note_on (
        channel, note, velocity, units::samples (0u));
      port->buffer_.push_back (_ev.time_, _ev.data ());
    }
  }

  template <typename PortT>
  static PortT * find_first_port_of_type (const auto &port_refs)
  {
    auto typed_ports =
      port_refs | std::views::transform ([] (const auto &port_ref) {
        return port_ref.template get_object_as<PortT> ();
      })
      | std::views::filter ([] (const PortT * port) { return port != nullptr; });
    const auto it = typed_ports.begin ();
    return it == typed_ports.end () ? nullptr : *it;
  }

  template <typename PortT>
  static std::vector<PortT *> find_all_ports_of_type (const auto &port_refs)
  {
    return port_refs | std::views::transform ([] (const auto &port_ref) {
             return port_ref.template get_object_as<PortT> ();
           })
           | std::views::filter ([] (const PortT * port) {
               return port != nullptr;
             })
           | std::ranges::to<std::vector> ();
  }

  static dsp::MidiPort * find_midi_input_port (plugins::Plugin &plugin)
  {
    return find_first_port_of_type<dsp::MidiPort> (plugin.get_input_ports ());
  }

  static std::vector<dsp::AudioPort *>
  find_audio_output_ports (plugins::Plugin &plugin)
  {
    return find_all_ports_of_type<dsp::AudioPort> (plugin.get_output_ports ());
  }

  static dsp::graph::ProcessBlockInfo make_time_info (uint32_t num_frames = 256)
  {
    return dsp::graph::ProcessBlockInfo{
      .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
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
          return test_helpers::find_test_clap_plugin_by_name (plugin_name);
        }

      return test_helpers::find_test_vst3_plugin_by_name (plugin_name);
    }();
    EXPECT_NE (juce_desc, nullptr)
      << "Test " << format_str << " plugin '" << param.plugin_name
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

    auto &plugin_registry = project->get_registry ();
    EXPECT_EQ (plugin_registry.count_matching<plugins::Plugin> (), 1);

    plugins::Plugin * plugin = nullptr;
    plugin_registry.for_each_matching<plugins::Plugin> (
      [&] (plugins::Plugin &pl) { plugin = &pl; });
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
  TestSynth,
  PluginInstrumentTest,
  testing::Values (
    PluginTestParam{ "Test Synth", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Test Synth", PluginTestParam::Format::VST3 }));

TEST_P (PluginInstrumentTest, FreshInstanceProducesSound)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto * midi_in = find_midi_input_port (*plugin);
  ASSERT_NE (midi_in, nullptr);
  inject_midi_note_on (midi_in, 0, 60, 100);

  auto  time_nfo = make_time_info (256);
  auto &transport = *owned_project_->transport_;
  auto &tempo_map = owned_project_->tempo_map ();
  auto  transport_snapshot = transport.get_snapshot ();
  for (int i = 0; i < 5; ++i)
    plugin->process_block (time_nfo, transport_snapshot, tempo_map);

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
  auto transport_snapshot = transport.get_snapshot ();
  for (int i = 0; i < 3; ++i)
    plugin->process_block (time_nfo, transport_snapshot, tempo_map);

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
      juce_desc = test_helpers::find_test_clap_plugin_by_name (
        juce::String::fromUTF8 (GetParam ().plugin_name.data ()));
    }
  else
    {
      juce_desc = test_helpers::find_test_vst3_plugin_by_name (
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

  auto &plugin_registry2 = project2->get_registry ();
  ASSERT_EQ (plugin_registry2.count_matching<plugins::Plugin> (), 1);

  plugins::Plugin * plugin2 = nullptr;
  plugin_registry2.for_each_matching<plugins::Plugin> (
    [&] (plugins::Plugin &pl) { plugin2 = &pl; });
  ASSERT_NE (plugin2, nullptr);

  plugin2->load_state (saved_state);

  plugin2->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));

  auto * midi_in2 = find_midi_input_port (*plugin2);
  ASSERT_NE (midi_in2, nullptr);
  inject_midi_note_on (midi_in2, 0, 60, 100);

  auto &transport2 = *project2->transport_;
  auto &tempo_map2 = project2->tempo_map ();
  auto  transport2_snapshot = transport2.get_snapshot ();

  for (int i = 0; i < 5; ++i)
    plugin2->process_block (time_nfo, transport2_snapshot, tempo_map2);

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
  TestSynth,
  PluginParamSyncTest,
  testing::Values (
    PluginTestParam{ "Test Synth", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Test Synth", PluginTestParam::Format::VST3 }));

INSTANTIATE_TEST_SUITE_P (
  TestGain,
  PluginParamSyncTest,
  testing::Values (
    PluginTestParam{ "Test Gain", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Test Gain", PluginTestParam::Format::VST3 }));

TEST_P (PluginParamSyncTest, ParameterChangesReachPlugin)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto  time_nfo = make_time_info (256);
  auto &transport = *owned_project_->transport_;
  auto &tempo_map = owned_project_->tempo_map ();

  auto * target_param = find_first_plugin_param (*plugin);
  ASSERT_NE (target_param, nullptr) << "No automatable parameter found";

  auto transport_snapshot = transport.get_snapshot ();
  for (int i = 0; i < 3; ++i)
    plugin->process_block (time_nfo, transport_snapshot, tempo_map);

  auto state_default = plugin->save_state ();
  ASSERT_FALSE (state_default.empty ());

  target_param->setBaseValue (0.9f);
  auto state_changed = state_default;
  ScopedQCoreApplication::process_events_until_true ([&] () {
    auto transport_snap = transport.get_snapshot ();
    plugin->process_block (time_nfo, transport_snap, tempo_map);
    state_changed = plugin->save_state ();
    return state_changed != state_default;
  });

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

  auto transport_snapshot = transport.get_snapshot ();
  for (int i = 0; i < 5; ++i)
    plugin->process_block (time_nfo, transport_snapshot, tempo_map);

  std::vector<float> values_before;
  for (const auto &param_ref : plugin->get_parameters ())
    {
      auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
      values_before.push_back (p->baseValue ());
    }

  transport_snapshot = transport.get_snapshot ();
  for (int i = 0; i < 20; ++i)
    plugin->process_block (time_nfo, transport_snapshot, tempo_map);

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

// ============================================================================
// Chunk processing tests (loop-point splits pass a nonzero buffer offset)
// ============================================================================

class PluginChunkGainTest : public PluginIntegrationTestBase
{
};

INSTANTIATE_TEST_SUITE_P (
  TestGain,
  PluginChunkGainTest,
  testing::Values (
    PluginTestParam{ "Test Gain", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Test Gain", PluginTestParam::Format::VST3 }));

// When the graph splits a cycle at a loop point, the node is called with a
// nonzero buffer offset: the plugin must receive exactly the chunk's input
// frames, and its output must land at the chunk's position in the port
// buffers
TEST_P (PluginChunkGainTest, ProcessesCorrectChunkWhenBufferOffsetIsNonZero)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto * audio_in =
    find_first_port_of_type<dsp::AudioPort> (plugin->get_input_ports ());
  const auto audio_outs = find_audio_output_ports (*plugin);
  ASSERT_NE (audio_in, nullptr);
  ASSERT_FALSE (audio_outs.empty ());
  auto * audio_out = audio_outs.front ();

  constexpr auto kOffset = 64u;
  constexpr auto kNframes = 128u;
  constexpr auto kBlockSize = 256u;
  constexpr auto kSentinel = -1.f;

  // Ramp on the input; sentinel on the output so writes outside the chunk
  // are detected (process_block only clears the chunk region)
  for (const auto ch : { 0, 1 })
    {
      for (const auto i : std::views::iota (0u, kBlockSize))
        {
          audio_in->buffers ()->setSample (
            ch, static_cast<int> (i), static_cast<float> (i) / 256.f);
          audio_out->buffers ()->setSample (ch, static_cast<int> (i), kSentinel);
        }
    }

  auto      &transport = *owned_project_->transport_;
  auto      &tempo_map = owned_project_->tempo_map ();
  const auto transport_snapshot = transport.get_snapshot ();
  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0u),
    .buffer_offset_ = units::samples (kOffset),
    .nframes_ = units::samples (kNframes),
  };
  plugin->process_block (time_nfo, transport_snapshot, tempo_map);

  for (const auto ch : { 0, 1 })
    {
      for (const auto i : std::views::iota (0u, kBlockSize))
        {
          const bool in_chunk = i >= kOffset && i < kOffset + kNframes;
          // The fixtures default to unity gain: the chunk must be an exact
          // copy
          const float expected =
            in_chunk ? static_cast<float> (i) / 256.f : kSentinel;
          EXPECT_FLOAT_EQ (
            audio_out->buffers ()->getSample (ch, static_cast<int> (i)),
            expected)
            << "ch " << ch << " sample " << i;
        }
    }
}

class PluginChunkSynthTest : public PluginIntegrationTestBase
{
};

INSTANTIATE_TEST_SUITE_P (
  TestSynth,
  PluginChunkSynthTest,
  testing::Values (
    PluginTestParam{ "Test Synth", PluginTestParam::Format::CLAP },
    PluginTestParam{ "Test Synth", PluginTestParam::Format::VST3 }));

// The MIDI port buffer holds events for the whole cycle, so on loop-point
// splits each chunk must only receive the events inside it, re-based to the
// chunk start; plugin output events must be re-based back to cycle-relative
// times (the fixtures echo note events to their note output)
TEST_P (PluginChunkSynthTest, NoteEventsAreFilteredAndRebasedAroundChunkSplits)
{
  auto * plugin = import_and_prepare_plugin (GetParam ());
  ASSERT_NE (plugin, nullptr);

  auto * midi_in = find_midi_input_port (*plugin);
  auto * midi_out =
    find_first_port_of_type<dsp::MidiPort> (plugin->get_output_ports ());
  const auto audio_outs = find_audio_output_ports (*plugin);
  ASSERT_NE (midi_in, nullptr);
  ASSERT_NE (midi_out, nullptr);
  ASSERT_FALSE (audio_outs.empty ());

  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (100u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  auto      &transport = *owned_project_->transport_;
  auto      &tempo_map = owned_project_->tempo_map ();
  const auto transport_snapshot = transport.get_snapshot ();

  // Chunk [0, 64) does not contain the note
  const dsp::graph::ProcessBlockInfo first_chunk{
    .transport_position_ = units::samples (0u),
    .buffer_offset_ = units::samples (0u),
    .nframes_ = units::samples (64u),
  };
  plugin->process_block (first_chunk, transport_snapshot, tempo_map);
  EXPECT_TRUE (midi_out->buffer_.empty ())
    << "Note outside the chunk was delivered to the plugin";
  EXPECT_FALSE (
    utils::audio::buffer_has_audio (*audio_outs.front ()->buffers (), 0, 64));

  // Chunk [64, 192) contains the note at chunk-relative sample 36
  const dsp::graph::ProcessBlockInfo second_chunk{
    .transport_position_ = units::samples (0u),
    .buffer_offset_ = units::samples (64u),
    .nframes_ = units::samples (128u),
  };
  plugin->process_block (second_chunk, transport_snapshot, tempo_map);

  ASSERT_EQ (midi_out->buffer_.size (), 1u);
  const auto echoed = midi_out->buffer_.front ();
  EXPECT_EQ (echoed.time (), units::samples (100u))
    << "Echoed note was not re-based to the cycle timeline";
  const auto echoed_data = echoed.data ();
  ASSERT_GE (echoed_data.size (), 3u);
  EXPECT_EQ (echoed_data[0], 0x90);
  EXPECT_EQ (echoed_data[1], 60);
  EXPECT_TRUE (
    utils::audio::buffer_has_audio (*audio_outs.front ()->buffers (), 64, 128));
}

} // namespace zrythm::tests
