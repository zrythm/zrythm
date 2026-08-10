// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "plugins/faust/faust_plugin.h"
#include "plugins/faust/faust_registry.h"
#include "plugins/plugin_configuration.h"
#include "utils/audio.h"
#include "utils/object_registry.h"

#include "helpers/scoped_qcoreapplication.h"

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::plugins
{

class FaustPluginTest
    : public ::testing::Test,
      public test_helpers::ScopedQCoreApplication
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

  void load_faust_plugin (const utils::Utf8String &key)
  {
    const auto * info = faust::find_faust_plugin_by_key (key);
    ASSERT_NE (info, nullptr);

    auto config = std::make_unique<PluginConfiguration> ();
    config->descr_ = faust::make_faust_plugin_descriptor (*info);
    ASSERT_NE (config->descr_, nullptr);

    plugin_ = std::make_unique<FaustPlugin> (*registry_);
    // Instantiation is synchronous
    plugin_->set_configuration (*config);
    plugin_->prepare_for_processing (
      nullptr, units::sample_rate (48000), units::samples (256));
  }

  dsp::ProcessorParameter * find_param_by_label (const char8_t * label)
  {
    for (const auto &param_ref : plugin_->get_parameters ())
      {
        auto * param = param_ref.get_object_as<dsp::ProcessorParameter> ();
        if (param->label () == label)
          return param;
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

  dsp::AudioPort * get_audio_out ()
  {
    for (const auto &port_ref : plugin_->get_output_ports ())
      {
        if (auto * port = port_ref.get_object_as<dsp::AudioPort> ())
          return port;
      }
    return nullptr;
  }

  dsp::MidiPort * get_midi_in ()
  {
    for (const auto &port_ref : plugin_->get_input_ports ())
      {
        if (auto * port = port_ref.get_object_as<dsp::MidiPort> ())
          return port;
      }
    return nullptr;
  }

  static float output_peak (const dsp::AudioPort &port, int num_samples)
  {
    float peak = 0.f;
    for (int ch = 0; ch < port.buffers ()->getNumChannels (); ++ch)
      {
        peak =
          std::max (peak, port.buffers ()->getMagnitude (ch, 0, num_samples));
      }
    return peak;
  }

  std::unique_ptr<utils::ObjectRegistry>          registry_;
  std::unique_ptr<dsp::graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>                  tempo_map_;
  std::unique_ptr<FaustPlugin>                    plugin_;
};

// Parameters are created from the Faust UI metadata (label, unit, scale,
// min/max/init)
TEST_F (FaustPluginTest, ParamsCreatedFromUiMetadata)
{
  load_faust_plugin (u8"zrythm.faust.lowpass_filter");

  // hslider("Frequency [unit:Hz] [scale:log]", 5000, 10, 18000, 1)
  auto * freq_param = find_param_by_label (u8"Frequency");
  ASSERT_NE (freq_param, nullptr);
  EXPECT_EQ (freq_param->range ().type_, dsp::ParameterRange::Type::Logarithmic);
  EXPECT_EQ (freq_param->range ().unit_, dsp::ParameterRange::Unit::Hz);
  EXPECT_FLOAT_EQ (freq_param->range ().minf_, 10.f);
  EXPECT_FLOAT_EQ (freq_param->range ().maxf_, 18000.f);
  EXPECT_NEAR (
    freq_param->baseValue (), freq_param->range ().convertTo0To1 (5000.f),
    0.001f);
}

// Ports are created according to the dsp's input/output counts and whether
// the plugin is an instrument
TEST_F (FaustPluginTest, PortsMatchDspInsOuts)
{
  // Stereo FX (2 ins, 2 outs)
  load_faust_plugin (u8"zrythm.faust.lowpass_filter");
  ASSERT_NE (get_audio_out (), nullptr);
  EXPECT_EQ (get_audio_out ()->num_channels (), 2u);
  EXPECT_EQ (get_midi_in (), nullptr);
  int audio_ins = 0;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      if (port_ref.get_object_as<dsp::AudioPort> () != nullptr)
        ++audio_ins;
    }
  EXPECT_EQ (audio_ins, 1);

  // Generator (0 ins, 2 outs)
  load_faust_plugin (u8"zrythm.faust.white_noise");
  audio_ins = 0;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      if (port_ref.get_object_as<dsp::AudioPort> () != nullptr)
        ++audio_ins;
    }
  EXPECT_EQ (audio_ins, 0);
  ASSERT_NE (get_audio_out (), nullptr);
  EXPECT_EQ (get_audio_out ()->num_channels (), 2u);

  // Instrument (MIDI in, no audio in, 2 outs)
  load_faust_plugin (u8"zrythm.faust.triple_synth");
  EXPECT_NE (get_midi_in (), nullptr);
  audio_ins = 0;
  for (const auto &port_ref : plugin_->get_input_ports ())
    {
      if (port_ref.get_object_as<dsp::AudioPort> () != nullptr)
        ++audio_ins;
    }
  EXPECT_EQ (audio_ins, 0);
  ASSERT_NE (get_audio_out (), nullptr);
  EXPECT_EQ (get_audio_out ()->num_channels (), 2u);
}

// An FX/generator plugin produces non-silent output
TEST_F (FaustPluginTest, FxProcessingProducesOutput)
{
  load_faust_plugin (u8"zrythm.faust.white_noise");
  process_blocks (5);

  auto * audio_out = get_audio_out ();
  ASSERT_NE (audio_out, nullptr);
  EXPECT_GT (output_peak (*audio_out, 256), 0.001f)
    << "White noise generator produced silent output";
}

// A note-on to an instrument produces non-silent output
TEST_F (FaustPluginTest, InstrumentNoteOnProducesAudio)
{
  load_faust_plugin (u8"zrythm.faust.triple_synth");

  auto * midi_in = get_midi_in ();
  ASSERT_NE (midi_in, nullptr);
  const auto note_on =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0u));
  midi_in->buffer_.push_back (note_on.time_, note_on.data ());

  process_blocks (10);

  auto * audio_out = get_audio_out ();
  ASSERT_NE (audio_out, nullptr);
  EXPECT_GT (output_peak (*audio_out, 256), 0.001f)
    << "Instrument produced silent output after note-on";
}

// Parameter changes propagate to the Faust control zones and affect
// processing
TEST_F (FaustPluginTest, ParamChangeAffectsProcessing)
{
  load_faust_plugin (u8"zrythm.faust.white_noise");

  auto * amp_param = find_param_by_label (u8"Amp");
  ASSERT_NE (amp_param, nullptr);
  auto * audio_out = get_audio_out ();
  ASSERT_NE (audio_out, nullptr);

  // hslider("Amp [unit:dB]", -10, -70, 10, 0.1)
  // Note: si.smoo smooths in the linear domain, so the -70 dB tail needs
  // ~10k samples to settle - use enough blocks for both phases
  amp_param->setBaseValue (amp_param->range ().convertTo0To1 (10.f));
  process_blocks (100);
  const float loud_peak = output_peak (*audio_out, 256);

  amp_param->setBaseValue (amp_param->range ().convertTo0To1 (-70.f));
  process_blocks (100);
  const float quiet_peak = output_peak (*audio_out, 256);

  EXPECT_GT (loud_peak, quiet_peak * 100.f)
    << "Amp parameter change did not affect output (loud=" << loud_peak
    << " quiet=" << quiet_peak << ")";
}

// Parameter values survive a JSON round trip (internal plugins persist
// state via the parameter registry, not a plugin state blob)
TEST_F (FaustPluginTest, ParamStateRoundTripsThroughJson)
{
  load_faust_plugin (u8"zrythm.faust.white_noise");

  auto * amp_param = find_param_by_label (u8"Amp");
  ASSERT_NE (amp_param, nullptr);

  amp_param->setBaseValue (0.25f);
  const nlohmann::json param_json = *amp_param;

  amp_param->setBaseValue (0.8f);
  param_json.get_to (*amp_param);

  EXPECT_FLOAT_EQ (amp_param->baseValue (), 0.25f);
}

// After full plugin deserialization (the path a loaded project takes),
// parameter changes must still propagate to the Faust DSP zones, and the
// deserialized values must be pushed to the zones on setup.
TEST_F (FaustPluginTest, ParamMappingSurvivesDeserialization)
{
  load_faust_plugin (u8"zrythm.faust.white_noise");

  auto * amp_param = find_param_by_label (u8"Amp");
  ASSERT_NE (amp_param, nullptr);
  const float loud_value = amp_param->range ().convertTo0To1 (10.f);
  amp_param->setBaseValue (loud_value);

  // Capture the specs the deserializer would restore (unique ID, range,
  // label, value).
  const auto amp_unique_id = amp_param->get_unique_id ();
  const auto amp_range = amp_param->range ();
  const auto amp_label = utils::Utf8String{ u8"Amp" };

  // Build a second plugin in the post-deserialization state: ports and
  // params already present (with restored values) when set_configuration()
  // runs, so the handler takes the generateNewPluginPortsAndParams=false
  // path — the same state a loaded project produces.
  auto registry2 = std::make_unique<utils::ObjectRegistry> ();
  auto plugin2 = std::make_unique<FaustPlugin> (*registry2);

  // White noise: 0 audio ins, 1 stereo audio out.
  auto out_port_ref = utils::create_object<dsp::AudioPort> (
    *registry2, u8"audio_out", dsp::PortFlow::Output,
    dsp::SpeakerArrangement::stereo ());
  plugin2->add_output_port (out_port_ref);

  auto amp_ref = utils::create_object<dsp::ProcessorParameter> (
    *registry2, *registry2, amp_unique_id, amp_range, amp_label);
  auto * amp2 = amp_ref.get_object_as<dsp::ProcessorParameter> ();
  amp2->setBaseValue (loud_value);
  plugin2->add_parameter (amp_ref);

  const auto * info =
    faust::find_faust_plugin_by_key (u8"zrythm.faust.white_noise");
  ASSERT_NE (info, nullptr);
  auto config = std::make_unique<PluginConfiguration> ();
  config->descr_ = faust::make_faust_plugin_descriptor (*info);
  ASSERT_NE (config->descr_, nullptr);
  plugin2->set_configuration (*config);
  plugin2->prepare_for_processing (
    nullptr, units::sample_rate (48000), units::samples (256));

  auto * out_port = out_port_ref.get_object_as<dsp::AudioPort> ();
  ASSERT_NE (out_port, nullptr);

  const dsp::graph::ProcessBlockInfo block{
    .transport_position_ = units::samples (0),
    .buffer_offset_ = units::samples (0),
    .nframes_ = units::samples (256),
  };

  // The deserialized 10 dB value must have been pushed to the Faust zone on
  // setup — loud output without touching the param.
  for (int i = 0; i < 100; ++i)
    plugin2->process_block (block, *mock_transport_, *tempo_map_);
  const float loud_peak = output_peak (*out_port, 256);

  // Changing the param must still propagate to the zone.
  amp2->setBaseValue (amp2->range ().convertTo0To1 (-70.f));
  for (int i = 0; i < 100; ++i)
    plugin2->process_block (block, *mock_transport_, *tempo_map_);
  const float quiet_peak = output_peak (*out_port, 256);

  EXPECT_GT (loud_peak, quiet_peak * 100.f)
    << "Deserialized param mapping broken (loud=" << loud_peak
    << " quiet=" << quiet_peak << ")";

  plugin2->release_resources ();
}

TEST_F (FaustPluginTest, HasNativeUiIsFalse)
{
  load_faust_plugin (u8"zrythm.faust.lowpass_filter");
  EXPECT_FALSE (plugin_->hasNativeUi ());
}

} // namespace zrythm::plugins
