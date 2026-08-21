// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <string_view>
#include <thread>
#include <vector>

#include "dsp/fork_join_executor.h"
#include "dsp/midi_event.h"
#include "plugins/CLAPPluginFormat.h"
#include "plugins/clap_plugin.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "utils/audio.h"
#include "utils/object_registry.h"
#include "utils/views.h"

#include <QCoreApplication>

#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_juce_qapplication.h"
#include "helpers/test_plugin_finder.h"

#include "unit/dsp/graph_helpers.h"
#include <clap/events.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::plugins
{
using namespace std::literals;

namespace
{
/**
 * @brief Reads the plugin's saved state and parses it as JSON (empty object
 * if unavailable).
 */
nlohmann::json
read_plugin_state_json (Plugin &plugin)
{
  const auto state = plugin.save_state ();
  if (state.empty ())
    return {};
  const auto raw = QByteArray::fromBase64 (QByteArray::fromStdString (state));
  return nlohmann::json::parse (
    std::string_view (raw.constData (), static_cast<size_t> (raw.size ())),
    nullptr, false);
}
} // namespace

class ClapPluginTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication,
      public testing::WithParamInterface<std::string_view>
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    mock_transport_ =
      std::make_unique<::testing::NiceMock<dsp::graph_test::MockTransport>> ();
    // Plugins query the transport every process block: NiceMock keeps the
    // logs clean, and these give the queries paused defaults
    ON_CALL (*mock_transport_, get_play_state ())
      .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Paused));
    ON_CALL (*mock_transport_, recording_enabled ())
      .WillByDefault (::testing::Return (false));
    ON_CALL (*mock_transport_, recording_preroll_frames_remaining ())
      .WillByDefault (::testing::Return (units::samples (0)));
    ON_CALL (*mock_transport_, loop_enabled ())
      .WillByDefault (::testing::Return (false));
    ON_CALL (*mock_transport_, get_loop_range_positions ())
      .WillByDefault (
        ::testing::Return (
          std::make_pair (units::samples (0), units::samples (0))));
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (48000));
    dispatcher_context_ = std::make_unique<QObject> ();
    main_dispatcher_ = std::make_unique<utils::MainThreadClosureDispatcher> (
      *dispatcher_context_, std::chrono::milliseconds{ 10 });
    fork_join_executor_.start (2);
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
    ASSERT_FALSE (plugin_->get_all_output_ports ().empty ())
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
   * Runs paused-processing requests synchronously, counting them.
   *
   * The graph recalculation callback re-prepares the plugin, reallocating
   * port buffers, mirroring how a real recalculation prepares the nodes.
   *
   * @param pause_hook When given, invoked inside each pause with the pause
   * number (1-based) and the paused action, which the hook must invoke —
   * before, after, or sandwiched between its own assertions — to let the
   * action run.
   */
  void install_direct_paused_processing (
    std::function<void (int, const std::function<void ()> &)> pause_hook = {})
  {
    PluginHostMainThreadCallbacks callbacks{};
    callbacks.graph_recalc_ = [this] {
      ++graph_recalc_calls_;
      plugin_->prepare_for_processing (
        nullptr, units::sample_rate (48000), units::samples (256));
    };
    callbacks.with_paused_processing_ =
      [this, hook = std::move (pause_hook)] (const std::function<void ()> &fn) {
        ++paused_processing_calls_;
        if (hook)
          {
            hook (paused_processing_calls_, fn);
          }
        else
          {
            fn ();
          }
      };
    plugin_->set_main_thread_services (*main_dispatcher_, callbacks);
  }

  std::vector<dsp::AudioPort *> all_audio_ports (bool input)
  {
    return plugin_->get_all_audio_ports (
      input ? dsp::PortFlow::Input : dsp::PortFlow::Output);
  }

  std::vector<dsp::AudioPort *> attached_audio_ports (bool input)
  {
    return plugin_->get_attached_audio_ports (
      input ? dsp::PortFlow::Input : dsp::PortFlow::Output);
  }

  /**
   * @brief Runs the actions the plugin posted for the main thread.
   *
   * Host callbacks the plugin invokes (gui.closed, request_resize, ...)
   * only take effect once the plugin's own call has returned, so tests
   * must let the dispatcher run before observing the result.
   */
  void pump_main_thread () { main_dispatcher_->process_pending (); }

  void process_blocks (Plugin &plugin, int num_blocks)
  {
    const dsp::graph::ProcessBlockInfo time_nfo{
      .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = units::samples (256),
      .fork_join_executor_ = &fork_join_executor_,
    };
    for (int i = 0; i < num_blocks; ++i)
      plugin.process_block (time_nfo, *mock_transport_, *tempo_map_);
  }

  void process_blocks (int num_blocks)
  {
    process_blocks (*plugin_, num_blocks);
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<::testing::NiceMock<dsp::graph_test::MockTransport>>
                                                           mock_transport_;
  std::unique_ptr<dsp::TempoMap>                           tempo_map_;
  std::unique_ptr<QObject>                                 dispatcher_context_;
  std::unique_ptr<utils::MainThreadClosureDispatcher>      main_dispatcher_;
  dsp::graph::ForkJoinExecutor                             fork_join_executor_;
  std::unique_ptr<ClapPlugin>                              plugin_;
  std::shared_ptr<test_helpers::MockPluginHostWindowState> window_state_;
  int paused_processing_calls_ = 0;
  int graph_recalc_calls_ = 0;

  void expect_note_on_produces_audio ()
  {
    dsp::MidiPort * midi_in = nullptr;
    for (const auto &port_ref : plugin_->get_all_input_ports ())
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
    for (const auto &port_ref : plugin_->get_all_output_ports ())
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
  for (const auto &port_ref : plugin_->get_all_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  dsp::MidiPort * midi_out = nullptr;
  for (const auto &port_ref : plugin_->get_all_output_ports ())
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

// The process transport must carry the host transport (play state, tempo,
// musical position, time signature, loop range) so tempo-synced plugins can
// follow it. The fixture reports the last received transport in its state.
TEST_P (ClapPluginTest, TransportContextCarriesHostTransport)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (GetParam ()));

  ON_CALL (*mock_transport_, get_play_state ())
    .WillByDefault (::testing::Return (dsp::ITransport::PlayState::Rolling));
  ON_CALL (*mock_transport_, recording_enabled ())
    .WillByDefault (::testing::Return (false));
  ON_CALL (*mock_transport_, loop_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (*mock_transport_, get_loop_range_positions ())
    .WillByDefault (
      ::testing::Return (
        std::make_pair (units::samples (96000), units::samples (192000))));

  // 240000 samples at 48kHz = 5s = 10 beats (quarter notes) at the default
  // 120 BPM
  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (240000),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto state = read_plugin_state_json (*plugin_);
  ASSERT_TRUE (state.contains ("transport"));
  const auto &transport = state["transport"];
  ASSERT_TRUE (transport["present"].get<bool> ());

  const auto flags = transport["flags"].get<uint32_t> ();
  EXPECT_TRUE (flags & CLAP_TRANSPORT_HAS_TEMPO);
  EXPECT_TRUE (flags & CLAP_TRANSPORT_HAS_BEATS_TIMELINE);
  EXPECT_TRUE (flags & CLAP_TRANSPORT_HAS_SECONDS_TIMELINE);
  EXPECT_TRUE (flags & CLAP_TRANSPORT_HAS_TIME_SIGNATURE);
  EXPECT_TRUE (flags & CLAP_TRANSPORT_IS_PLAYING);
  EXPECT_TRUE (flags & CLAP_TRANSPORT_IS_LOOP_ACTIVE);

  // Default tempo map: 120 BPM, 4/4 - 5s is beat 10, inside bar 3 (0-based
  // bar 2) which starts at beat 8
  EXPECT_DOUBLE_EQ (transport["tempo"].get<double> (), 120.0);
  EXPECT_DOUBLE_EQ (transport["songPosBeats"].get<double> (), 10.0);
  EXPECT_DOUBLE_EQ (transport["songPosSeconds"].get<double> (), 5.0);
  EXPECT_DOUBLE_EQ (transport["barStartBeats"].get<double> (), 8.0);
  EXPECT_EQ (transport["barNumber"].get<int> (), 2);
  EXPECT_EQ (transport["timeSigNum"].get<int> (), 4);
  EXPECT_EQ (transport["timeSigDenom"].get<int> (), 4);

  // Loop at 96000..192000 samples = beats 4..8
  EXPECT_DOUBLE_EQ (transport["loopStartBeats"].get<double> (), 4.0);
  EXPECT_DOUBLE_EQ (transport["loopEndBeats"].get<double> (), 8.0);

  // The recording and preroll states must also reach the plugin
  ON_CALL (*mock_transport_, recording_enabled ())
    .WillByDefault (::testing::Return (true));
  ON_CALL (*mock_transport_, recording_preroll_frames_remaining ())
    .WillByDefault (::testing::Return (units::samples (256)));
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);
  const auto recording_state = read_plugin_state_json (*plugin_);
  const auto recording_flags =
    recording_state["transport"]["flags"].get<uint32_t> ();
  EXPECT_TRUE (recording_flags & CLAP_TRANSPORT_IS_RECORDING);
  EXPECT_TRUE (recording_flags & CLAP_TRANSPORT_IS_WITHIN_PRE_ROLL);
}

// Events the plugin emits with a timestamp outside the processed block
// must be dropped (the fixture echoes key 127 with an out-of-block time)
TEST_P (ClapPluginTest, OutputEventWithOutOfBlockTimeIsDropped)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (GetParam ()));

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_all_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  dsp::MidiPort * midi_out = nullptr;
  for (const auto &port_ref : plugin_->get_all_output_ports ())
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

// Ports carry the plugin's stable audio port ids, and the first enumerated
// port of each flow is the main one
TEST_F (ClapPluginTest, LoadAssignsStableIdsAndMainPurpose)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));

  const auto audio_ins = attached_audio_ports (true);
  ASSERT_EQ (audio_ins.size (), 1);
  EXPECT_EQ (audio_ins.at (0)->external_port_id (), 0);
  EXPECT_EQ (audio_ins.at (0)->purpose (), dsp::AudioPort::Purpose::Main);

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 1);
  EXPECT_EQ (audio_outs.at (0)->external_port_id (), 0);
  EXPECT_EQ (audio_outs.at (0)->purpose (), dsp::AudioPort::Purpose::Main);
}

// A names-only rescan is legal while the plugin is active: the labels are
// synced without a graph recalculation (which would restart the plugin)
TEST_F (ClapPluginTest, RescanNamesSyncsLabelsWithoutGraphRecalc)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  ASSERT_EQ (attached_audio_ports (false).at (0)->get_label (), u8"Output");

  auto * trigger = find_param_by_label ("Rename Output");
  ASSERT_NE (trigger, nullptr);
  trigger->setBaseValue (1.0f);
  process_blocks (1);

  // The rename arrives via the plugin's main-thread callback; the
  // names-only rescan's deferred reconciliation runs in one pause
  process_events_until_true ([this] { return paused_processing_calls_ >= 1; });

  EXPECT_EQ (attached_audio_ports (false).at (0)->get_label (), u8"Renamed Out");
  EXPECT_EQ (graph_recalc_calls_, 0);
}

// An untrustworthy port report refuses activation; a repeated prepare (as a
// routine graph recalculation would issue) keeps refusing it
TEST_F (ClapPluginTest, UntrustworthyPortReportRefusesActivationRepeatedly)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Bad Ports"));

  // The prepare inside load already failed; preparing again must not trip
  // the state machine
  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));

  // The plugin never activated: no audio ports were created from the
  // untrustworthy report and nothing processes
  EXPECT_TRUE (attached_audio_ports (false).empty ());
  EXPECT_TRUE (attached_audio_ports (true).empty ());
  process_blocks (1);
}

// When the plugin adds a bus and reports RESCAN_LIST from its deactivate()
// during the restart, a new port is created for it
TEST_F (ClapPluginTest, RescanListWithNewBusCreatesPort)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  ASSERT_EQ (attached_audio_ports (false).size (), 1);

  auto * trigger = find_param_by_label ("Grow Output");
  ASSERT_NE (trigger, nullptr);
  trigger->setBaseValue (1.0f);
  process_blocks (1);

  // The restart and the deferred rescan reconciliation each pause
  // processing once
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 2);
  EXPECT_EQ (audio_outs.at (1)->get_label (), u8"Out 2");
  EXPECT_EQ (
    audio_outs.at (1)->arrangement (), dsp::SpeakerArrangement::stereo ());
  EXPECT_EQ (audio_outs.at (1)->external_port_id (), 1);
  // No IS_MAIN flag: only the first output is the main bus
  EXPECT_EQ (audio_outs.at (1)->purpose (), dsp::AudioPort::Purpose::Sidechain);
}

// While a list rescan that adds an input bus awaits its deferred
// reconciliation, the bus has no port yet and the plugin receives silence
// on it
TEST_F (ClapPluginTest, RescanListWithNewInputBusFeedsSilenceUntilReconciled)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));

  // On the second pause (the deferred rescan reconciliation), process one
  // block before the reconciliation runs: the grown input bus has no port
  // yet, and the fixture overwrites its main output with a sentinel level
  // if it sees anything but silence on that bus
  std::optional<float> window_output;
  install_direct_paused_processing (
    [this, &window_output] (int pause, const std::function<void ()> &fn) {
      if (pause == 2 && !window_output.has_value ())
        {
          process_blocks (1);
          window_output =
            attached_audio_ports (false).at (0)->buffers ()->getReadPointer (
              0)[0];
        }
      fn ();
    });

  auto * grow = find_param_by_label ("Grow Input");
  ASSERT_NE (grow, nullptr);
  grow->setBaseValue (1.0f);
  process_blocks (1);

  // The restart and the deferred rescan reconciliation each pause once
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  // 0.5 is the fixture's identifying level for output bus 0; the sentinel
  // means it saw nonzero input on the port-less bus
  ASSERT_TRUE (window_output.has_value ());
  EXPECT_FLOAT_EQ (*window_output, 0.5f);

  // Reconciliation created a port for the new input bus
  const auto audio_ins = attached_audio_ports (true);
  ASSERT_EQ (audio_ins.size (), 2);
  EXPECT_EQ (audio_ins.at (1)->external_port_id (), 1);
  EXPECT_EQ (audio_ins.at (1)->purpose (), dsp::AudioPort::Purpose::Sidechain);
}

// When the plugin removes a bus and reports RESCAN_LIST, the corresponding
// port is detached, not destroyed: the object (and any connections to it)
// survives and revives if the bus returns
TEST_F (ClapPluginTest, RescanListWithRemovedBusDetachesPort)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  auto * grow = find_param_by_label ("Grow Output");
  ASSERT_NE (grow, nullptr);
  grow->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  const auto audio_outs_after_grow = attached_audio_ports (false);
  ASSERT_EQ (audio_outs_after_grow.size (), 2);
  auto * const added_port = audio_outs_after_grow.at (1);

  auto * shrink = find_param_by_label ("Shrink Output");
  ASSERT_NE (shrink, nullptr);
  shrink->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 4; });

  EXPECT_EQ (attached_audio_ports (false).size (), 1);
  const auto all_outs = all_audio_ports (false);
  ASSERT_EQ (all_outs.size (), 2);
  EXPECT_EQ (all_outs.at (1), added_port);
  EXPECT_TRUE (added_port->detached ());
  EXPECT_EQ (added_port->external_port_id (), 1);
}

// A bus that returns after being removed re-attaches the same port object
TEST_F (ClapPluginTest, RescanListWithReaddedBusReattachesPort)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  auto * grow = find_param_by_label ("Grow Output");
  ASSERT_NE (grow, nullptr);
  grow->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  auto * shrink = find_param_by_label ("Shrink Output");
  ASSERT_NE (shrink, nullptr);
  shrink->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 4; });

  // The toggles are one-shot: reset the host-side value so the second fire
  // registers as a change
  grow->setBaseValue (0.0f);
  process_blocks (1);
  grow->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 6; });

  const auto all_outs = all_audio_ports (false);
  ASSERT_EQ (all_outs.size (), 2);
  auto * const port = all_outs.at (1);
  EXPECT_FALSE (port->detached ());
  EXPECT_EQ (port->external_port_id (), 1);
  EXPECT_EQ (port->get_label (), u8"Out 2");
}

// When a bus grows its channel count, the restart resizes the plugin-facing
// scratch buffers before the engine ports are reconciled: audio cycles
// processed in that window run with mismatched scratch/port channel counts
TEST_F (ClapPluginTest, RescanChannelCountGrowsPortSafely)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));

  install_direct_paused_processing (
    [this] (int pause, const std::function<void ()> &fn) {
      fn ();
      if (pause == 1)
        {
          // The restart has resized the scratch buffers to the new channel
          // count while the engine port still has the old one, and the
          // deferred reconciliation has not run yet
          process_blocks (1);
        }
    });

  auto * widen = find_param_by_label ("Widen Output");
  ASSERT_NE (widen, nullptr);
  widen->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 1);
  EXPECT_EQ (
    audio_outs.at (0)->arrangement (),
    dsp::SpeakerArrangement::discrete_channels (6));
}

// Reconciling ports drops the buffers of ports whose arrangement changed;
// the graph recalculation that reallocates them must happen before
// processing resumes, not after
TEST_F (ClapPluginTest, RescanLeavesPortBuffersPreparedBeforeResume)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));

  install_direct_paused_processing (
    [this] (int, const std::function<void ()> &fn) {
      fn ();
      // Processing may resume as soon as the pause returns: every attached
      // port must already have a buffer matching its arrangement
      for (const auto * port : attached_audio_ports (false))
        {
          EXPECT_NE (port->buffers (), nullptr);
          if (port->buffers () != nullptr)
            {
              EXPECT_EQ (
                port->buffers ()->getNumChannels (),
                static_cast<int> (port->arrangement ().channel_count ()));
            }
        }
    });

  auto * widen = find_param_by_label ("Widen Output");
  ASSERT_NE (widen, nullptr);
  widen->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 1);
  EXPECT_EQ (
    audio_outs.at (0)->arrangement (),
    dsp::SpeakerArrangement::discrete_channels (6));
}

// On load, ports are synced to the live layout even when the bus topology
// already matches, reflecting buses renamed or main flags moved since the
// project was saved
TEST_F (ClapPluginTest, RestoreSyncsMetadataWhenTopologyMatches)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));

  auto * out = attached_audio_ports (false).at (0);
  out->set_purpose (dsp::AudioPort::Purpose::Sidechain);
  out->set_label (u8"Stale Name");

  // Restores negotiate with the plugin, which is only valid while deactivated
  plugin_->release_resources ();
  plugin_->restore_saved_bus_arrangements ();

  EXPECT_EQ (out->purpose (), dsp::AudioPort::Purpose::Main);
  EXPECT_EQ (out->get_label (), u8"Output");
}

// When the saved topology differs from the live one and the plugin
// implements configurable audio ports, the saved layout is pushed into the
// plugin and the ports reflect it
TEST_F (ClapPluginTest, RestorePushesSavedConfiguration)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Configurable"));

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 3);
  // The fixture's main output starts as 5.1
  ASSERT_EQ (audio_outs.at (0)->arrangement ().channel_count (), 6);

  // Configuration pushes are only valid while the plugin is deactivated
  plugin_->release_resources ();

  // Simulate a project saved while the plugin's main output was stereo
  audio_outs.at (0)->set_arrangement (dsp::SpeakerArrangement::stereo ());
  plugin_->restore_saved_bus_arrangements ();

  // The push succeeded and the ports were synced to the accepted layout
  EXPECT_EQ (
    audio_outs.at (0)->arrangement (), dsp::SpeakerArrangement::stereo ());
}

// Pushing a configuration with several ambisonic buses hands one request
// per bus to the plugin; every request's port details must stay valid until
// the plugin has consumed them
TEST_F (ClapPluginTest, RestoreWithMultipleAmbisonicBusesKeepsRequestDetailsValid)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Configurable"));

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 3);

  plugin_->release_resources ();

  // Simulate a project saved with third-order ambisonic side buses
  const auto fuma_third_order = dsp::SpeakerArrangement::ambisonics (
    3, dsp::SpeakerArrangement::AmbisonicOrdering::FuMa,
    dsp::SpeakerArrangement::AmbisonicNormalization::MaxN);
  audio_outs.at (1)->set_arrangement (fuma_third_order);
  audio_outs.at (2)->set_arrangement (fuma_third_order);
  plugin_->restore_saved_bus_arrangements ();

  // The fixture's apply only honors requests for its main output bus and
  // leaves the side buses untouched, so the live layout wins there and the
  // ports are synced back
  EXPECT_EQ (
    audio_outs.at (1)->arrangement (), dsp::SpeakerArrangement::stereo ());
  EXPECT_EQ (
    audio_outs.at (2)->arrangement (), dsp::SpeakerArrangement::stereo ());
}

// A plugin that asks for an audio ports rescan without implementing the
// audio ports extension is rejected with a warning, not a crash
TEST_F (ClapPluginTest, RescanWithoutAudioPortsExtensionIsRejected)
{
  // The fixture requests the rescan from its activate()
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Rescan Misuse"));

  // The rescan is refused: no audio ports appear for a plugin without the
  // extension
  EXPECT_TRUE (all_audio_ports (true).empty ());
  EXPECT_TRUE (all_audio_ports (false).empty ());
}

// A list rescan may reorder buses; ports must be paired with buses by the
// plugin's stable port ids, not by port list position
TEST_F (ClapPluginTest, RescanListWithReorderedBusesKeepsRouting)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));

  install_direct_paused_processing ();

  auto * grow = find_param_by_label ("Grow Output");
  ASSERT_NE (grow, nullptr);
  grow->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 2);
  ASSERT_EQ (audio_outs.at (0)->external_port_id (), 0);
  ASSERT_EQ (audio_outs.at (1)->external_port_id (), 1);

  // The fixture fills each bus with a constant identifying its enumeration
  // index: 0.5 for bus 0, 1.0 for bus 1
  process_blocks (1);
  EXPECT_FLOAT_EQ (audio_outs.at (0)->buffers ()->getReadPointer (0)[0], 0.5f);
  EXPECT_FLOAT_EQ (audio_outs.at (1)->buffers ()->getReadPointer (0)[0], 1.0f);

  // Swap the enumeration order of the two buses
  auto * swap = find_param_by_label ("Swap Outputs");
  ASSERT_NE (swap, nullptr);
  swap->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 4; });

  // Bus 0 (value 0.5) is now the bus with stable id 1 and vice versa
  process_blocks (1);
  EXPECT_FLOAT_EQ (audio_outs.at (0)->buffers ()->getReadPointer (0)[0], 1.0f);
  EXPECT_FLOAT_EQ (audio_outs.at (1)->buffers ()->getReadPointer (0)[0], 0.5f);
}

// A saved discrete bus topology (no port type, any channel count) is
// pushed into the plugin like any other
TEST_F (ClapPluginTest, RestorePushesDiscreteChannelCount)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Configurable"));

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 3);
  // The fixture's main output starts as 5.1
  ASSERT_EQ (audio_outs.at (0)->arrangement ().channel_count (), 6);

  // Configuration pushes are only valid while the plugin is deactivated
  plugin_->release_resources ();

  // Simulate a project saved while the plugin's main output was 6 discrete
  // channels
  audio_outs.at (0)->set_arrangement (
    dsp::SpeakerArrangement::discrete_channels (6));
  plugin_->restore_saved_bus_arrangements ();

  EXPECT_EQ (
    audio_outs.at (0)->arrangement (),
    dsp::SpeakerArrangement::discrete_channels (6));
}

// The fixture reports its 5.1 main output in a non-canonical wire order
// (FC first) and fills each channel with its wire index; the host hands the
// plugin permuted channel pointers so the port buffers stay canonical
TEST_F (
  ClapPluginTest,
  SurroundNonCanonicalWireOrderIsPermutedIntoCanonicalBuffers)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Configurable"));

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 3);
  const auto * main_out = audio_outs.at (0);
  ASSERT_EQ (main_out->arrangement ().channel_count (), 6);

  process_blocks (1);

  // Wire channel w carries the value w + 1. The wire order is FC, FL, FR,
  // LFE, SL, SR while the canonical order is FL, FR, FC, LFE, SL, SR
  constexpr std::array<float, 6> expected{ 2.f, 3.f, 1.f, 4.f, 5.f, 6.f };
  for (const auto ch : std::views::iota (0, 6))
    {
      EXPECT_FLOAT_EQ (
        main_out->buffers ()->getReadPointer (ch)[0], expected.at (ch));
    }
}

// The fixture requests 8 parallel tasks from the host pool per process
// block; the host must execute every task index exactly once per block
TEST_F (ClapPluginTest, ThreadPoolExecutesAllTasksAcrossWorkers)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Thread Pool"));

  process_blocks (4);

  const auto state = read_plugin_state_json (*plugin_);
  ASSERT_FALSE (state.is_discarded ());
  EXPECT_EQ (state["pool_requests_succeeded"].get<int> (), 4);
  EXPECT_EQ (state["pool_requests_rejected"].get<int> (), 0);
  EXPECT_EQ (state["fallback_runs"].get<int> (), 0);
  const auto &counts = state["exec_counts"];
  ASSERT_EQ (counts.size (), 64);
  for (const auto i : std::views::iota (0, 8))
    {
      EXPECT_EQ (counts[i].get<int> (), 4) << "task " << i;
    }
}

// A single-task request must be executed inline on the audio thread
TEST_F (ClapPluginTest, ThreadPoolSingleTaskRunsInlineOnAudioThread)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Thread Pool"));

  auto * num_tasks = find_param_by_label ("Num Tasks");
  ASSERT_NE (num_tasks, nullptr);
  // 1 of 0..64 (exact binary fraction)
  num_tasks->setBaseValue (1.0f / 64.0f);

  process_blocks (3);

  const auto state = read_plugin_state_json (*plugin_);
  ASSERT_FALSE (state.is_discarded ());
  EXPECT_EQ (state["pool_requests_succeeded"].get<int> (), 3);
  EXPECT_EQ (state["fallback_runs"].get<int> (), 0);
  EXPECT_EQ (state["exec_counts"][0].get<int> (), 3);
  EXPECT_EQ (state["execs_on_process_thread"].get<int> (), 3);
}

// Without an executor in the processing context the host must reject the
// request; the plugin must then complete its tasks by itself (CLAP spec
// fallback)
TEST_F (ClapPluginTest, ThreadPoolRejectedWithoutExecutorFallsBackToSerial)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Thread Pool"));

  static constexpr int               kNumBlocks = 3;
  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  for (const auto _ : std::views::iota (0, kNumBlocks))
    plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto state = read_plugin_state_json (*plugin_);
  ASSERT_FALSE (state.is_discarded ());
  EXPECT_EQ (state["pool_requests_succeeded"].get<int> (), 0);
  EXPECT_EQ (state["pool_requests_rejected"].get<int> (), kNumBlocks);
  EXPECT_EQ (state["fallback_runs"].get<int> (), kNumBlocks);
  const auto &counts = state["exec_counts"];
  ASSERT_EQ (counts.size (), 64);
  for (const auto i : std::views::iota (0, 8))
    {
      EXPECT_EQ (counts[i].get<int> (), kNumBlocks) << "task " << i;
    }
}

// Tasks beyond the worker count must still all execute (in waves)
TEST_F (ClapPluginTest, ThreadPoolMoreTasksThanWorkersStillCompletes)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Thread Pool"));

  auto * num_tasks = find_param_by_label ("Num Tasks");
  ASSERT_NE (num_tasks, nullptr);
  num_tasks->setBaseValue (1.0f); // 64 tasks, more than the 2 test workers

  process_blocks (2);

  const auto state = read_plugin_state_json (*plugin_);
  ASSERT_FALSE (state.is_discarded ());
  EXPECT_EQ (state["pool_requests_succeeded"].get<int> (), 2);
  EXPECT_EQ (state["fallback_runs"].get<int> (), 0);
  const auto &counts = state["exec_counts"];
  ASSERT_EQ (counts.size (), 64);
  for (const auto i : std::views::iota (0, 64))
    {
      EXPECT_EQ (counts[i].get<int> (), 2) << "task " << i;
    }
}

// Two instances processing concurrently on different threads (as the
// parallel graph runs them) submit to the shared executor; both must
// complete all their tasks without interference
TEST_F (ClapPluginTest, ThreadPoolConcurrentInstancesDoNotInterfere)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Thread Pool"));

  // Second instance: sequential construction on this thread, its own
  // registry/window state, shared main-thread dispatcher
  const auto juce_desc = test_helpers::find_test_clap_plugin_by_name (
    juce::String ("Test Thread Pool"));
  ASSERT_NE (juce_desc, nullptr);
  auto config = std::make_unique<PluginConfiguration> ();
  config->descr_ = PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (config->descr_, nullptr);
  auto second_registry = std::make_unique<utils::ObjectRegistry> ();
  auto second_window_state =
    std::make_shared<test_helpers::MockPluginHostWindowState> ();
  auto second_plugin = std::make_unique<ClapPlugin> (
    *second_registry,
    test_helpers::make_mock_plugin_host_window_factory (second_window_state));
  second_plugin->set_main_thread_services (*main_dispatcher_, {});
  second_plugin->set_configuration (*config);
  ASSERT_FALSE (second_plugin->get_all_output_ports ().empty ())
    << "Second plugin failed to load";
  second_plugin->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));

  static constexpr int kBlocksPerThread = 50;
  {
    std::jthread first ([this] {
      process_blocks (*plugin_, kBlocksPerThread);
    });
    std::jthread second ([this, &second_plugin] {
      process_blocks (*second_plugin, kBlocksPerThread);
    });
  }

  second_plugin->release_resources ();

  for (auto * plugin : { plugin_.get (), second_plugin.get () })
    {
      const auto state = read_plugin_state_json (*plugin);
      ASSERT_FALSE (state.is_discarded ());
      EXPECT_EQ (state["pool_requests_succeeded"].get<int> (), kBlocksPerThread);
      EXPECT_EQ (state["pool_requests_rejected"].get<int> (), 0);
      EXPECT_EQ (state["fallback_runs"].get<int> (), 0);
      const auto &counts = state["exec_counts"];
      ASSERT_EQ (counts.size (), 64);
      for (const auto i : std::views::iota (0, 8))
        {
          EXPECT_EQ (counts[i].get<int> (), kBlocksPerThread) << "task " << i;
        }
    }
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
