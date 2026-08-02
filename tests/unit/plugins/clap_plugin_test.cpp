// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <string_view>
#include <thread>

#include "dsp/midi_event.h"
#include "plugins/CLAPPluginFormat.h"
#include "plugins/clap_plugin.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "utils/audio.h"
#include "utils/object_registry.h"

#include <QCoreApplication>

#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_juce_qapplication.h"
#include "helpers/test_plugin_finder.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::plugins
{
using namespace std::literals;

class ClapPluginTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication,
      public testing::WithParamInterface<std::string_view>
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

  void load_test_plugin (std::string_view name)
  {
    const auto juce_desc = test_helpers::find_test_clap_plugin_by_name (
      juce::String::fromUTF8 (name.data (), static_cast<int> (name.size ())));
    ASSERT_NE (juce_desc, nullptr)
      << "Test CLAP plugin '" << name << "' not found";

    auto config = std::make_unique<PluginConfiguration> ();
    config->descr_ = PluginDescriptor::from_juce_description (*juce_desc);
    ASSERT_NE (config->descr_, nullptr);

    window_state_ = std::make_shared<test_helpers::MockPluginHostWindowState> ();
    plugin_ = std::make_unique<ClapPlugin> (
      *registry_,
      test_helpers::make_mock_plugin_host_window_factory (window_state_));
    // CLAP instantiation is synchronous
    plugin_->set_configuration (*config);
    ASSERT_FALSE (plugin_->get_output_ports ().empty ())
      << "Plugin failed to load";

    plugin_->prepare_for_processing (
      nullptr, units::sample_rate (48000), units::samples (256));
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
  std::unique_ptr<ClapPlugin>                              plugin_;
  std::shared_ptr<test_helpers::MockPluginHostWindowState> window_state_;

  void expect_note_on_produces_audio ()
  {
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

    bool has_audio = false;
    for (const auto &port_ref : plugin_->get_output_ports ())
      {
        if (auto * port = port_ref.get_object_as<dsp::AudioPort> ())
          {
            if (utils::audio::buffer_has_audio (*port->buffers (), 0, 256))
              {
                has_audio = true;
                break;
              }
          }
      }
    EXPECT_TRUE (has_audio)
      << "Plugin produced silent output for note-on (dialect mismatch?)";
  }
};

// A note-on sent to the plugin's MIDI input must produce audio on the
// plugin's audio outputs, regardless of the note dialect the plugin
// declares (CLAP notes or raw MIDI)
TEST_P (ClapPluginTest, NoteOnProducesAudio)
{
  load_test_plugin (GetParam ());
  expect_note_on_produces_audio ();
}

TEST_P (ClapPluginTest, HasNativeUiIsFalseForGuiLessPlugins)
{
  load_test_plugin (GetParam ());
  EXPECT_FALSE (plugin_->hasNativeUi ());
}

TEST_F (ClapPluginTest, HasNativeUiTrueForPluginWithGui)
{
  load_test_plugin ("Test GUI CLAP");
  EXPECT_TRUE (plugin_->hasNativeUi ());
}

TEST_F (ClapPluginTest, EditorShowHide)
{
  load_test_plugin ("Test GUI CLAP");

  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
  // The fixture view has a fixed 320x240 size
  EXPECT_EQ (window_state_->width, 320);
  EXPECT_EQ (window_state_->height, 240);
  // The fixture GUI is not resizable
  EXPECT_FALSE (window_state_->resizable);

  // Hiding keeps the GUI and its window alive (guiHide does not free
  // resources)
  plugin_->setUiVisible (false);
  EXPECT_FALSE (window_state_->visible);
  EXPECT_FALSE (window_state_->destroyed);

  // Re-showing reuses the same window
  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
  EXPECT_FALSE (window_state_->destroyed);
}

TEST_F (ClapPluginTest, BypassToggleViaHostWindow)
{
  load_test_plugin ("Test GUI CLAP");
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

TEST_F (ClapPluginTest, AbSwitchRestoresPluginState)
{
  load_test_plugin ("Test Gain");

  auto * gain_param = find_param_by_label ("Level"sv);
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

TEST_F (ClapPluginTest, GuiRequestResizeResizesWindow)
{
  load_test_plugin ("Test GUI CLAP");

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);

  EXPECT_TRUE (plugin_->guiRequestResize (640, 480));
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
}

TEST_F (ClapPluginTest, PluginInitiatedGuiCloseDestroysEditor)
{
  load_test_plugin ("Test GUI CLAP");

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);

  // The plugin reports its GUI was closed; per the spec the host must
  // acknowledge with guiDestroy() and drop its editor state
  plugin_->guiClosed (true);
  EXPECT_TRUE (window_state_->destroyed);
  EXPECT_FALSE (plugin_->uiVisible ());

  // The GUI can be recreated afterwards
  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
}

TEST_F (ClapPluginTest, CrossThreadGuiRequestsAreDeliveredOnMainThread)
{
  load_test_plugin ("Test GUI CLAP");

  // The CLAP host gui callbacks are [thread-safe]: they may be called from
  // any thread and are marshaled to the main thread
  std::jthread requester ([this] {
    plugin_->guiRequestShow ();
    plugin_->guiRequestResize (640, 480);
  });

  process_events_until_true ([this] {
    return window_state_->visible && window_state_->width == 640;
  });

  EXPECT_TRUE (window_state_->visible);
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
}

TEST_F (ClapPluginTest, RequestRestartKeepsPluginProcessing)
{
  load_test_plugin ("Test Synth");

  // The plugin requests a restart: the host must deactivate and re-activate
  // it on the main thread (handled synchronously when requested from the
  // main thread)
  plugin_->requestRestart ();

  expect_note_on_produces_audio ();
}

TEST_F (ClapPluginTest, RequestCallbackIsDeliveredOnMainThread)
{
  load_test_plugin ("Test GUI CLAP");

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);
  ASSERT_EQ (window_state_->width, 320);

  // requestCallback() is [thread-safe]; the host must schedule
  // plugin->on_main_thread() on the main thread. The fixture requests a
  // resize from its on_main_thread() as an observable side effect (and its
  // maximal clap-helpers checking aborts on wrong-thread delivery)
  std::jthread requester ([this] { plugin_->requestCallback (); });

  process_events_until_true ([this] { return window_state_->width == 640; });

  EXPECT_EQ (window_state_->width, 640);
}

INSTANTIATE_TEST_SUITE_P (
  NoteDialects,
  ClapPluginTest,
  testing::Values (
    // CLAP note dialect only
    "Test Synth"sv,
    // MIDI dialect only
    "Test Synth MIDI"sv));

} // namespace zrythm::plugins
