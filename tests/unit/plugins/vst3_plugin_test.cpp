// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/vst3_plugin.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/audio.h"
#include "utils/object_registry.h"

#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_juce_qapplication.h"
#include "helpers/test_plugin_finder.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::plugins
{

class Vst3PluginTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    mock_transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (48000));
  }

  void TearDown () override
  {
    if (plugin_ != nullptr)
      {
        plugin_->release_resources ();
      }
    plugin_.reset ();
    registry_.reset ();
  }

  void load_test_plugin (const juce::String &name)
  {
    const auto juce_desc = test_helpers::find_test_vst3_plugin_by_name (name);
    ASSERT_NE (juce_desc, nullptr)
      << "Test VST3 plugin '" << name << "' not found";

    auto config = std::make_unique<PluginConfiguration> ();
    config->descr_ = PluginDescriptor::from_juce_description (*juce_desc);
    ASSERT_NE (config->descr_, nullptr);

    window_state_ = std::make_shared<test_helpers::MockPluginHostWindowState> ();
    plugin_ = std::make_unique<Vst3Plugin> (
      *registry_,
      test_helpers::make_mock_plugin_host_window_factory (window_state_));
    plugin_->set_configuration (*config);
    ASSERT_FALSE (plugin_->get_output_ports ().empty ())
      << "Plugin failed to load";

    plugin_->prepare_for_processing (
      nullptr, units::sample_rate (48000), units::samples (256));
  }

  dsp::AudioPort * find_audio_port (bool input)
  {
    const auto &port_refs =
      input ? plugin_->get_input_ports () : plugin_->get_output_ports ();
    for (const auto &port_ref : port_refs)
      {
        if (auto * port = port_ref.get_object_as<dsp::AudioPort> ())
          return port;
      }
    return nullptr;
  }

  dsp::ProcessorParameter * find_param_by_label (std::string_view label)
  {
    for (const auto &param_ref : plugin_->get_parameters ())
      {
        auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
        if (p->label () == label)
          return p;
      }
    return nullptr;
  }

  void process_blocks (int num_blocks)
  {
    const dsp::graph::ProcessBlockInfo time_nfo{
      .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = units::samples (256),
    };
    for (int i = 0; i < num_blocks; ++i)
      plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);
  }

  std::unique_ptr<utils::ObjectRegistry>                   registry_;
  std::unique_ptr<dsp::graph_test::MockTransport>          mock_transport_;
  std::unique_ptr<dsp::TempoMap>                           tempo_map_;
  std::unique_ptr<Vst3Plugin>                              plugin_;
  std::shared_ptr<test_helpers::MockPluginHostWindowState> window_state_;
};

TEST_F (Vst3PluginTest, HasNativeUiFalseForViewlessPlugin)
{
  load_test_plugin ("Test Gain");
  EXPECT_FALSE (plugin_->hasNativeUi ());
}

TEST_F (Vst3PluginTest, HasNativeUiTrueForPluginWithView)
{
  load_test_plugin ("Test GUI");
  EXPECT_TRUE (plugin_->hasNativeUi ());
}

TEST_F (Vst3PluginTest, EditorShowHide)
{
  load_test_plugin ("Test GUI");

  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
  // The fixture view starts at 320x240 and requests 640x480 via
  // IPlugFrame::resizeView when attached
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
  // The fixture view reports it cannot be resized
  EXPECT_FALSE (window_state_->resizable);

  // Hiding keeps the native GUI alive: the host window is only unmapped
  plugin_->setUiVisible (false);
  EXPECT_FALSE (window_state_->visible);
  EXPECT_FALSE (window_state_->destroyed);

  // Re-showing reuses the same view and host window
  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
  EXPECT_FALSE (window_state_->destroyed);
}

TEST_F (Vst3PluginTest, BypassToggleViaHostWindow)
{
  load_test_plugin ("Test GUI");
  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);
  ASSERT_NE (window_state_->window, nullptr);

  const auto * bypass = plugin_->bypassParameter ();
  const auto   is_bypassed = [&] {
    return bypass->range ().isToggled (bypass->baseValue ());
  };
  ASSERT_FALSE (is_bypassed ());

  bool last_bypassed = false;
  int  bypassed_change_count = 0;
  QObject::connect (
    window_state_->window, &PluginHostWindow::bypassedChanged,
    window_state_->window, [&] (bool bypassed) {
      last_bypassed = bypassed;
      ++bypassed_change_count;
    });

  // Clicking the host window's bypass button toggles the plugin's bypass
  // parameter, and the change flows back as bypassedChanged
  Q_EMIT window_state_->window->bypassToggleRequested ();
  EXPECT_TRUE (is_bypassed ());
  EXPECT_EQ (bypassed_change_count, 1);
  EXPECT_TRUE (last_bypassed);

  Q_EMIT window_state_->window->bypassToggleRequested ();
  EXPECT_FALSE (is_bypassed ());
  EXPECT_EQ (bypassed_change_count, 2);
  EXPECT_FALSE (last_bypassed);
}

TEST_F (Vst3PluginTest, AbSwitchRestoresPluginState)
{
  load_test_plugin ("Test Gain");

  auto * gain_param = find_param_by_label ("Level");
  ASSERT_NE (gain_param, nullptr);

  // The gain plugin has no native UI - create a host window directly
  test_helpers::MockPluginHostWindow window (*plugin_, window_state_);

  constexpr float kValueA = 0.25f;
  constexpr float kValueB = 0.75f;

  // Save state A with the initial value
  gain_param->setBaseValue (kValueA);
  process_blocks (5);
  Q_EMIT window.abSwitchRequested (); // -> B active (initialized as copy of A)

  // Change the value while in slot B, then switch back to A
  gain_param->setBaseValue (kValueB);
  process_blocks (5);
  Q_EMIT window.abSwitchRequested (); // saves B, restores A
  EXPECT_NEAR (gain_param->baseValue (), kValueA, 0.01f);

  // Switching to B restores the B value
  Q_EMIT window.abSwitchRequested ();
  EXPECT_NEAR (gain_param->baseValue (), kValueB, 0.01f);
}

TEST_F (Vst3PluginTest, UiVisibleRejectedForViewlessPlugin)
{
  load_test_plugin ("Test Gain");

  plugin_->setUiVisible (true);
  EXPECT_FALSE (plugin_->uiVisible ());
  EXPECT_FALSE (window_state_->visible);
}

// MIDI-CC-mapped params (IMidiMapping) must not be exposed as Zrythm
// parameters; other params must still appear
TEST_F (Vst3PluginTest, MidiCcMappedParamsAreNotExposed)
{
  load_test_plugin ("Test MIDI CC");

  EXPECT_NE (find_param_by_label ("Level"), nullptr);
  EXPECT_EQ (find_param_by_label ("CC Level"), nullptr)
    << "CC-mapped param should be filtered out";
}

// A MIDI CC message must reach the plugin as a parameter change (via the
// IMidiMapping table) and affect processing
TEST_F (Vst3PluginTest, MidiCcReachesPluginAsParamChange)
{
  load_test_plugin ("Test MIDI CC");

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  ASSERT_NE (midi_in, nullptr);

  auto * audio_in = find_audio_port (true);
  ASSERT_NE (audio_in, nullptr);
  ASSERT_GE (audio_in->buffers ()->getNumChannels (), 2);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, 256))
        {
          audio_in->buffers ()->setSample (ch, i, 1.f);
        }
    }

  // CC 20 (the fixture's mapped controller) at half value; with Level at its
  // default of 1.0, the output should be scaled by 64/127
  const auto cc =
    dsp::midi_event::make_control_change (0, 20, 64, units::samples (0u));
  midi_in->buffer_.push_back (cc.time_, cc.data ());

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
  EXPECT_NEAR (audio_out->buffers ()->getSample (0, 255), 64.f / 127.f, 0.01f);
}

// A note-on sent to the plugin's MIDI input must produce audio on the
// plugin's audio outputs
TEST_F (Vst3PluginTest, NoteOnProducesAudio)
{
  load_test_plugin ("Test Synth");

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  ASSERT_NE (midi_in, nullptr);

  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  for (int i = 0; i < 5; ++i)
    plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
  EXPECT_TRUE (utils::audio::buffer_has_audio (*audio_out->buffers (), 0, 256))
    << "Plugin produced silent output for note-on";
}

// With the default gain of 1.0, input audio must pass through unchanged
TEST_F (Vst3PluginTest, GainAppliedToAudio)
{
  load_test_plugin ("Test Gain");

  auto * audio_in = find_audio_port (true);
  ASSERT_NE (audio_in, nullptr);
  ASSERT_GE (audio_in->buffers ()->getNumChannels (), 2);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, 256))
        {
          audio_in->buffers ()->setSample (ch, i, 0.5f);
        }
    }

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, 256))
        {
          EXPECT_FLOAT_EQ (audio_out->buffers ()->getSample (ch, i), 0.5f)
            << "ch " << ch << " sample " << i;
        }
    }
}

// A second prepare cycle must not crash or leak (release + re-prepare)
TEST_F (Vst3PluginTest, ReprepareForProcessing)
{
  load_test_plugin ("Test Gain");
  plugin_->release_resources ();
  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (44100), units::samples (512));

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (512),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
}

TEST_F (Vst3PluginTest, LatencyIsReported)
{
  load_test_plugin ("Test Gain");
  // Test Gain has no latency, but the getter must be wired up
  EXPECT_EQ (plugin_->get_single_playback_latency ().in (units::samples), 0u);
}

TEST_F (Vst3PluginTest, ParametersAreExposed)
{
  load_test_plugin ("Test Gain");

  auto * level_param = find_param_by_label ("Level");
  ASSERT_NE (level_param, nullptr);
  EXPECT_TRUE (level_param->automatable ());
  // Test Gain defaults to unity gain
  EXPECT_FLOAT_EQ (level_param->baseValue (), 1.f);
}

TEST_F (Vst3PluginTest, ParameterChangeAffectsAudio)
{
  load_test_plugin ("Test Gain");

  auto * level_param = find_param_by_label ("Level");
  ASSERT_NE (level_param, nullptr);
  level_param->setBaseValue (0.5f);

  auto * audio_in = find_audio_port (true);
  ASSERT_NE (audio_in, nullptr);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, 256))
        {
          audio_in->buffers ()->setSample (ch, i, 1.f);
        }
    }

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, 256))
        {
          EXPECT_NEAR (audio_out->buffers ()->getSample (ch, i), 0.5f, 0.001f)
            << "ch " << ch << " sample " << i;
        }
    }
}

TEST_F (Vst3PluginTest, StateSaveLoadRoundtrip)
{
  load_test_plugin ("Test Gain");

  auto * level_param = find_param_by_label ("Level");
  ASSERT_NE (level_param, nullptr);
  level_param->setBaseValue (0.25f);

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto saved_state = plugin_->save_state ();
  ASSERT_FALSE (saved_state.empty ());

  // Load the state into a fresh instance
  auto       registry2 = std::make_unique<utils::ObjectRegistry> ();
  const auto juce_desc =
    test_helpers::find_test_vst3_plugin_by_name ("Test Gain");
  ASSERT_NE (juce_desc, nullptr);
  auto config2 = std::make_unique<PluginConfiguration> ();
  config2->descr_ = PluginDescriptor::from_juce_description (*juce_desc);
  auto plugin2 = std::make_unique<Vst3Plugin> (
    *registry2,
    test_helpers::make_mock_plugin_host_window_factory (window_state_));
  plugin2->load_state (saved_state);
  plugin2->set_configuration (*config2);
  ASSERT_FALSE (plugin2->get_output_ports ().empty ());

  // The loaded state must be reflected in the parameter
  dsp::ProcessorParameter * level_param2 = nullptr;
  for (const auto &param_ref : plugin2->get_parameters ())
    {
      auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
      if (p->label () == "Level")
        {
          level_param2 = p;
          break;
        }
    }
  ASSERT_NE (level_param2, nullptr);
  EXPECT_NEAR (level_param2->baseValue (), 0.25f, 0.001f);

  plugin2->release_resources ();
}

} // namespace zrythm::plugins
