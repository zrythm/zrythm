// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <array>
#include <ranges>
#include <utility>
#include <vector>

#include "dsp/midi_event.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "plugins/vst3_plugin.h"
#include "plugins/vst3_plugin_format.h"
#include "utils/audio.h"
#include "utils/object_registry.h"
#include "utils/views.h"

#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_juce_qapplication.h"
#include "helpers/test_plugin_finder.h"

#include "unit/dsp/graph_helpers.h"
#include <base/source/fstreamer.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <pluginterfaces/vst/ivstprocesscontext.h>
#include <public.sdk/source/vst/hosting/module.h>
#include <public.sdk/source/vst/hosting/plugprovider.h>
#include <public.sdk/source/vst/utility/memoryibstream.h>
#include <public.sdk/source/vst/vstpresetfile.h>

namespace zrythm::plugins
{

namespace
{
/**
 * @brief Reads the plugin's controller state chunk and parses it as JSON
 * (empty object if unavailable).
 */
nlohmann::json
read_controller_state_json (Vst3Plugin &plugin)
{
  const auto state = plugin.save_state ();
  if (state.empty ())
    return {};
  auto raw = QByteArray::fromBase64 (QByteArray::fromStdString (state));
  Steinberg::ResizableMemoryIBStream stream;
  stream.write (
    raw.data (), static_cast<Steinberg::int32> (raw.size ()), nullptr);
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
  return nlohmann::json::parse (json_text, nullptr, false);
}
} // namespace

class Vst3PluginTest
    : public ::testing::Test,
      public test_helpers::ScopedJuceQApplication
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
    plugin_->set_main_thread_services (*main_dispatcher_, {});
    plugin_->set_configuration (*config);
    ASSERT_FALSE (plugin_->get_all_output_ports ().empty ())
      << "Plugin failed to load";

    plugin_->prepare_for_processing (
      nullptr, units::sample_rate (48000), units::samples (256));
  }

  dsp::AudioPort * find_audio_port (bool input)
  {
    const auto &port_refs =
      input ? plugin_->get_all_input_ports () : plugin_->get_all_output_ports ();
    for (const auto &port_ref : port_refs)
      {
        if (auto * port = port_ref.get_object_as<dsp::AudioPort> ())
          return port;
      }
    return nullptr;
  }

  /** Runs paused-processing requests synchronously, counting them. */
  void install_direct_paused_processing ()
  {
    PluginHostMainThreadCallbacks callbacks{};
    callbacks
      .with_paused_processing_ = [this] (const std::function<void ()> &fn) {
      ++paused_processing_calls_;
      fn ();
    };
    plugin_->set_main_thread_services (*main_dispatcher_, callbacks);
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
  std::vector<dsp::AudioPort *> all_audio_ports (bool input)
  {
    auto port_refs =
      input ? plugin_->get_all_input_ports () : plugin_->get_all_output_ports ();
    return port_refs | std::views::transform (&dsp::PortUuidReference::get)
           | utils::views::qobject_cast_and_filter<dsp::AudioPort>
           | std::ranges::to<std::vector> ();
  }

  std::vector<dsp::AudioPort *> attached_audio_ports (bool input)
  {
    auto port_refs =
      input ? plugin_->get_attached_input_ports ()
            : plugin_->get_attached_output_ports ();
    return port_refs | std::views::transform (&dsp::PortUuidReference::get)
           | utils::views::qobject_cast_and_filter<dsp::AudioPort>
           | std::ranges::to<std::vector> ();
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

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<::testing::NiceMock<dsp::graph_test::MockTransport>>
                                                           mock_transport_;
  std::unique_ptr<dsp::TempoMap>                           tempo_map_;
  std::unique_ptr<QObject>                                 dispatcher_context_;
  std::unique_ptr<utils::MainThreadClosureDispatcher>      main_dispatcher_;
  std::unique_ptr<Vst3Plugin>                              plugin_;
  std::shared_ptr<test_helpers::MockPluginHostWindowState> window_state_;
  int paused_processing_calls_ = 0;
};

TEST_F (Vst3PluginTest, HasNativeUiFalseForViewlessPlugin)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));
  EXPECT_FALSE (plugin_->hasNativeUi ());
}

TEST_F (Vst3PluginTest, ViewlessPluginStaysUiVisibleForGenericUi)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));
  ASSERT_FALSE (plugin_->hasNativeUi ());

  plugin_->setUiVisible (true);

  // The generic UI keys on uiVisible && !hasPresentableNativeUi, so a
  // plugin without a native editor stays UI-visible when shown, and no
  // native window is created
  EXPECT_TRUE (plugin_->uiVisible ());
  EXPECT_FALSE (window_state_->visible);
}

TEST_F (Vst3PluginTest, HasNativeUiTrueForPluginWithView)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI"));
  EXPECT_TRUE (plugin_->hasNativeUi ());
}

TEST_F (Vst3PluginTest, EditorShowHide)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI"));

  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
  // The fixture view starts at 320x240 and requests 640x480 via
  // IPlugFrame::resizeView when attached
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
  // The fixture view reports it cannot be resized
  EXPECT_FALSE (window_state_->resizable);
  // Showing runs the native-embedding handshake
  EXPECT_EQ (window_state_->complete_native_embedding_calls, 1);

  // Hiding keeps the native GUI alive: the host window is only unmapped
  plugin_->setUiVisible (false);
  EXPECT_FALSE (window_state_->visible);
  EXPECT_FALSE (window_state_->destroyed);

  // Re-showing reuses the same view and host window
  plugin_->setUiVisible (true);
  EXPECT_TRUE (window_state_->visible);
  EXPECT_FALSE (window_state_->destroyed);
}

TEST_F (Vst3PluginTest, ScaleFactorFedAndSizesConverted)
{
#ifdef Q_OS_MACOS
  // Cocoa has no host-side logical/physical size conversion (sizes are
  // passed through), so the 640x480 expectations below do not apply
  GTEST_SKIP ();
#endif
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI"));
  // Set the scale before the factory creates the window (windows are
  // created inside setUiVisible)
  window_state_->content_scale_factor = 2.f;

  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);
  // attached() resize: 1280x960 physical at scale 2 -> 640x480 logical
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
}

TEST_F (Vst3PluginTest, ScaleChangeIsReFedToPlugin)
{
#ifdef Q_OS_MACOS
  // Cocoa has no host-side logical/physical size conversion (sizes are
  // passed through), so the 640x480 expectations below do not apply
  GTEST_SKIP ();
#endif
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test GUI"));
  plugin_->setUiVisible (true);
  ASSERT_TRUE (window_state_->visible);
  ASSERT_EQ (window_state_->width, 640);

  const auto calls_before = window_state_->set_size_calls;
  window_state_->window->set_content_scale_factor_for_test (2.f);
  // setContentScaleFactor -> fixture resizeView(1280x960) -> same
  // logical 640x480 re-applied
  EXPECT_GT (window_state_->set_size_calls, calls_before);
  EXPECT_EQ (window_state_->width, 640);
  EXPECT_EQ (window_state_->height, 480);
}

TEST_F (Vst3PluginTest, AbSwitchRestoresPluginState)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  auto * gain_param = find_param_by_label ("Level");
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

// MIDI-CC-mapped params (IMidiMapping) must not be exposed as Zrythm
// parameters; other params must still appear
TEST_F (Vst3PluginTest, MidiCcMappedParamsAreNotExposed)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test MIDI CC"));

  EXPECT_NE (find_param_by_label ("Level"), nullptr);
  EXPECT_EQ (find_param_by_label ("CC Level"), nullptr)
    << "CC-mapped param should be filtered out";
}

// A MIDI CC message must reach the plugin as a parameter change (via the
// IMidiMapping table) and affect processing
TEST_F (Vst3PluginTest, MidiCcReachesPluginAsParamChange)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test MIDI CC"));

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_all_input_ports ())
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
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Synth"));

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

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
  EXPECT_TRUE (utils::audio::buffer_has_audio (*audio_out->buffers (), 0, 256))
    << "Plugin produced silent output for note-on";
}

// A note-off's velocity must reach the plugin verbatim (not replaced with a
// fixed default): the fixture outputs the received velocity as a DC offset
TEST_F (Vst3PluginTest, NoteOffVelocityReachesPlugin)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Synth"));

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_all_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  ASSERT_NE (midi_in, nullptr);

  const auto note_off =
    dsp::midi_event::make_note_off (0, 60, 100, units::samples (0u));
  midi_in->buffer_.push_back (note_off.time_, note_off.data ());

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
  EXPECT_NEAR (audio_out->buffers ()->getSample (0, 255), 100.f / 127.f, 0.01f);
}

// The process context must carry the host transport (play state, tempo,
// musical position, time signature, loop range) so tempo-synced plugins can
// follow it. The fixture reports the last received ProcessContext in its
// state chunk.
TEST_F (Vst3PluginTest, TransportContextCarriesHostTransport)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Synth"));

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

  // 240000 samples at 48kHz = 5s = 10 quarter notes at the default 120 BPM
  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (240000),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto state = read_controller_state_json (*plugin_);
  ASSERT_TRUE (state.contains ("transport"));
  const auto &transport = state["transport"];
  ASSERT_TRUE (transport["present"].get<bool> ());

  using VstContext = Steinberg::Vst::ProcessContext;
  const auto flags = transport["state"].get<uint32_t> ();
  EXPECT_TRUE (flags & VstContext::kPlaying);
  EXPECT_TRUE (flags & VstContext::kCycleActive);
  EXPECT_TRUE (flags & VstContext::kProjectTimeMusicValid);
  EXPECT_TRUE (flags & VstContext::kTempoValid);
  EXPECT_TRUE (flags & VstContext::kBarPositionValid);
  EXPECT_TRUE (flags & VstContext::kCycleValid);
  EXPECT_TRUE (flags & VstContext::kTimeSigValid);

  // Default tempo map: 120 BPM, 4/4 - 5s is quarter 10, inside bar 3
  // (0-based bar 2) which starts at quarter 8
  EXPECT_DOUBLE_EQ (transport["tempo"].get<double> (), 120.0);
  EXPECT_DOUBLE_EQ (transport["projectTimeMusic"].get<double> (), 10.0);
  EXPECT_DOUBLE_EQ (transport["barPositionMusic"].get<double> (), 8.0);
  EXPECT_EQ (transport["timeSigNumerator"].get<int> (), 4);
  EXPECT_EQ (transport["timeSigDenominator"].get<int> (), 4);

  // Loop at 96000..192000 samples = quarters 4..8
  EXPECT_DOUBLE_EQ (transport["cycleStartMusic"].get<double> (), 4.0);
  EXPECT_DOUBLE_EQ (transport["cycleEndMusic"].get<double> (), 8.0);

  // The recording state must also reach the plugin (VST3 has no preroll
  // concept; that flag is covered in the CLAP transport test)
  ON_CALL (*mock_transport_, recording_enabled ())
    .WillByDefault (::testing::Return (true));
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);
  const auto recording_state = read_controller_state_json (*plugin_);
  const auto recording_flags =
    recording_state["transport"]["state"].get<uint32_t> ();
  EXPECT_TRUE (recording_flags & VstContext::kRecording);
}

// Note events emitted on the plugin's event output bus must be forwarded
// to the host's MIDI output port with their velocity preserved (the
// fixture echoes incoming notes)
TEST_F (Vst3PluginTest, NoteOutputIsForwardedToMidiOut)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Synth"));

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

  // Velocities round-trip verbatim (nearest-int through the VST3 0..1
  // float domain)
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

// With the default gain of 1.0, input audio must pass through unchanged
TEST_F (Vst3PluginTest, GainAppliedToAudio)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

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

// A second prepare cycle must not crash or leak, and processing must still
// produce output (release + re-prepare)
TEST_F (Vst3PluginTest, ReprepareForProcessing)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));
  plugin_->release_resources ();
  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (44100), units::samples (512));

  auto * audio_in = find_audio_port (true);
  ASSERT_NE (audio_in, nullptr);
  ASSERT_GE (audio_in->buffers ()->getNumChannels (), 2);
  for (const auto ch : std::views::iota (0, 2))
    {
      for (const auto i : std::views::iota (0, 512))
        {
          audio_in->buffers ()->setSample (ch, i, 0.5f);
        }
    }

  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (512),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto * audio_out = find_audio_port (false);
  ASSERT_NE (audio_out, nullptr);
  EXPECT_FLOAT_EQ (audio_out->buffers ()->getSample (0, 511), 0.5f);
  EXPECT_FLOAT_EQ (audio_out->buffers ()->getSample (1, 511), 0.5f);
}

TEST_F (Vst3PluginTest, LatencyIsReported)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));
  // Test Gain has no latency, but the getter must be wired up
  EXPECT_EQ (plugin_->get_single_playback_latency ().in (units::samples), 0u);
}

TEST_F (Vst3PluginTest, ParametersAreExposed)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  auto * level_param = find_param_by_label ("Level");
  ASSERT_NE (level_param, nullptr);
  EXPECT_TRUE (level_param->automatable ());
  // Test Gain defaults to unity gain
  EXPECT_FLOAT_EQ (level_param->baseValue (), 1.f);
}

TEST_F (Vst3PluginTest, ParameterChangeAffectsAudio)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

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
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

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
  ASSERT_FALSE (plugin2->get_all_output_ports ().empty ());

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

TEST_F (Vst3PluginTest, LatencyIsQueriedOnPrepare)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Latency"));
  EXPECT_EQ (plugin_->get_single_playback_latency (), units::samples (256u));
}

// The plugin context factory is a process-wide singleton (SDK design), so
// each instance must claim it before view-lifecycle calls and clear it at
// teardown — but only when it is still the current claimant
TEST_F (Vst3PluginTest, PluginContextTracksInstanceLifetime)
{
  auto &context_factory = Steinberg::Vst::PluginContextFactory::instance ();

  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));
  auto * const first_context = context_factory.getPluginContext ();
  ASSERT_NE (first_context, nullptr);

  // A second instance claims the context for itself on load
  auto       registry2 = std::make_unique<utils::ObjectRegistry> ();
  const auto juce_desc =
    test_helpers::find_test_vst3_plugin_by_name ("Test Gain");
  ASSERT_NE (juce_desc, nullptr);
  auto config2 = std::make_unique<PluginConfiguration> ();
  config2->descr_ = PluginDescriptor::from_juce_description (*juce_desc);
  ASSERT_NE (config2->descr_, nullptr);
  auto plugin2 = std::make_unique<Vst3Plugin> (
    *registry2,
    test_helpers::make_mock_plugin_host_window_factory (window_state_));
  plugin2->set_configuration (*config2);
  auto * const second_context = context_factory.getPluginContext ();
  ASSERT_NE (second_context, nullptr);
  EXPECT_NE (first_context, second_context);

  // View-lifecycle calls re-claim the context for the calling instance
  // (hasNativeUi() lazily creates the view, which is when plugins query
  // the context)
  plugin_->hasNativeUi ();
  EXPECT_EQ (context_factory.getPluginContext (), first_context);

  // Destroying a non-current instance must not disturb the current claim
  plugin2.reset ();
  EXPECT_EQ (context_factory.getPluginContext (), first_context);

  // Destroying the current claimant clears the context, so later
  // getPluginContext() calls never observe freed memory
  plugin_.reset ();
  EXPECT_EQ (context_factory.getPluginContext (), nullptr);
}

// Host-initiated parameter changes must reach the edit controller (via
// beginEdit/setParamNormalized/endEdit on the main thread), not just the
// processor-side queues, or the plugin's own UI never learns about them.
// The fixture counts controller-side setParamNormalized calls and exposes
// the count in its structured (JSON) state
TEST_F (Vst3PluginTest, HostParamChangesReachEditController)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  auto * level_param = find_param_by_label ("Level");
  ASSERT_NE (level_param, nullptr);
  level_param->setBaseValue (0.5f);
  process_blocks (1);

  const auto read_edit_count = [this] () -> int {
    const auto state = read_controller_state_json (*plugin_);
    return state.value ("controllerEditCount", 0);
  };
  process_events_until_true ([&] { return read_edit_count () >= 1; });
}

// A plugin that changes its MIDI-CC mapping at runtime (MIDI learn) reports
// it via restartComponent(kMidiCCAssignmentChanged); the host must rebuild
// its CC -> ParamID translation table or later CCs keep using the stale
// mapping. The fixture arms a CC 7 -> Level mapping when its "CC Assign"
// toggle is set
TEST_F (Vst3PluginTest, MidiCcAssignmentChangeRebuildsMapping)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  dsp::MidiPort * midi_in = nullptr;
  for (const auto &port_ref : plugin_->get_all_input_ports ())
    {
      midi_in = port_ref.get_object_as<dsp::MidiPort> ();
      if (midi_in != nullptr)
        break;
    }
  ASSERT_NE (midi_in, nullptr);
  auto * cc_assign = find_param_by_label ("CC Assign");
  ASSERT_NE (cc_assign, nullptr);

  // The fixture exposes its current gain in its structured (JSON)
  // controller state
  const auto read_gain = [this] () -> double {
    const auto state = read_controller_state_json (*plugin_);
    return state.contains ("gain") ? state["gain"].get<double> () : -1.0;
  };

  // No mapping yet: CC 7 changes nothing
  const auto cc_before =
    dsp::midi_event::make_control_change (0, 7, 32, units::samples (0u));
  midi_in->buffer_.push_back (cc_before.time_, cc_before.data ());
  process_blocks (1);
  EXPECT_DOUBLE_EQ (read_gain (), 1.0);

  // Arm the mapping. The toggle reaches the fixture via the posted
  // controller sync, and the resulting kMidiCCAssignmentChanged restart is
  // itself posted from within that handler, so the rebuild completes on a
  // later main-thread pass
  cc_assign->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([&] {
    const auto cc =
      dsp::midi_event::make_control_change (0, 7, 64, units::samples (0u));
    midi_in->buffer_.push_back (cc.time_, cc.data ());
    process_blocks (1);
    const auto gain = read_gain ();
    return gain >= 0.0 && gain < 0.6;
  });
  EXPECT_NEAR (read_gain (), 64.0 / 127.0, 1e-9);
}

// kIoChanged requires the host to deactivate the component before
// re-activating its buses and re-preparing processing. The fixture records
// any bus activated while it was still active and reports it in its state
// chunk
TEST_F (Vst3PluginTest, IoChangeDeactivatesBeforeReactivatingBuses)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  auto * trigger = find_param_by_label ("Trigger IO Change");
  ASSERT_NE (trigger, nullptr);
  trigger->setBaseValue (1.0f);
  process_blocks (1);

  // The restart request is handled asynchronously on the main thread
  process_events_until_true ([this] { return paused_processing_calls_ >= 1; });
  process_blocks (1);

  const auto state = read_controller_state_json (*plugin_);
  ASSERT_TRUE (state.contains ("busesActivatedWhileActive"));
  EXPECT_FALSE (state["busesActivatedWhileActive"].get<bool> ())
    << "Buses were re-activated while the component was still active";
}

// kReloadComponent requires the host to tear down and re-create the
// component. The fixture counts its initialize() calls (the module stays
// loaded) and reports the count in its state chunk
TEST_F (Vst3PluginTest, ReloadComponentRecreatesInstance)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  // Pin the module so the fixture's initialize count survives the plugin's
  // own unload/reload cycle (the module binary is unloaded when its last
  // reference dies)
  const auto &path = std::get<std::filesystem::path> (
    plugin_->configuration ()->descr_->path_or_id_);
  std::string module_error;
  const auto  module_pin = VST3::Hosting::Module::create (
    utils::Utf8String::from_path (path).str (), module_error);
  ASSERT_NE (module_pin, nullptr) << module_error;

  const auto count_before =
    read_controller_state_json (*plugin_).at ("initializeCount").get<int> ();
  ASSERT_GE (count_before, 1);

  auto * trigger = find_param_by_label ("Trigger Reload");
  ASSERT_NE (trigger, nullptr);
  trigger->setBaseValue (1.0f);
  process_blocks (1);

  process_events_until_true ([this] { return paused_processing_calls_ >= 1; });
  process_blocks (1);

  EXPECT_EQ (
    read_controller_state_json (*plugin_).at ("initializeCount").get<int> (),
    count_before + 1);
}

// The fixture only accepts a stereo/stereo bus configuration; a matching
// request succeeds and leaves the ports as they are
TEST_F (Vst3PluginTest, SetBusArrangementsAcceptedForMatchingLayout)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  const std::array layouts{ dsp::SpeakerArrangement::stereo () };
  EXPECT_TRUE (plugin_->set_bus_arrangements (layouts, layouts));

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 1);
  EXPECT_EQ (
    audio_outs.front ()->arrangement (), dsp::SpeakerArrangement::stereo ());
}

// The fixture refuses anything but stereo/stereo; the host reports the
// refusal and the ports stay untouched
TEST_F (Vst3PluginTest, SetBusArrangementsRefusedForUnsupportedLayout)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  constexpr auto surround51 = dsp::SpeakerArrangement::from_speaker_bits (
    std::to_underlying (dsp::SpeakerArrangement::Speaker::Left)
    | std::to_underlying (dsp::SpeakerArrangement::Speaker::Right)
    | std::to_underlying (dsp::SpeakerArrangement::Speaker::Center)
    | std::to_underlying (dsp::SpeakerArrangement::Speaker::Lfe)
    | std::to_underlying (dsp::SpeakerArrangement::Speaker::LeftSurround)
    | std::to_underlying (dsp::SpeakerArrangement::Speaker::RightSurround));
  const std::array layouts{ surround51 };
  EXPECT_FALSE (plugin_->set_bus_arrangements (layouts, layouts));

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 1);
  EXPECT_EQ (
    audio_outs.front ()->arrangement (), dsp::SpeakerArrangement::stereo ());
}

// Discrete arrangements have no VST3 representation, so the request keeps
// the bus's current arrangement and the (stereo) fixture accepts it
TEST_F (Vst3PluginTest, SetBusArrangementsLeavesBusUnchangedForDiscrete)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Gain"));

  const std::array layouts{ dsp::SpeakerArrangement::discrete_channels (2) };
  EXPECT_TRUE (plugin_->set_bus_arrangements (layouts, layouts));

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 1);
  EXPECT_EQ (
    audio_outs.front ()->arrangement (), dsp::SpeakerArrangement::stereo ());
}

// When the plugin grows a bus and reports kIoChanged, the host reconciles
// its port topology with the live layout: a new port appears with the new
// bus's name and arrangement
TEST_F (Vst3PluginTest, IoChangeWithNewBusCreatesPort)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  ASSERT_EQ (attached_audio_ports (false).size (), 1);

  auto * trigger = find_param_by_label ("Grow Output");
  ASSERT_NE (trigger, nullptr);
  trigger->setBaseValue (1.0f);
  process_blocks (1);

  // The restart request is handled asynchronously on the main thread
  process_events_until_true ([this] { return paused_processing_calls_ >= 1; });

  const auto audio_outs = attached_audio_ports (false);
  ASSERT_EQ (audio_outs.size (), 2);
  EXPECT_EQ (audio_outs.at (1)->get_label (), u8"Out 2");
  EXPECT_EQ (
    audio_outs.at (1)->arrangement (), dsp::SpeakerArrangement::stereo ());
}

// When the plugin removes a bus and reports kIoChanged, the corresponding
// port is detached, not destroyed: the object (and any connections to it)
// survives and revives if the bus returns
TEST_F (Vst3PluginTest, IoChangeWithRemovedBusDetachesPort)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));
  install_direct_paused_processing ();

  auto * grow = find_param_by_label ("Grow Output");
  ASSERT_NE (grow, nullptr);
  grow->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 1; });

  const auto audio_outs_after_grow = attached_audio_ports (false);
  ASSERT_EQ (audio_outs_after_grow.size (), 2);
  auto * const added_port = audio_outs_after_grow.at (1);

  auto * shrink = find_param_by_label ("Shrink Output");
  ASSERT_NE (shrink, nullptr);
  shrink->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 2; });

  EXPECT_EQ (attached_audio_ports (false).size (), 1);
  const auto all_outs = all_audio_ports (false);
  ASSERT_EQ (all_outs.size (), 2);
  EXPECT_EQ (all_outs.at (1), added_port);
  EXPECT_TRUE (all_outs.at (1)->detached ());
}

// Reconciling ports drops the buffers of ports whose arrangement changed
// and creates new ports unprepared; the graph recalculation that prepares
// them must happen before processing resumes, not after
TEST_F (Vst3PluginTest, IoChangeLeavesPortBuffersPreparedBeforeResume)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("Test Restart"));

  PluginHostMainThreadCallbacks callbacks{};
  // Stand in for the graph recalculation: it re-prepares the plugin,
  // reallocating the port buffers
  callbacks.graph_recalc_ = [this] {
    plugin_->prepare_for_processing (
      nullptr, units::sample_rate (48000), units::samples (256));
  };
  callbacks.with_paused_processing_ = [this] (const std::function<void ()> &fn) {
    ++paused_processing_calls_;
    fn ();
    // Processing may resume as soon as this returns: every attached port
    // must already have a buffer matching its arrangement
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
  };
  plugin_->set_main_thread_services (*main_dispatcher_, callbacks);

  auto * trigger = find_param_by_label ("Grow Output");
  ASSERT_NE (trigger, nullptr);
  trigger->setBaseValue (1.0f);
  process_blocks (1);
  process_events_until_true ([this] { return paused_processing_calls_ >= 1; });

  EXPECT_EQ (attached_audio_ports (false).size (), 2);
}

} // namespace zrythm::plugins
