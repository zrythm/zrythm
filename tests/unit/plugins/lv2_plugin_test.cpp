// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <cmath>
#include <filesystem>
#include <memory>
#include <ranges>
#include <set>
#include <string_view>
#include <vector>

#include "dsp/midi_event.h"
#include "plugins/lv2_plugin.h"
#include "plugins/lv2_plugin_format.h"
#include "plugins/lv2_world.h"
#include "plugins/plugin_configuration.h"
#include "plugins/plugin_descriptor.h"
#include "utils/audio.h"
#include "utils/base64.h"
#include "utils/object_registry.h"
#include "utils/serialization.h"
#include "utils/zip_utils.h"

#include <QByteArray>
#include <QCoreApplication>

#include "helpers/mock_plugin_host_window.h"
#include "helpers/scoped_juce_qapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::plugins
{

class Lv2PluginTest
    : public ::testing::Test,
      private test_helpers::ScopedJuceQApplication
{
protected:
  static std::filesystem::path bundle_path (const char * bundle_name)
  {
    return std::filesystem::path{ TEST_LV2_SEARCH_PATHS } / bundle_name;
  }

  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    mock_transport_ =
      std::make_unique<::testing::NiceMock<dsp::graph_test::MockTransport>> ();
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
    world_ =
      std::make_shared<Lv2World> (Lv2PluginFormat::get_spec_bundles_dir ());
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
    world_.reset ();
  }

  /**
   * @brief Loads the plugin at @p plugin_index of @p bundle_name,
   * optionally preparing it for processing at 48 kHz / 256 samples.
   * Without @p pause_callbacks the host cannot pause processing, so
   * re-instantiation is refused and the current instance is kept.
   */
  void load_test_plugin (
    const char * bundle_name,
    bool         prepare = true,
    bool         pause_callbacks = true,
    int          plugin_index = 0)
  {
    Lv2PluginFormat                           format{ world_ };
    juce::OwnedArray<juce::PluginDescription> found;
    format.findAllTypesForFile (
      found,
      utils::Utf8String::from_path (bundle_path (bundle_name)).to_juce_string ());
    ASSERT_FALSE (found.isEmpty ()) << "No plugins found in " << bundle_name;
    ASSERT_LT (plugin_index, found.size ())
      << "Bundle has no plugin index " << plugin_index;

    auto config = std::make_unique<PluginConfiguration> ();
    config->descr_ =
      PluginDescriptor::from_juce_description (*found[plugin_index]);
    ASSERT_NE (config->descr_, nullptr);

    window_state_ = std::make_shared<test_helpers::MockPluginHostWindowState> ();
    plugin_ = std::make_unique<Lv2Plugin> (
      *registry_, world_, std::function<units::sample_rate_t ()>{},
      std::function<units::sample_u32_t ()>{},
      test_helpers::make_mock_plugin_host_window_factory (window_state_));
    // Tests run without an audio thread, so "pausing" processing is a
    // pass-through
    PluginHostMainThreadCallbacks main_thread_callbacks;
    if (pause_callbacks)
      {
        main_thread_callbacks.with_paused_processing_ =
          [] (std::function<void ()> action) { action (); };
      }
    plugin_->set_main_thread_services (
      *main_dispatcher_, std::move (main_thread_callbacks));
    plugin_->set_configuration (*config);
    ASSERT_FALSE (plugin_->get_all_output_ports ().empty ())
      << "Plugin failed to load";

    if (prepare)
      {
        plugin_->prepare_for_processing (
          nullptr, units::sample_rate (48000), units::samples (256));
      }
  }

  dsp::ProcessorParameter * find_param_by_unique_id (std::string_view unique_id)
  {
    for (const auto &param_ref : plugin_->get_parameters ())
      {
        auto * p = param_ref.get_object_as<dsp::ProcessorParameter> ();
        if (type_safe::get (p->get_unique_id ()).view () == unique_id)
          return p;
      }
    return nullptr;
  }

  dsp::AudioPort * first_audio_port (dsp::PortFlow flow)
  {
    const auto ports = plugin_->get_attached_audio_ports (flow);
    EXPECT_FALSE (ports.empty ());
    return ports.empty () ? nullptr : ports.front ();
  }

  void fill_input_with (float value)
  {
    auto * in_port = first_audio_port (dsp::PortFlow::Input);
    ASSERT_NE (in_port, nullptr);
    ASSERT_NE (in_port->buffers (), nullptr);
    auto * data = in_port->buffers ()->getWritePointer (0);
    for (const auto i : std::views::iota (0, 256))
      {
        data[i] = value;
      }
  }

  float read_first_output_sample ()
  {
    auto * out_port = first_audio_port (dsp::PortFlow::Output);
    EXPECT_NE (out_port, nullptr);
    EXPECT_NE (out_port->buffers (), nullptr);
    return out_port->buffers ()->getReadPointer (0)[0];
  }

  void process_blocks (int num_blocks)
  {
    const dsp::graph::ProcessBlockInfo time_nfo{
      .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = units::samples (256),
    };
    for (int i = 0; i < num_blocks; ++i)
      {
        plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);
      }
  }

  dsp::MidiPort * midi_in_port ()
  {
    for (const auto &port_ref : plugin_->get_all_input_ports ())
      {
        if (auto * port = port_ref.get_object_as<dsp::MidiPort> ())
          return port;
      }
    return nullptr;
  }

  dsp::MidiPort * midi_out_port ()
  {
    for (const auto &port_ref : plugin_->get_all_output_ports ())
      {
        if (auto * port = port_ref.get_object_as<dsp::MidiPort> ())
          return port;
      }
    return nullptr;
  }

  std::unique_ptr<utils::ObjectRegistry> registry_;
  std::unique_ptr<::testing::NiceMock<dsp::graph_test::MockTransport>>
                                                           mock_transport_;
  std::unique_ptr<dsp::TempoMap>                           tempo_map_;
  std::unique_ptr<QObject>                                 dispatcher_context_;
  std::unique_ptr<utils::MainThreadClosureDispatcher>      main_dispatcher_;
  std::shared_ptr<Lv2World>                                world_;
  std::unique_ptr<Lv2Plugin>                               plugin_;
  std::shared_ptr<test_helpers::MockPluginHostWindowState> window_state_;
};

// Ungrouped mono audio ports form one mono bus per flow, and the control
// input becomes a parameter carrying the port's range and unit
TEST_F (Lv2PluginTest, CreatesPortsAndParameterForAmp)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-amp.lv2"));

  EXPECT_EQ (
    plugin_->get_attached_audio_ports (dsp::PortFlow::Input).size (), 1);
  EXPECT_EQ (
    plugin_->get_attached_audio_ports (dsp::PortFlow::Output).size (), 1);
  EXPECT_EQ (
    plugin_->get_attached_audio_ports (dsp::PortFlow::Input)
      .front ()
      ->num_channels (),
    1);

  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);
  const auto range = gain->range ();
  EXPECT_NEAR (range.convertFrom0To1 (0.f), -90.f, 0.001f);
  EXPECT_NEAR (range.convertFrom0To1 (1.f), 24.f, 0.001f);
  // The port default (0 dB) seeds the parameter's base value
  EXPECT_NEAR (gain->baseValue (), range.convertTo0To1 (0.f), 0.001f);
  EXPECT_TRUE (gain->automatable ());
}

// The plugin multiplies its input by the linear gain of the dB parameter
// value
TEST_F (Lv2PluginTest, AppliesGainParameterToAudio)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-amp.lv2"));

  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);

  // -6.0206 dB is a linear factor of 0.5
  gain->setBaseValue (gain->range ().convertTo0To1 (-6.0206f));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 0.5f, 0.01f);
}

// The TTL state snapshot restores the control port value into both the
// instance and the Zrythm parameter
TEST_F (Lv2PluginTest, StateRoundTripRestoresGain)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-amp.lv2"));

  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);

  // 6.0206 dB is a linear factor of 2
  const auto doubled = gain->range ().convertTo0To1 (6.0206f);
  gain->setBaseValue (doubled);
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);

  const auto state = plugin_->save_state ();
  ASSERT_FALSE (state.empty ());

  gain->setBaseValue (gain->range ().convertTo0To1 (0.f));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 1.f, 0.01f);

  EXPECT_TRUE (plugin_->load_state (state));
  EXPECT_NEAR (gain->baseValue (), doubled, 0.001f);
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);
}

// Files created by the plugin through state:makePath travel inside the
// state archive: the marker file written during save is read back during
// restore and surfaces as the output DC offset, both on the same
// instance and on a fresh instance (the blob is self-contained)
TEST_F (Lv2PluginTest, StateFilesSurviveRoundTrip)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("state-files.lv2"));

  // The gain port range is 0..1 linear; a value of 1 stores a marker
  // byte of 255
  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);
  gain->setBaseValue (gain->range ().convertTo0To1 (1.f));

  fill_input_with (1.f);
  process_blocks (1);
  // The state carried across the preparation re-instantiation was
  // saved with the default gain, so both restored marker bytes are 0
  EXPECT_NEAR (read_first_output_sample (), 0.f, 0.01f);

  const auto state = plugin_->save_state ();
  ASSERT_FALSE (state.empty ());

  // The archive holds exactly the TTL, the manifest lilv writes
  // alongside it, and the three state files: marker.bin written
  // during save, sub/runtime.bin created at instantiate time and
  // carried into the archive through lilv's path mapping, and
  // restored.bin created when the carried state was applied at
  // preparation
  const auto entries = utils::zip_utils::extract (
    utils::base64::decode (QByteArray::fromStdString (state)), 100, 1 << 20);
  const auto paths = [&entries] () {
    std::set<std::string> result;
    for (const auto &entry : entries)
      result.insert (entry.path_);
    return result;
  }();
  EXPECT_EQ (
    paths,
    (std::set<std::string>{
      "manifest.ttl", "marker.bin", "restored.bin", "state.ttl",
      "sub/runtime.bin" }));
  for (const auto * name : { "marker.bin", "restored.bin", "sub/runtime.bin" })
    {
      const auto it = std::ranges::find_if (entries, [name] (const auto &entry) {
        return entry.path_ == name;
      });
      ASSERT_NE (it, entries.end ());
      ASSERT_EQ (it->data_.size (), 1);
      EXPECT_EQ (static_cast<uint8_t> (it->data_[0]), 255) << name;
    }

  // The output is the sum of both restored marker bytes
  EXPECT_TRUE (plugin_->load_state (state));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 510.f, 0.01f);

  // The file the plugin created during restore() through the
  // restore-scoped makePath is rooted like a runtime file: the next
  // saved state carries it inside the archive (with the byte
  // restore() wrote)
  const auto expect_restored_file = [] (const std::string &blob) {
    const auto archive_entries = utils::zip_utils::extract (
      utils::base64::decode (QByteArray::fromStdString (blob)), 100, 1 << 20);
    std::set<std::string> archive_paths;
    for (const auto &entry : archive_entries)
      archive_paths.insert (entry.path_);
    EXPECT_EQ (
      archive_paths,
      (std::set<std::string>{
        "manifest.ttl", "marker.bin", "restored.bin", "state.ttl",
        "sub/runtime.bin" }));
    const auto it =
      std::ranges::find_if (archive_entries, [] (const auto &entry) {
        return entry.path_ == "restored.bin";
      });
    ASSERT_NE (it, archive_entries.end ());
    ASSERT_EQ (it->data_.size (), 1);
    EXPECT_EQ (static_cast<uint8_t> (it->data_[0]), 255);
  };
  const auto state_after_restore = plugin_->save_state ();
  ASSERT_FALSE (state_after_restore.empty ());
  expect_restored_file (state_after_restore);

  // A fresh instance (whose session files are unrelated to the first
  // instance's) restores from the same blob
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("state-files.lv2"));
  EXPECT_TRUE (plugin_->load_state (state));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 510.f, 0.01f);
  const auto fresh_state_after_restore = plugin_->save_state ();
  ASSERT_FALSE (fresh_state_after_restore.empty ());
  expect_restored_file (fresh_state_after_restore);
}

// A change of the processing sample rate re-instantiates the plugin,
// carrying the current state over
TEST_F (Lv2PluginTest, SampleRateChangeCarriesStateOver)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-amp.lv2"));

  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);
  gain->setBaseValue (gain->range ().convertTo0To1 (6.0206f));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);

  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (44100), units::samples (256));

  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);
}

// Without the ability to pause processing, a stale instance is kept and
// blocks larger than the block length it was created for are dropped
// with zeroed output instead of running past its buffers; the dropped
// chunk only zeroes its own window of the cycle
TEST_F (Lv2PluginTest, OversizedBlocksAreDroppedOnAStaleInstance)
{
  // Prepared with pause callbacks first, so ports pair with the
  // instance's buses and audio routes to the engine buffers
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (
    "eg-amp.lv2", /*prepare=*/true, /*pause_callbacks=*/true));

  // Dropping the pause callbacks keeps further re-instantiations from
  // running: the 256-frame instance is kept when preparation moves to
  // 4096-frame blocks
  plugin_->set_main_thread_services (
    *main_dispatcher_, PluginHostMainThreadCallbacks{});
  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (4096));

  // A block within the stale instance's bounds is processed normally
  // (the default 0 dB gain passes the input through)
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 1.f, 0.01f);

  fill_input_with (1.f);
  const dsp::graph::ProcessBlockInfo time_nfo{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (256),
    .nframes_ = units::samples (3840),
  };
  plugin_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  // The output the first chunk wrote survives the dropped chunk
  EXPECT_NEAR (read_first_output_sample (), 1.f, 0.01f);
}

// MIDI input is forged into the atom sequence and MIDI output is parsed
// back: eg-fifths transposes notes by a perfect fifth (7 semitones)
TEST_F (Lv2PluginTest, FifthsTransposesMidiNotes)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-fifths.lv2"));

  auto * midi_in = midi_in_port ();
  ASSERT_NE (midi_in, nullptr);
  auto * midi_out = midi_out_port ();
  ASSERT_NE (midi_out, nullptr);

  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  process_blocks (1);

  bool found_transposed = false;
  for (const auto &ev : midi_out->buffer_)
    {
      const auto data = ev.data ();
      if (data.size () == 3 && (data[0] & 0xF0) == 0x90 && data[1] == 67)
        {
          found_transposed = true;
          break;
        }
    }
  EXPECT_TRUE (found_transposed) << "No transposed note-on found in the output";
}

// The input port supports both MIDI and time:Position: both arrive in
// the same sequence, and the position's framesPerSecond property is an
// atom:Float frame-timed event (the fixture emits it as a DC offset of
// frames-per-second / sample-rate)
TEST_F (Lv2PluginTest, PositionFramesPerSecondIsDeliveredAsFloat)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("pos-probe.lv2"));

  process_blocks (1);

  EXPECT_NEAR (read_first_output_sample (), 1.f, 0.001f);
}

// A control port annotated units:degree maps to the degrees parameter
// unit
TEST_F (Lv2PluginTest, DegreeUnitIsMapped)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("pos-probe.lv2"));

  auto * degree = find_param_by_unique_id ("degree"sv);
  ASSERT_NE (degree, nullptr);
  EXPECT_EQ (degree->range ().unit_, dsp::ParameterRange::Unit::Degrees);
}

// A plugin with separate MIDI and time:Position atom inputs receives
// the position on its own port (the fixture emits frames-per-second /
// sample-rate as DC)
TEST_F (Lv2PluginTest, SeparateTimePortReceivesPosition)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (
    "pos-probe.lv2", /*prepare=*/true, /*pause_callbacks=*/true,
    /*plugin_index=*/1));

  process_blocks (1);

  EXPECT_NEAR (read_first_output_sample (), 1.f, 0.001f);
}

// A latency report that is not a finite number is ignored
TEST_F (Lv2PluginTest, NonFiniteLatencyReportIsIgnored)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (
    "pos-probe.lv2", /*prepare=*/true, /*pause_callbacks=*/true,
    /*plugin_index=*/1));

  process_blocks (1);

  EXPECT_EQ (plugin_->get_single_playback_latency (), units::samples (0u));
}

// An rsz:minimumSize declaration sizes the input sequence buffer: a
// dense event stream larger than the default capacity is delivered in
// full (the fixture adds the MIDI event count to its DC offset)
TEST_F (Lv2PluginTest, MinimumSizeDeclarationBuffersDenseEventStreams)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (
    "pos-probe.lv2", /*prepare=*/true, /*pause_callbacks=*/true,
    /*plugin_index=*/1));

  auto * midi_in = midi_in_port ();
  ASSERT_NE (midi_in, nullptr);
  // The fixture echoes received events to its MIDI output, so both
  // engine-side MIDI buffers are prepared for the full stream upfront
  auto * midi_out = midi_out_port ();
  ASSERT_NE (midi_out, nullptr);

  // 2000 note-on events (19 bytes forged each) exceed the default
  // 16 KiB capacity but fit the declared 64 KiB
  constexpr auto num_events = 2000;
  const auto     note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in->buffer_.reserve (
    num_events * (dsp::MidiEventBuffer::kHeaderSize + note_on.data ().size ()));
  midi_out->buffer_.reserve (
    num_events * (dsp::MidiEventBuffer::kHeaderSize + note_on.data ().size ()));
  for (const auto _ : std::views::iota (0, num_events))
    {
      midi_in->buffer_.push_back (note_on.time_, note_on.data ());
    }

  process_blocks (1);

  // 1 is the position DC (frames per second / sample rate)
  EXPECT_NEAR (read_first_output_sample (), 1.f + num_events, 0.001f);
}

// An rsz:minimumSize declaration also sizes the routed output sequence
// buffer: the fixture echoes every received MIDI event into an output
// buffer sized by its own declaration
TEST_F (Lv2PluginTest, EchoedDenseOutputStreamFitsDeclaredMinimumSize)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin (
    "pos-probe.lv2", /*prepare=*/true, /*pause_callbacks=*/true,
    /*plugin_index=*/1));

  auto * midi_in = midi_in_port ();
  ASSERT_NE (midi_in, nullptr);
  auto * midi_out = midi_out_port ();
  ASSERT_NE (midi_out, nullptr);

  constexpr auto num_events = 2000;
  const auto     note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  // Both engine-side MIDI buffers are prepared for the full stream
  // upfront like an engine would
  midi_in->buffer_.reserve (
    num_events * (dsp::MidiEventBuffer::kHeaderSize + note_on.data ().size ()));
  midi_out->buffer_.reserve (
    num_events * (dsp::MidiEventBuffer::kHeaderSize + note_on.data ().size ()));
  for (const auto _ : std::views::iota (0, num_events))
    {
      midi_in->buffer_.push_back (note_on.time_, note_on.data ());
    }

  process_blocks (1);

  EXPECT_EQ (midi_out->buffer_.size (), num_events);
}

// A MIDI event larger than the sequence buffer is skipped whole, so the
// sequence stays valid and events after it are still delivered
TEST_F (Lv2PluginTest, OversizedMidiEventIsSkippedAndLaterEventsDelivered)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-fifths.lv2"));

  auto * midi_in = midi_in_port ();
  ASSERT_NE (midi_in, nullptr);
  auto * midi_out = midi_out_port ();
  ASSERT_NE (midi_out, nullptr);

  // A system-exclusive message far larger than the 16 KiB buffer
  std::vector<midi_byte_t> sysex (20003, 0);
  sysex.front () = 0xF0;
  sysex.back () = 0xF7;
  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  // The event stream exceeds the port's default reservation, so the
  // buffer is prepared for it upfront like an engine would
  midi_in->buffer_.reserve (
    2 * dsp::MidiEventBuffer::kHeaderSize + sysex.size ()
    + note_on.data ().size ());
  midi_in->buffer_.push_back (units::samples (0u), sysex);
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  process_blocks (1);

  bool found_transposed = false;
  for (const auto &ev : midi_out->buffer_)
    {
      const auto data = ev.data ();
      if (data.size () == 3 && (data[0] & 0xF0) == 0x90 && data[1] == 67)
        {
          found_transposed = true;
          break;
        }
    }
  EXPECT_TRUE (found_transposed)
    << "Note-on after the oversized event was not delivered";
}

// A note-on produces non-silent audio output, and the fixture's CV output
// port is written every block
TEST_F (Lv2PluginTest, InstrumentNoteOnProducesAudioAndCv)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("test-instrument.lv2"));

  auto * midi_in = midi_in_port ();
  ASSERT_NE (midi_in, nullptr);
  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  process_blocks (5);

  auto * out_port = first_audio_port (dsp::PortFlow::Output);
  ASSERT_NE (out_port, nullptr);
  EXPECT_TRUE (utils::audio::buffer_has_audio (*out_port->buffers (), 0, 256))
    << "Plugin produced silent output for note-on";

  dsp::CVPort * cv_out = nullptr;
  for (const auto &port_ref : plugin_->get_all_output_ports ())
    {
      cv_out = port_ref.get_object_as<dsp::CVPort> ();
      if (cv_out != nullptr)
        break;
    }
  ASSERT_NE (cv_out, nullptr);
  ASSERT_FALSE (cv_out->buf_.empty ());
  EXPECT_NEAR (cv_out->buf_.front (), 1.f, 0.0001f);
}

// Deserialization restores the serialized MIDI and CV ports: the
// configuration change after port restoration must not append duplicates
TEST_F (Lv2PluginTest, DeserializationKeepsMidiAndCvPortCount)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("test-instrument.lv2"));

  const auto count_midi_in = [] (const Lv2Plugin &plugin) {
    return std::ranges::count_if (
      plugin.get_all_input_ports (), [] (const auto &port_ref) {
        return port_ref.template get_object_as<dsp::MidiPort> () != nullptr;
      });
  };
  const auto count_cv_out = [] (const Lv2Plugin &plugin) {
    return std::ranges::count_if (
      plugin.get_all_output_ports (), [] (const auto &port_ref) {
        return port_ref.template get_object_as<dsp::CVPort> () != nullptr;
      });
  };
  ASSERT_EQ (count_midi_in (*plugin_), 1);
  ASSERT_EQ (count_cv_out (*plugin_), 1);

  nlohmann::json json;
  to_json (json, *plugin_);

  auto deserialized = std::make_unique<Lv2Plugin> (
    *registry_, world_, std::function<units::sample_rate_t ()>{},
    std::function<units::sample_u32_t ()>{},
    test_helpers::make_mock_plugin_host_window_factory (window_state_));
  from_json (json, *deserialized);

  EXPECT_EQ (count_midi_in (*deserialized), 1);
  EXPECT_EQ (count_cv_out (*deserialized), 1);
}

// The instance is created eagerly when the configuration arrives, before
// any processing preparation: a state snapshot is possible right away
TEST_F (Lv2PluginTest, InstanceIsLiveAtConfiguration)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-amp.lv2", /*prepare=*/false));

  EXPECT_FALSE (plugin_->save_state ().empty ());
}

// Releasing the resources (every hard graph rechain does) frees the
// instance; the snapshotted state is re-applied at the next processing
// preparation
TEST_F (Lv2PluginTest, StateSurvivesReleaseAndReprepare)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-amp.lv2"));

  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);
  // 6.0206 dB is a linear factor of 2
  gain->setBaseValue (gain->range ().convertTo0To1 (6.0206f));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);

  plugin_->release_resources ();
  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));

  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);
}

// A pending state that fails to restore is kept and retried on every
// processing preparation; the plugin keeps processing and a valid state
// still applies afterwards
TEST_F (Lv2PluginTest, FailedPendingRestoreIsRetried)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-amp.lv2"));

  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);
  const auto doubled = gain->range ().convertTo0To1 (6.0206f);
  gain->setBaseValue (doubled);
  // Flush the parameter change into the control buffer before saving
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);

  const auto valid_state = plugin_->save_state ();
  ASSERT_FALSE (valid_state.empty ());

  plugin_->release_resources ();

  // Non-archive garbage becomes pending and fails to restore at
  // preparation
  const auto garbage_state =
    utils::to_std_string (QByteArray ("not a zip archive").toBase64 ());
  EXPECT_TRUE (plugin_->load_state (garbage_state));
  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));
  // Retry on a live, unchanged instance
  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));

  // The failed restores leave a healthy instance that still processes
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_TRUE (std::isfinite (read_first_output_sample ()));

  // A valid state applies to the live instance and restores the gain
  EXPECT_TRUE (plugin_->load_state (valid_state));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 2.f, 0.01f);
}

// Enumeration parameters are index-valued, but the LV2 port expects the
// selected scale point's value: index 1 of {-6, 0, +6} dB is 0 dB (a
// factor of 1), not 1 dB
TEST_F (Lv2PluginTest, EnumerationPortUsesScalePointValues)
{
  ASSERT_NO_FATAL_FAILURE (load_test_plugin ("eg-enum.lv2"));

  auto * gain = find_param_by_unique_id ("gain"sv);
  ASSERT_NE (gain, nullptr);
  ASSERT_EQ (gain->range ().enumCount (), 3)
    << "gain is not an enumeration parameter";

  gain->setBaseValue (gain->range ().normalizedEnumValue (1));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 1.f, 0.01f);

  // The stored state carries scale point values: restoring it selects the
  // same scale point again
  const auto state = plugin_->save_state ();
  ASSERT_FALSE (state.empty ());

  gain->setBaseValue (gain->range ().normalizedEnumValue (0));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 0.5f, 0.01f);

  EXPECT_TRUE (plugin_->load_state (state));
  fill_input_with (1.f);
  process_blocks (1);
  EXPECT_NEAR (read_first_output_sample (), 1.f, 0.01f);
}

// A plugin with malformed port data is refused at configuration time and
// leaves no partially initialized state behind: processing preparation
// is a no-op and no state exists
TEST_F (Lv2PluginTest, MalformedPluginIsRefusedCleanly)
{
  Lv2PluginFormat                           format{ world_ };
  juce::OwnedArray<juce::PluginDescription> found;
  format.findAllTypesForFile (
    found,
    utils::Utf8String::from_path (bundle_path ("broken.lv2")).to_juce_string ());
  ASSERT_FALSE (found.isEmpty ()) << "No plugins found in broken.lv2";

  auto config = std::make_unique<PluginConfiguration> ();
  config->descr_ = PluginDescriptor::from_juce_description (*found[0]);
  ASSERT_NE (config->descr_, nullptr);

  window_state_ = std::make_shared<test_helpers::MockPluginHostWindowState> ();
  plugin_ = std::make_unique<Lv2Plugin> (
    *registry_, world_, std::function<units::sample_rate_t ()>{},
    std::function<units::sample_u32_t ()>{},
    test_helpers::make_mock_plugin_host_window_factory (window_state_));
  PluginHostMainThreadCallbacks main_thread_callbacks;
  main_thread_callbacks.with_paused_processing_ =
    [] (std::function<void ()> action) { action (); };
  plugin_->set_main_thread_services (
    *main_dispatcher_, std::move (main_thread_callbacks));

  plugin_->set_configuration (*config);
  EXPECT_TRUE (plugin_->get_all_output_ports ().empty ())
    << "Malformed plugin was not refused";

  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));
  EXPECT_TRUE (plugin_->save_state ().empty ())
    << "A refused load left a live instance behind";

  plugin_->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));
}

} // namespace zrythm::plugins
