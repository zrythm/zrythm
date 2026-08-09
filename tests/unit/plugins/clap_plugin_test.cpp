// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <string_view>
#include <thread>
#include <vector>

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
    dispatcher_context_ = std::make_unique<QObject> ();
    main_dispatcher_ = std::make_unique<utils::MainThreadClosureDispatcher> (
      *dispatcher_context_, std::chrono::milliseconds{ 10 });
  }

  void TearDown () override
  {
    if (plugin_ != nullptr)
      {
        plugin_->release_resources ();
      }
    plugin_.reset ();
    main_dispatcher_.reset ();
    dispatcher_context_.reset ();
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
    plugin_->set_main_thread_services (*main_dispatcher_, {});
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

  /**
   * @brief Runs the actions the plugin posted for the main thread.
   *
   * Host callbacks the plugin invokes (gui.closed, request_resize, ...)
   * only take effect once the plugin's own call has returned, so tests
   * must let the dispatcher run before observing the result.
   */
  void pump_main_thread () { main_dispatcher_->process_pending (); }

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
  std::unique_ptr<QObject>                                 dispatcher_context_;
  std::unique_ptr<utils::MainThreadClosureDispatcher>      main_dispatcher_;
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
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (GetParam ()));
  expect_note_on_produces_audio ();
}

TEST_P (ClapPluginTest, HasNativeUiIsFalseForGuiLessPlugins)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (GetParam ()));
  EXPECT_FALSE (plugin_->hasNativeUi ());
}

TEST_F (ClapPluginTest, HasNativeUiTrueForPluginWithGui)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));
  EXPECT_TRUE (plugin_->hasNativeUi ());
}

TEST_F (ClapPluginTest, EditorShowHide)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));

  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
  // The fixture view has a fixed 320x240 size
  EXPECT_EQ (window_state_->width, 320);
  EXPECT_EQ (window_state_->height, 240);
  // The fixture GUI is not resizable
  EXPECT_FALSE (window_state_->resizable);
  // Showing runs the native-embedding handshake
  EXPECT_EQ (window_state_->complete_native_embedding_calls, 1);

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

TEST_F (ClapPluginTest, AbSwitchRestoresPluginState)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  auto * gain_param = find_param_by_label ("Level"sv);
  ASSERT_NE (gain_param, nullptr);

  constexpr float kValueA = 0.25f;
  constexpr float kValueB = 0.75f;

  EXPECT_FALSE (plugin_->abActive ());

  // Save state A with the initial value
  gain_param->setBaseValue (kValueA);
  process_blocks (5);
  plugin_->switchAbState (); // -> B active (initialized as copy of A)
  EXPECT_TRUE (plugin_->abActive ());

  // Change the value while in slot B, then switch back to A
  gain_param->setBaseValue (kValueB);
  process_blocks (5);
  plugin_->switchAbState (); // saves B, restores A
  EXPECT_FALSE (plugin_->abActive ());
  EXPECT_NEAR (gain_param->baseValue (), kValueA, 0.01f);

  // Switching to B restores the B value
  plugin_->switchAbState ();
  EXPECT_TRUE (plugin_->abActive ());
  EXPECT_NEAR (gain_param->baseValue (), kValueB, 0.01f);
}

TEST_F (ClapPluginTest, GuiRequestResizeResizesWindow)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);

  EXPECT_TRUE (plugin_->guiRequestResize (640, 480));
  pump_main_thread ();
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
}

TEST_F (ClapPluginTest, ScaleFactorFedBeforeInitialSizeQuery)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));
  // Set the scale before the factory creates the window (windows are
  // created inside setUiVisible)
  window_state_->content_scale_factor = 2.f;

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);
  // Fixture reports 640x480 physical at scale 2 -> 320x240 logical
  EXPECT_EQ (window_state_->width, 320);
  EXPECT_EQ (window_state_->height, 240);
}

TEST_F (ClapPluginTest, ScaleChangeIsReFedToPlugin)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));
  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);
  ASSERT_EQ (window_state_->width, 320);

  const auto calls_before = window_state_->set_size_calls;
  window_state_->window->set_content_scale_factor_for_test (2.f);
  // Host re-feeds the scale, re-queries (fixture now reports 640x480
  // physical) and re-applies the same logical size
  EXPECT_GT (window_state_->set_size_calls, calls_before);
  EXPECT_EQ (window_state_->width, 320);
  EXPECT_EQ (window_state_->height, 240);
}

TEST_F (ClapPluginTest, PluginInitiatedGuiCloseDestroysEditor)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);

  // The plugin reports its GUI was closed; per the spec the host must
  // acknowledge with guiDestroy() and drop its editor state
  plugin_->guiClosed (true);
  // The plugin's GUI may only be destroyed once its own gui.closed() call
  // has returned
  EXPECT_FALSE (window_state_->destroyed);

  pump_main_thread ();
  EXPECT_TRUE (window_state_->destroyed);
  EXPECT_FALSE (plugin_->uiVisible ());

  // The GUI can be recreated afterwards
  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
}

TEST_F (ClapPluginTest, CrossThreadGuiRequestsAreDeliveredOnMainThread)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));

  // The CLAP host gui callbacks are [thread-safe]: they may be called from
  // any thread and are marshaled to the main thread
  std::jthread requester ([this] {
    plugin_->guiRequestShow ();
    plugin_->guiRequestResize (640, 480);
  });

  process_events_until_true ([this] {
    return window_state_->visible && window_state_->width == 640
           && window_state_->height == 480;
  });
}

TEST_F (ClapPluginTest, RequestRestartKeepsPluginProcessing)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Synth"));

  // The plugin requests a restart: the host must deactivate and re-activate
  // it on the main thread (handled synchronously when requested from the
  // main thread)
  plugin_->requestRestart ();

  expect_note_on_produces_audio ();
}

TEST_F (ClapPluginTest, RequestCallbackIsDeliveredOnMainThread)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI CLAP"));

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);
  ASSERT_EQ (window_state_->width, 320);

  // requestCallback() is [thread-safe]; the host must schedule
  // plugin->on_main_thread() on the main thread. The fixture requests a
  // resize from its on_main_thread() as an observable side effect (and its
  // maximal clap-helpers checking aborts on wrong-thread delivery)
  std::jthread requester ([this] { plugin_->requestCallback (); });

  process_events_until_true ([this] { return window_state_->width == 640; });
}

// Note events emitted on the plugin's note output port must be forwarded to
// the host's MIDI output port with their velocity preserved (the fixture
// echoes incoming notes)
TEST_P (ClapPluginTest, NoteOutputIsForwardedToMidiOut)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (GetParam ()));

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  dsp::MidiPort * midi_out = nullptr;
  for (const auto &port_ref : plugin_->get_output_ports ())
    {
      midi_out = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_out != nullptr)
        break;
    }
  ASSERT_NE (midi_in, nullptr);
  ASSERT_NE (midi_out, nullptr);

  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  const auto note_off =
    dsp::midi_event::make_note_off (0, 60, 33, units::samples (0u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());
  midi_in->buffer_.push_back (note_off.time_, note_off.data ());

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  // Velocities round-trip verbatim in both dialects (raw bytes for MIDI,
  // nearest-int through the CLAP 0..1 double domain for CLAP notes)
  const std::array<int, 3> expected_note_on{ 0x90, 60, 100 };
  const std::array<int, 3> expected_note_off{ 0x80, 60, 33 };

  std::vector<std::array<int, 3>> received;
  for (const auto &ev : midi_out->buffer_)
    {
      const auto data = ev.data ();
      if (data.size () >= 3)
        received.push_back ({ data[0], data[1], data[2] });
    }
  EXPECT_THAT (
    received,
    ::testing::UnorderedElementsAre (expected_note_on, expected_note_off));
}

// Events the plugin emits with a timestamp outside the processed block
// must be dropped (the fixture echoes key 127 with an out-of-block time)
TEST_P (ClapPluginTest, OutputEventWithOutOfBlockTimeIsDropped)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (GetParam ()));

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  dsp::MidiPort * midi_out = nullptr;
  for (const auto &port_ref : plugin_->get_output_ports ())
    {
      midi_out = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_out != nullptr)
        break;
    }
  ASSERT_NE (midi_in, nullptr);
  ASSERT_NE (midi_out, nullptr);

  const auto note_on =
    dsp::midi_event::make_note_on (0, 127, 100, units::samples (0u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  for (const auto &ev : midi_out->buffer_)
    {
      const auto data = ev.data ();
      ASSERT_FALSE (data.size () >= 3 && data[1] == 127)
        << "Out-of-block event was forwarded (time "
        << ev.time ().in (units::samples) << ")";
    }
}

TEST_F (ClapPluginTest, LatencyReportedDuringActivateIsHandled)
{
  // The fixture reports its latency from within activate()
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Latency"));
  EXPECT_EQ (plugin_->get_single_playback_latency (), units::samples (256u));
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
