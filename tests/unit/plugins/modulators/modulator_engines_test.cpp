// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <array>
#include <cmath>
#include <ranges>

#include "plugins/modulators/engines/adsr_engine.h"
#include "plugins/modulators/engines/envelope_follower_engine.h"
#include "plugins/modulators/engines/lfo_engine.h"
#include "plugins/modulators/engines/macro_engine.h"
#include "plugins/modulators/engines/math_engine.h"
#include "plugins/modulators/engines/midi_to_cv_engine.h"
#include "plugins/modulators/modulator.h"
#include "plugins/plugin_configuration.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "helpers/scoped_qcoreapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::plugins::modulators
{

namespace
{

constexpr auto kSampleRate = units::sample_rate (44100);
constexpr auto kBlockLength = units::samples (256);

dsp::ProcessorParameter *
find_parameter (dsp::ProcessorBase &processor, std::string_view unique_id)
{
  const auto found = std::ranges::find_if (
    processor.get_parameters (), [unique_id] (const auto &param_ref) {
      auto * param = param_ref.get ();
      return param != nullptr
             && param->get_unique_id ()
                  == dsp::ProcessorParameter::UniqueId (
                    utils::Utf8String::from_utf8_encoded_string (unique_id));
    });
  return found != processor.get_parameters ().end () ? found->get () : nullptr;
}

template <typename PortT>
PortT *
find_port (
  dsp::ProcessorBase &processor,
  dsp::PortFlow       flow,
  std::string_view    symbol)
{
  auto &ports =
    flow == dsp::PortFlow::Input
      ? processor.get_all_input_ports ()
      : processor.get_all_output_ports ();
  const auto found =
    std::ranges::find_if (ports, [symbol] (const auto &port_ref) {
      auto * port = port_ref.get ();
      return port != nullptr
             && port->get_symbol ()
                  == utils::Utf8String::from_utf8_encoded_string (symbol);
    });
  return found != ports.end () ? qobject_cast<PortT *> (found->get ()) : nullptr;
}

} // namespace

class ModulatorEnginesTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    transport_ = std::make_unique<dsp::graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (kSampleRate);
  }

  void TearDown () override
  {
    if (plugin_ != nullptr)
      plugin_->release_resources ();
    plugin_.reset ();
    // Source references must drop before the registry they point into.
    midi_source_ = nullptr;
    midi_source_ref_.reset ();
    sources_.clear ();
    registry_.reset ();
  }

  // Creates a modulator hosting the engine with @p descriptor_id and
  // prepares it for processing.
  void load_engine (std::string_view descriptor_id)
  {
    plugin_ = std::make_unique<Modulator> (*registry_);
    plugin_->set_configuration (*PluginConfiguration::create_new_for_descriptor (
      *Modulator::make_descriptor (descriptor_id)));
    plugin_->prepare_for_processing (nullptr, kSampleRate, kBlockLength);
  }

  dsp::graph::ProcessBlockInfo block_info (int64_t position = 0) const
  {
    return dsp::graph::ProcessBlockInfo::from_position_and_nframes (
      units::samples (position), kBlockLength);
  }

  // Creates a prepared CV source port filled with @p value and connects it
  // as the only source of @p input.
  void drive_cv_input (dsp::CVPort &input, float value)
  {
    auto source_ref = utils::create_object<dsp::CVPort> (
      *registry_, u8"Source", dsp::PortFlow::Output);
    auto * source = source_ref.get ();
    source->prepare_for_processing (nullptr, kSampleRate, kBlockLength);
    std::ranges::fill (source->buf_, value);
    const std::array sources{
      dsp::graph::PortSourceConfig<dsp::CVPort>{ source, std::nullopt }
    };
    input.set_port_sources (sources);
    sources_.push_back (std::move (source_ref));
  }

  // Pushes @p event_bytes at @p time into a prepared MIDI source port
  // connected as the only source of @p input.
  void push_midi_event (
    dsp::MidiPort &input,
    std::uint8_t   status,
    std::uint8_t   data1,
    std::uint8_t   data2,
    std::uint32_t  time)
  {
    if (midi_source_ == nullptr)
      {
        midi_source_ref_ = utils::create_object<dsp::MidiPort> (
          *registry_, u8"MIDI Source", dsp::PortFlow::Output);
        midi_source_ = midi_source_ref_->get ();
        midi_source_->prepare_for_processing (
          nullptr, kSampleRate, kBlockLength);
        const std::array sources{
          dsp::graph::PortSourceConfig<dsp::MidiPort>{ midi_source_, std::nullopt }
        };
        input.set_port_sources (sources);
      }
    const std::array<std::uint8_t, 3> bytes{ status, data1, data2 };
    midi_source_->buffer_.push_back (units::samples (time), bytes);
  }

  void process_midi_cycle (dsp::MidiPort &input, int64_t position = 0)
  {
    const auto time_info = block_info (position);
    input.process_block (time_info, *transport_, *tempo_map_);
    plugin_->process_block (time_info, *transport_, *tempo_map_);
    // The graph clears port buffers at the end of each cycle; mirror that
    // so source events do not leak into the next cycle.
    if (midi_source_ != nullptr)
      midi_source_->buffer_.clear ();
  }

  std::unique_ptr<utils::ObjectRegistry>                  registry_;
  std::unique_ptr<Modulator>                              plugin_;
  std::unique_ptr<dsp::graph_test::MockTransport>         transport_;
  std::unique_ptr<dsp::TempoMap>                          tempo_map_;
  std::vector<utils::TypedUuidReference<dsp::CVPort>>     sources_;
  std::optional<utils::TypedUuidReference<dsp::MidiPort>> midi_source_ref_;
  dsp::MidiPort *                                         midi_source_{};
};

// ---------------------------------------------------------------------------
// Registry
// ---------------------------------------------------------------------------

TEST_F (ModulatorEnginesTest, RegistryHoldsEveryNativeModulator)
{
  const auto ids = Modulator::available_descriptor_ids ();
  EXPECT_EQ (ids.size (), 6u);
  for (
    const auto * id :
    { &MacroEngine::kDescriptorId, &LfoEngine::kDescriptorId,
      &EnvelopeFollowerEngine::kDescriptorId, &MidiToCvEngine::kDescriptorId,
      &AdsrEngine::kDescriptorId, &MathEngine::kDescriptorId })
    {
      EXPECT_EQ (std::ranges::count (ids, *id), 1);
      EXPECT_TRUE (
        Modulator::descriptor_is_native_modulator (
          *Modulator::make_descriptor (*id)));
    }
}

// ---------------------------------------------------------------------------
// LFO
// ---------------------------------------------------------------------------

TEST_F (ModulatorEnginesTest, LfoSawRampsFromZero)
{
  load_engine (LfoEngine::kDescriptorId);
  auto * waveform = find_parameter (*plugin_, LfoEngine::kWaveformParameterId);
  ASSERT_NE (waveform, nullptr);
  waveform->setBaseValue (waveform->range ().normalizedEnumValue (2)); // Saw

  plugin_->process_block (block_info (), *transport_, *tempo_map_);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, LfoEngine::kOutputSymbol);
  ASSERT_NE (cv_out, nullptr);

  // Free-running at the default 1 Hz: each block advances the phase by
  // block / sample rate, so the saw tracks the per-sample phase exactly.
  const double increment = 1. / kSampleRate.in<double> (units::sample_rate);
  EXPECT_NEAR (cv_out->buf_[0], increment, 1e-6);
  EXPECT_NEAR (cv_out->buf_[255], 256. * increment, 1e-6);

  plugin_->process_block (
    block_info (kBlockLength.in<int64_t> (units::samples)), *transport_,
    *tempo_map_);
  EXPECT_NEAR (cv_out->buf_[0], 257. * increment, 1e-6);
}

TEST_F (ModulatorEnginesTest, LfoTempoSyncUsesTransportRate)
{
  load_engine (LfoEngine::kDescriptorId);
  auto * sync = find_parameter (*plugin_, LfoEngine::kSyncParameterId);
  auto * waveform = find_parameter (*plugin_, LfoEngine::kWaveformParameterId);
  ASSERT_NE (sync, nullptr);
  ASSERT_NE (waveform, nullptr);
  sync->setBaseValue (1.f);
  waveform->setBaseValue (waveform->range ().normalizedEnumValue (2)); // Saw

  // Default tempo is 120 BPM and the default division 1/4 (one beat per
  // cycle): the LFO runs at 2 Hz.
  plugin_->process_block (block_info (), *transport_, *tempo_map_);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, LfoEngine::kOutputSymbol);
  ASSERT_NE (cv_out, nullptr);

  const double increment = 2. / kSampleRate.in<double> (units::sample_rate);
  EXPECT_NEAR (cv_out->buf_[0], increment, 1e-6);
  EXPECT_NEAR (cv_out->buf_[255], 256. * increment, 1e-6);
}

TEST_F (ModulatorEnginesTest, LfoSyncedPhaseFollowsTimelinePosition)
{
  load_engine (LfoEngine::kDescriptorId);
  auto * sync = find_parameter (*plugin_, LfoEngine::kSyncParameterId);
  auto * waveform = find_parameter (*plugin_, LfoEngine::kWaveformParameterId);
  ASSERT_NE (sync, nullptr);
  ASSERT_NE (waveform, nullptr);
  sync->setBaseValue (1.f);
  waveform->setBaseValue (waveform->range ().normalizedEnumValue (2)); // Saw

  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, LfoEngine::kOutputSymbol);
  ASSERT_NE (cv_out, nullptr);

  // At the default 120 BPM with a 1/4 division, one cycle spans one beat
  // (22050 samples at this rate), so a block starting at position 100
  // begins with the beat fraction of that position.
  const double samples_per_beat =
    0.5 * kSampleRate.in<double> (units::sample_rate);
  const double increment = 2. / kSampleRate.in<double> (units::sample_rate);

  plugin_->process_block (block_info (100), *transport_, *tempo_map_);
  EXPECT_NEAR (cv_out->buf_[0], 100. / samples_per_beat + increment, 1e-6);

  // The same timeline window yields the same waveform regardless of
  // processing history: revisiting it directly or reaching it after a
  // different window both reproduce the first rendering exactly.
  std::array<float, 256> reference{};
  std::ranges::copy_n (cv_out->buf_.begin (), 256, reference.begin ());

  plugin_->process_block (block_info (100), *transport_, *tempo_map_);
  for (const auto i : std::views::iota (0, 256))
    EXPECT_FLOAT_EQ (cv_out->buf_[i], reference[i]) << "revisit sample " << i;

  plugin_->process_block (block_info (0), *transport_, *tempo_map_);
  plugin_->process_block (block_info (100), *transport_, *tempo_map_);
  for (const auto i : std::views::iota (0, 256))
    EXPECT_FLOAT_EQ (cv_out->buf_[i], reference[i]) << "history sample " << i;
}

TEST_F (ModulatorEnginesTest, LfoSquareGateAlternates)
{
  load_engine (LfoEngine::kDescriptorId);
  auto * waveform = find_parameter (*plugin_, LfoEngine::kWaveformParameterId);
  auto * rate = find_parameter (*plugin_, LfoEngine::kRateParameterId);
  ASSERT_NE (waveform, nullptr);
  ASSERT_NE (rate, nullptr);
  waveform->setBaseValue (waveform->range ().normalizedEnumValue (3)); // Square
  rate->setBaseValue (1.f); // Maximum free-running rate: 20 Hz

  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, LfoEngine::kOutputSymbol);
  ASSERT_NE (cv_out, nullptr);

  // At 20 Hz half a cycle lasts 1102.5 samples: four blocks stay low and
  // the fifth flips mid-block.
  for (const auto block : std::views::iota (0, 4))
    {
      plugin_->process_block (
        block_info (block * kBlockLength.in<int64_t> (units::samples)),
        *transport_, *tempo_map_);
      EXPECT_FLOAT_EQ (cv_out->buf_[255], 0.f);
    }
  plugin_->process_block (
    block_info (4 * kBlockLength.in<int64_t> (units::samples)), *transport_,
    *tempo_map_);
  EXPECT_FLOAT_EQ (cv_out->buf_[77], 0.f);
  EXPECT_FLOAT_EQ (cv_out->buf_[78], 1.f);
  EXPECT_FLOAT_EQ (cv_out->buf_[255], 1.f);
}

// ---------------------------------------------------------------------------
// Envelope Follower
// ---------------------------------------------------------------------------

TEST_F (ModulatorEnginesTest, EnvelopeFollowerTracksConstantInput)
{
  load_engine (EnvelopeFollowerEngine::kDescriptorId);
  auto * audio_in = find_port<dsp::AudioPort> (
    *plugin_, dsp::PortFlow::Input, EnvelopeFollowerEngine::kInputSymbol);
  ASSERT_NE (audio_in, nullptr);
  ASSERT_NE (audio_in->buffers (), nullptr);

  // A constant 0.5 input rises towards 0.5 with the default 10 ms attack.
  audio_in->buffers ()->clear ();
  for (
    const auto i : std::views::iota (0, kBlockLength.in<int> (units::samples)))
    audio_in->buffers ()->setSample (0, i, 0.5f);

  plugin_->process_block (block_info (), *transport_, *tempo_map_);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, EnvelopeFollowerEngine::kOutputSymbol);
  ASSERT_NE (cv_out, nullptr);

  const double coeff =
    std::exp (-1. / (0.01 * kSampleRate.in<double> (units::sample_rate)));
  const double expected = 0.5 * (1. - std::pow (coeff, 256.));
  EXPECT_NEAR (cv_out->buf_[0], 0.5 * (1. - coeff), 1e-4);
  EXPECT_NEAR (cv_out->buf_[255], expected, 1e-3);
  EXPECT_LT (cv_out->buf_[0], cv_out->buf_[255]);
}

TEST_F (ModulatorEnginesTest, EnvelopeFollowerReleasesAfterSilence)
{
  load_engine (EnvelopeFollowerEngine::kDescriptorId);
  auto * audio_in = find_port<dsp::AudioPort> (
    *plugin_, dsp::PortFlow::Input, EnvelopeFollowerEngine::kInputSymbol);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, EnvelopeFollowerEngine::kOutputSymbol);
  ASSERT_NE (audio_in, nullptr);
  ASSERT_NE (cv_out, nullptr);

  audio_in->buffers ()->clear ();
  for (
    const auto i : std::views::iota (0, kBlockLength.in<int> (units::samples)))
    audio_in->buffers ()->setSample (0, i, 0.5f);
  plugin_->process_block (block_info (), *transport_, *tempo_map_);
  const float held = cv_out->buf_[255];

  // Silence decays towards zero with the default 200 ms release.
  audio_in->buffers ()->clear ();
  plugin_->process_block (
    block_info (kBlockLength.in<int64_t> (units::samples)), *transport_,
    *tempo_map_);
  const double release_coeff =
    std::exp (-1. / (0.2 * kSampleRate.in<double> (units::sample_rate)));
  const double expected = held * std::pow (release_coeff, 256.);
  EXPECT_NEAR (cv_out->buf_[255], expected, 1e-3);
  EXPECT_LT (cv_out->buf_[255], held);
}

// ---------------------------------------------------------------------------
// MIDI to CV
// ---------------------------------------------------------------------------

TEST_F (ModulatorEnginesTest, MidiToCvOutputsHeldNote)
{
  load_engine (MidiToCvEngine::kDescriptorId);
  auto * midi_in = find_port<dsp::MidiPort> (
    *plugin_, dsp::PortFlow::Input, MidiToCvEngine::kInputSymbol);
  auto * pitch = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, MidiToCvEngine::kPitchSymbol);
  auto * gate = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, MidiToCvEngine::kGateSymbol);
  auto * velocity = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, MidiToCvEngine::kVelocitySymbol);
  ASSERT_NE (midi_in, nullptr);
  ASSERT_NE (pitch, nullptr);
  ASSERT_NE (gate, nullptr);
  ASSERT_NE (velocity, nullptr);

  push_midi_event (*midi_in, 0x90, 60, 100, 10);
  process_midi_cycle (*midi_in);

  // Before the note-on the gate is closed; from the event sample the
  // outputs carry the held note.
  EXPECT_FLOAT_EQ (gate->buf_[9], 0.f);
  EXPECT_FLOAT_EQ (gate->buf_[10], 1.f);
  EXPECT_FLOAT_EQ (gate->buf_[255], 1.f);
  EXPECT_FLOAT_EQ (pitch->buf_[10], 60.f / 127.f);
  EXPECT_FLOAT_EQ (velocity->buf_[10], 100.f / 127.f);

  // Releasing the held note closes the gate but keeps the last pitch.
  push_midi_event (*midi_in, 0x80, 60, 0, 0);
  process_midi_cycle (*midi_in, kBlockLength.in<int64_t> (units::samples));
  EXPECT_FLOAT_EQ (gate->buf_[0], 0.f);
  EXPECT_FLOAT_EQ (pitch->buf_[0], 60.f / 127.f);
  EXPECT_FLOAT_EQ (velocity->buf_[0], 100.f / 127.f);
}

TEST_F (ModulatorEnginesTest, MidiToCvAllNotesOffClosesGate)
{
  load_engine (MidiToCvEngine::kDescriptorId);
  auto * midi_in = find_port<dsp::MidiPort> (
    *plugin_, dsp::PortFlow::Input, MidiToCvEngine::kInputSymbol);
  auto * gate = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, MidiToCvEngine::kGateSymbol);
  ASSERT_NE (midi_in, nullptr);
  ASSERT_NE (gate, nullptr);

  push_midi_event (*midi_in, 0x90, 64, 127, 0);
  process_midi_cycle (*midi_in);
  EXPECT_FLOAT_EQ (gate->buf_[255], 1.f);

  // All-notes-off on channel 1 (control change 123).
  push_midi_event (*midi_in, 0xB0, 123, 0, 5);
  process_midi_cycle (*midi_in, kBlockLength.in<int64_t> (units::samples));
  EXPECT_FLOAT_EQ (gate->buf_[4], 1.f);
  EXPECT_FLOAT_EQ (gate->buf_[5], 0.f);
}

// ---------------------------------------------------------------------------
// ADSR
// ---------------------------------------------------------------------------

TEST_F (ModulatorEnginesTest, AdsrRunsThroughAttackAndDecay)
{
  load_engine (AdsrEngine::kDescriptorId);
  auto * midi_in = find_port<dsp::MidiPort> (
    *plugin_, dsp::PortFlow::Input, AdsrEngine::kInputSymbol);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, AdsrEngine::kOutputSymbol);
  auto * sustain = find_parameter (*plugin_, AdsrEngine::kSustainParameterId);
  ASSERT_NE (midi_in, nullptr);
  ASSERT_NE (cv_out, nullptr);
  ASSERT_NE (sustain, nullptr);
  sustain->setBaseValue (0.5f);

  // The default 10 ms attack spans 441 samples: within this block the
  // envelope rises but has not peaked yet.
  push_midi_event (*midi_in, 0x90, 60, 100, 0);
  process_midi_cycle (*midi_in);
  EXPECT_FLOAT_EQ (cv_out->buf_[0], 1.f / 441.f);
  EXPECT_LT (cv_out->buf_[100], 1.f);
  EXPECT_FLOAT_EQ (cv_out->buf_[255], 256.f / 441.f);
}

TEST_F (ModulatorEnginesTest, AdsrSustainsAndReleases)
{
  load_engine (AdsrEngine::kDescriptorId);
  auto * midi_in = find_port<dsp::MidiPort> (
    *plugin_, dsp::PortFlow::Input, AdsrEngine::kInputSymbol);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, AdsrEngine::kOutputSymbol);
  auto * sustain = find_parameter (*plugin_, AdsrEngine::kSustainParameterId);
  ASSERT_NE (midi_in, nullptr);
  ASSERT_NE (cv_out, nullptr);
  ASSERT_NE (sustain, nullptr);
  sustain->setBaseValue (0.75f);

  // Fastest attack and decay (1 ms) so the envelope reaches sustain within
  // the first block, then holds it.
  find_parameter (*plugin_, AdsrEngine::kAttackParameterId)->setBaseValue (0.f);
  find_parameter (*plugin_, AdsrEngine::kDecayParameterId)->setBaseValue (0.f);
  push_midi_event (*midi_in, 0x90, 60, 100, 0);
  process_midi_cycle (*midi_in);
  EXPECT_FLOAT_EQ (cv_out->buf_[255], 0.75f);

  // Releasing with the fastest release (1 ms) reaches zero within the next
  // block.
  find_parameter (*plugin_, AdsrEngine::kReleaseParameterId)->setBaseValue (0.f);
  push_midi_event (*midi_in, 0x80, 60, 0, 0);
  process_midi_cycle (*midi_in, kBlockLength.in<int64_t> (units::samples));
  EXPECT_FLOAT_EQ (cv_out->buf_[255], 0.f);
}

// ---------------------------------------------------------------------------
// Math
// ---------------------------------------------------------------------------

TEST_F (ModulatorEnginesTest, MathAppliesSelectedOperation)
{
  load_engine (MathEngine::kDescriptorId);
  auto * a_in = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Input, MathEngine::kInputASymbol);
  auto * b_in = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Input, MathEngine::kInputBSymbol);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, MathEngine::kOutputSymbol);
  auto * operation =
    find_parameter (*plugin_, MathEngine::kOperationParameterId);
  ASSERT_NE (a_in, nullptr);
  ASSERT_NE (b_in, nullptr);
  ASSERT_NE (cv_out, nullptr);
  ASSERT_NE (operation, nullptr);

  drive_cv_input (*a_in, 0.6f);
  drive_cv_input (*b_in, 0.4f);

  const auto process_and_expect = [&] (size_t op_index, float expected) {
    operation->setBaseValue (operation->range ().normalizedEnumValue (op_index));
    const auto time_info = block_info ();
    a_in->process_block (time_info, *transport_, *tempo_map_);
    b_in->process_block (time_info, *transport_, *tempo_map_);
    plugin_->process_block (time_info, *transport_, *tempo_map_);
    EXPECT_FLOAT_EQ (cv_out->buf_[128], expected) << "operation " << op_index;
  };

  process_and_expect (0, 1.f);   // Add (0.6 + 0.4 clamps to 1)
  process_and_expect (1, 0.2f);  // Subtract
  process_and_expect (2, 0.24f); // Multiply
  process_and_expect (3, 0.5f);  // Average
  process_and_expect (4, 0.4f);  // Minimum
  process_and_expect (5, 0.6f);  // Maximum
  process_and_expect (6, 0.4f);  // Invert A
}

TEST_F (ModulatorEnginesTest, MathUnconnectedInputReadsAsZero)
{
  load_engine (MathEngine::kDescriptorId);
  auto * a_in = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Input, MathEngine::kInputASymbol);
  auto * cv_out = find_port<dsp::CVPort> (
    *plugin_, dsp::PortFlow::Output, MathEngine::kOutputSymbol);
  ASSERT_NE (a_in, nullptr);
  ASSERT_NE (cv_out, nullptr);

  // Subtract with no B connected leaves A unchanged.
  drive_cv_input (*a_in, 0.6f);
  auto * operation =
    find_parameter (*plugin_, MathEngine::kOperationParameterId);
  ASSERT_NE (operation, nullptr);
  operation->setBaseValue (operation->range ().normalizedEnumValue (1));

  const auto time_info = block_info ();
  a_in->process_block (time_info, *transport_, *tempo_map_);
  plugin_->process_block (time_info, *transport_, *tempo_map_);
  EXPECT_FLOAT_EQ (cv_out->buf_[100], 0.6f);
}

// ---------------------------------------------------------------------------
// Serialization
// ---------------------------------------------------------------------------

TEST_F (ModulatorEnginesTest, BasePluginSerializationRestoresEngine)
{
  load_engine (LfoEngine::kDescriptorId);
  auto * rate = find_parameter (*plugin_, LfoEngine::kRateParameterId);
  ASSERT_NE (rate, nullptr);
  rate->setBaseValue (rate->range ().convertTo0To1 (5.f));

  nlohmann::json json;
  to_json (json, static_cast<const Plugin &> (*plugin_));

  auto restored = std::make_unique<Modulator> (*registry_);
  from_json (json, static_cast<Plugin &> (*restored));
  restored->prepare_for_processing (nullptr, kSampleRate, kBlockLength);

  auto * restored_rate = find_parameter (*restored, LfoEngine::kRateParameterId);
  ASSERT_NE (restored_rate, nullptr);
  EXPECT_FLOAT_EQ (
    restored_rate->baseValue (), rate->range ().convertTo0To1 (5.f));
  restored->release_resources ();
}

} // namespace zrythm::plugins::modulators
