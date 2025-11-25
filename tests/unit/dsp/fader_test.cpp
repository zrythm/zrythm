// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/fader.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"

#include <QObject>

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class FaderTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);

    // Create audio fader without hard limiting for most tests
    audio_fader_ = std::make_unique<Fader> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Audio, false, true, [] { return u8"Test Track"; },
      [] (bool solo_status) { return false; });

    // Create MIDI fader without hard limiting and non-automatable parameters
    midi_fader_ = std::make_unique<Fader> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Midi, false, false, [] { return u8"Test MIDI Track"; },
      [] (bool solo_status) { return false; });

    // Set up mock transport
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
  }

  void TearDown () override
  {
    if (audio_fader_)
      {
        audio_fader_->release_resources ();
      }
    if (midi_fader_)
      {
        midi_fader_->release_resources ();
      }
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  sample_rate_t                                    sample_rate_{ 48000 };
  nframes_t                                        max_block_length_{ 1024 };
  std::unique_ptr<Fader>                           audio_fader_;
  std::unique_ptr<Fader>                           midi_fader_;
  std::unique_ptr<graph_test::MockTransport>       mock_transport_;
};

TEST_F (FaderTest, ConstructionAndBasicProperties)
{
  // Test audio fader
  EXPECT_EQ (audio_fader_->is_audio (), true);
  EXPECT_EQ (audio_fader_->is_midi (), false);

  // Test MIDI fader
  EXPECT_EQ (midi_fader_->is_audio (), false);
  EXPECT_EQ (midi_fader_->is_midi (), true);

  // Test parameter accessors
  EXPECT_NE (audio_fader_->gain (), nullptr);
  EXPECT_NE (audio_fader_->balance (), nullptr);
  EXPECT_NE (audio_fader_->mute (), nullptr);
  EXPECT_NE (audio_fader_->solo (), nullptr);
  EXPECT_NE (audio_fader_->listen (), nullptr);
  EXPECT_NE (audio_fader_->monoToggle (), nullptr);
  EXPECT_NE (audio_fader_->swapPhaseToggle (), nullptr);

  // MIDI fader should not have mono/swap phase toggles
  EXPECT_EQ (midi_fader_->monoToggle (), nullptr);
  EXPECT_EQ (midi_fader_->swapPhaseToggle (), nullptr);

  // No hard limiting by default
  EXPECT_FALSE (audio_fader_->hard_limiting_enabled ());
  EXPECT_FALSE (midi_fader_->hard_limiting_enabled ());
}

TEST_F (FaderTest, PortConfiguration)
{
  // Test audio fader ports
  EXPECT_EQ (audio_fader_->get_input_ports ().size (), 1);
  EXPECT_EQ (audio_fader_->get_output_ports ().size (), 1);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  EXPECT_EQ (stereo_in.get_node_name (), u8"Test Track Fader/Fader input");
  EXPECT_EQ (stereo_out.get_node_name (), u8"Test Track Fader/Fader output");

  // Test MIDI fader ports
  EXPECT_EQ (midi_fader_->get_input_ports ().size (), 1);
  EXPECT_EQ (midi_fader_->get_output_ports ().size (), 1);

  auto &midi_in = midi_fader_->get_midi_in_port ();
  auto &midi_out = midi_fader_->get_midi_out_port ();

  EXPECT_EQ (
    midi_in.get_node_name (), u8"Test MIDI Track Fader/Ch MIDI Fader in");
  EXPECT_EQ (
    midi_out.get_node_name (), u8"Test MIDI Track Fader/Ch MIDI Fader out");
}

TEST_F (FaderTest, ParameterFunctionality)
{
  auto * gain_param = audio_fader_->gain ();
  auto * balance_param = audio_fader_->balance ();
  auto * mute_param = audio_fader_->mute ();
  auto * solo_param = audio_fader_->solo ();
  auto * listen_param = audio_fader_->listen ();

  // Test gain parameter range
  EXPECT_NEAR (gain_param->range ().minf_, 0.0f, 0.001f);
  EXPECT_NEAR (gain_param->range ().maxf_, 2.0f, 0.001f);
  EXPECT_NEAR (gain_param->range ().deff_, 1.0f, 0.001f);

  // Test balance parameter range
  EXPECT_NEAR (balance_param->range ().minf_, 0.0f, 0.001f);
  EXPECT_NEAR (balance_param->range ().maxf_, 1.0f, 0.001f);
  EXPECT_NEAR (balance_param->range ().deff_, 0.5f, 0.001f);

  // Test toggle parameters
  EXPECT_NEAR (mute_param->range ().minf_, 0.0f, 0.001f);
  EXPECT_NEAR (mute_param->range ().maxf_, 1.0f, 0.001f);
  EXPECT_NEAR (mute_param->range ().deff_, 0.0f, 0.001f);

  // Test parameter value setting
  gain_param->setBaseValue (0.5f);
  EXPECT_NEAR (gain_param->baseValue (), 0.5f, 0.001f);

  balance_param->setBaseValue (0.3f);
  EXPECT_NEAR (balance_param->baseValue (), 0.3f, 0.001f);

  // Test automatable flags
  EXPECT_TRUE (gain_param->automatable ());
  EXPECT_TRUE (balance_param->automatable ());
  EXPECT_TRUE (mute_param->automatable ());
  EXPECT_FALSE (solo_param->automatable ());
  EXPECT_FALSE (listen_param->automatable ());
}

TEST_F (FaderTest, PrepareAndReleaseResources)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  EXPECT_GE (stereo_in.buffers ()->getNumSamples (), max_block_length_);
  EXPECT_GE (stereo_out.buffers ()->getNumSamples (), max_block_length_);

  audio_fader_->release_resources ();

  EXPECT_EQ (stereo_in.buffers (), nullptr);
  EXPECT_EQ (stereo_out.buffers (), nullptr);
}

TEST_F (FaderTest, AudioProcessing)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Test unity gain (1.0) - account for smoothing
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Process multiple blocks to allow smoothing to reach target
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 2.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // After smoothing, should be close to target
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 2.0f, 0.05f);
    }

  // Test gain reduction (0.5) - account for smoothing
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (0.5f));

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 2.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.5f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 1.0f, 0.05f);
    }

  // Test boost (2.0) - account for smoothing (no hard limiting)
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (2.0f));

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 2.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 2.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 4.0f, 0.05f);
    }
}

TEST_F (FaderTest, MidiProcessing)
{
  midi_fader_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  auto &midi_in = midi_fader_->get_midi_in_port ();
  auto &midi_out = midi_fader_->get_midi_out_port ();

  // Add MIDI events to input
  midi_in.midi_events_.active_events_.add_note_on (1, 60, 100, 0);
  midi_in.midi_events_.active_events_.add_note_on (1, 64, 90, 10);
  midi_in.midi_events_.active_events_.add_note_off (1, 60, 100);

  // Set gain parameter
  midi_fader_->gain ()->setBaseValue (0.8f);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  midi_fader_->process_block (time_nfo, *mock_transport_);

  // Verify MIDI events are copied (fader doesn't modify MIDI events)
  EXPECT_EQ (midi_out.midi_events_.queued_events_.size (), 3);
}

TEST_F (FaderTest, MuteFunctionality)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Set gain to 1.0 and wait for smoothing
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Process multiple blocks to allow smoothing to reach target
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // After smoothing, should be close to target
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 1.0f, 0.05f);
    }

  // Test muted - account for smoothing
  audio_fader_->mute ()->setBaseValue (1.0f);

  // Process multiple blocks to allow smoothing to reach mute
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // After smoothing, should be close to 0
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.0f, 0.05f);
    }
}

TEST_F (FaderTest, BalanceFunctionality)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers with equal values
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Set gain to 1.0 and wait for smoothing
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Process multiple blocks to allow smoothing to reach target
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // Test center balance (0.5)
  audio_fader_->balance ()->setBaseValue (0.5f);

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 1.0f, 0.05f);
    }

  // Test hard left (0.0)
  audio_fader_->balance ()->setBaseValue (0.0f);

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.0f, 0.05f);
    }

  // Test hard right (1.0)
  audio_fader_->balance ()->setBaseValue (1.0f);

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 1.0f, 0.05f);
    }
}

TEST_F (FaderTest, SoloAndListenFunctionality)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Test solo functionality
  EXPECT_FALSE (audio_fader_->currently_soloed ());
  audio_fader_->solo ()->setBaseValue (1.0f);
  audio_fader_->process_block (time_nfo, *mock_transport_);
  EXPECT_TRUE (audio_fader_->currently_soloed ());

  // Test listen functionality
  EXPECT_FALSE (audio_fader_->currently_listened ());
  audio_fader_->listen ()->setBaseValue (1.0f);
  audio_fader_->process_block (time_nfo, *mock_transport_);
  EXPECT_TRUE (audio_fader_->currently_listened ());
}

TEST_F (FaderTest, MonoCompatibilityFunctionality)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers with different values
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Set gain to 1.0
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  // Test stereo mode (mono off)
  audio_fader_->monoToggle ()->setBaseValue (0.0f);
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 0.5f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 1e-5);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.5f, 1e-5);
    }

  // Test mono mode
  audio_fader_->monoToggle ()->setBaseValue (1.0f);

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 0.5f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      float expected_mono = (1.0f + 0.5f) * 0.5f;
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (0, i), expected_mono, 0.05f);
      EXPECT_NEAR (
        stereo_out.buffers ()->getSample (1, i), expected_mono, 0.05f);
    }
}

TEST_F (FaderTest, SwapPhaseFunctionality)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Set gain to 1.0
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  // Test normal phase
  audio_fader_->swapPhaseToggle ()->setBaseValue (0.0f);
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 1e-5);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 1.0f, 1e-5);
    }

  // Test phase swap
  audio_fader_->swapPhaseToggle ()->setBaseValue (1.0f);

  // Process multiple blocks to allow smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), -1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), -1.0f, 0.05f);
    }
}

TEST_F (FaderTest, MidiModeFunctionality)
{
  // Test default MIDI mode
  EXPECT_EQ (
    midi_fader_->midiMode (),
    Fader::MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER);

  // Test setting MIDI mode
  midi_fader_->setMidiMode (Fader::MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME);
  EXPECT_EQ (
    midi_fader_->midiMode (), Fader::MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME);

  // Test signal emission
  bool signal_emitted = false;
  QObject::connect (
    midi_fader_.get (), &Fader::midiModeChanged, midi_fader_.get (),
    [&signal_emitted] (Fader::MidiFaderMode mode) {
      signal_emitted = true;
      EXPECT_EQ (mode, Fader::MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER);
    });

  midi_fader_->setMidiMode (
    Fader::MidiFaderMode::MIDI_FADER_MODE_VEL_MULTIPLIER);
  EXPECT_TRUE (signal_emitted);
}

TEST_F (FaderTest, JsonSerializationRoundtrip)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  // Set some parameter values
  audio_fader_->gain ()->setBaseValue (0.75f);
  audio_fader_->balance ()->setBaseValue (0.3f);
  audio_fader_->mute ()->setBaseValue (1.0f);
  audio_fader_->solo ()->setBaseValue (1.0f);
  audio_fader_->listen ()->setBaseValue (1.0f);
  audio_fader_->monoToggle ()->setBaseValue (1.0f);
  audio_fader_->swapPhaseToggle ()->setBaseValue (1.0f);
  audio_fader_->setMidiMode (Fader::MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME);

  // Serialize
  nlohmann::json j = *audio_fader_;

  // Create new fader from JSON
  Fader deserialized (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, true, true, [] { return u8"Test Track"; },
    [] (bool solo_status) { return false; });

  from_json (j, deserialized);

  // Reinitialize after deserialization
  deserialized.prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Verify parameters
  EXPECT_NEAR (deserialized.gain ()->baseValue (), 0.75f, 0.001f);
  EXPECT_NEAR (deserialized.balance ()->baseValue (), 0.3f, 0.001f);
  EXPECT_NEAR (deserialized.mute ()->baseValue (), 1.0f, 0.001f);
  EXPECT_NEAR (deserialized.solo ()->baseValue (), 1.0f, 0.001f);
  EXPECT_NEAR (deserialized.listen ()->baseValue (), 1.0f, 0.001f);
  EXPECT_NEAR (deserialized.monoToggle ()->baseValue (), 1.0f, 0.001f);
  EXPECT_NEAR (deserialized.swapPhaseToggle ()->baseValue (), 1.0f, 0.001f);
  EXPECT_EQ (
    deserialized.midiMode (), Fader::MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME);
  EXPECT_EQ (
    deserialized.gain ()->get_uuid (), audio_fader_->gain ()->get_uuid ());
}

TEST_F (FaderTest, JsonSerializationMidiRoundtrip)
{
  midi_fader_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Set some parameter values
  midi_fader_->gain ()->setBaseValue (0.7f);
  midi_fader_->balance ()->setBaseValue (0.8f);
  midi_fader_->mute ()->setBaseValue (1.0f);
  midi_fader_->solo ()->setBaseValue (1.0f);
  midi_fader_->listen ()->setBaseValue (1.0f);
  midi_fader_->setMidiMode (Fader::MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME);

  // Serialize
  nlohmann::json j = *midi_fader_;

  // Create new fader from JSON
  Fader deserialized (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Midi, false, false, [] { return u8"Test MIDI Track"; },
    [] (bool solo_status) { return false; });

  from_json (j, deserialized);

  // Reinitialize after deserialization
  deserialized.prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Verify parameters
  EXPECT_NEAR (deserialized.gain ()->baseValue (), 0.7f, 0.001f);
  EXPECT_NEAR (deserialized.balance ()->baseValue (), 0.8f, 0.001f);
  EXPECT_NEAR (deserialized.mute ()->baseValue (), 1.0f, 0.001f);
  EXPECT_NEAR (deserialized.solo ()->baseValue (), 1.0f, 0.001f);
  EXPECT_NEAR (deserialized.listen ()->baseValue (), 1.0f, 0.001f);
  EXPECT_EQ (deserialized.monoToggle (), nullptr);
  EXPECT_EQ (deserialized.swapPhaseToggle (), nullptr);
  EXPECT_EQ (
    deserialized.midiMode (), Fader::MidiFaderMode::MIDI_FADER_MODE_CC_VOLUME);
}
TEST_F (FaderTest, ShouldBeMutedCallback)
{
  bool should_mute = false;
  auto fader_with_callback = std::make_unique<Fader> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, true, true, [] { return u8"Test Track"; },
    [&should_mute] (bool solo_status) { return should_mute; });

  fader_with_callback->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = fader_with_callback->get_stereo_in_port ();
  auto &stereo_out = fader_with_callback->get_stereo_out_port ();

  // Fill input buffers
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  fader_with_callback->gain ()->setBaseValue (
    fader_with_callback->gain ()->range ().convertTo0To1 (1.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Test no mute callback - wait for smoothing
  should_mute = false;
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      fader_with_callback->process_block (time_nfo, *mock_transport_);
    }
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 1.0f, 0.05f);
    }

  // Test mute callback - wait for smoothing
  should_mute = true;
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      fader_with_callback->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.0f, 0.05f);
    }
}

TEST_F (FaderTest, PreProcessAudioCallback)
{
  bool callback_called = false;
  auto pre_process_cb =
    [&callback_called] (
      std::pair<std::span<float>, std::span<float>> stereo_bufs,
      const EngineProcessTimeInfo                  &time_nfo) {
      callback_called = true;
      // Modify the buffers
      for (auto &sample : stereo_bufs.first)
        {
          sample *= 2.0f;
        }
      for (auto &sample : stereo_bufs.second)
        {
          sample *= 0.5f;
        }
    };

  audio_fader_->set_preprocess_audio_callback (pre_process_cb);
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  EXPECT_TRUE (callback_called);
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 2.0f, 1e-5);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.5f, 1e-5);
    }
}

TEST_F (FaderTest, MuteGainCallback)
{
  bool mute_gain_called = false;
  auto mute_gain_cb = [&mute_gain_called] () {
    mute_gain_called = true;
    return 0.25f;
  };

  audio_fader_->set_mute_gain_callback (mute_gain_cb);
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Test normal operation (no mute) - wait for smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }
  EXPECT_FALSE (mute_gain_called);

  // Test mute with custom gain - wait for smoothing
  audio_fader_->mute ()->setBaseValue (1.0f);
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }
  EXPECT_TRUE (mute_gain_called);

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.25f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.25f, 0.05f);
    }
}

TEST_F (FaderTest, EdgeCases)
{
  // Test with zero frames
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 0
  };
  EXPECT_NO_THROW (audio_fader_->process_block (time_nfo, *mock_transport_));

  // Test with maximum gain
  audio_fader_->gain ()->setBaseValue (1.0f); // 0-1 range for parameter
  EXPECT_NO_THROW (audio_fader_->process_block (time_nfo, *mock_transport_));

  // Test with zero gain (silence)
  audio_fader_->gain ()->setBaseValue (0.0f);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 10; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  time_nfo.nframes_ = 10;
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 10; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.0f, 1e-6);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.0f, 1e-6);
    }

  // Test exception for non-audio fader trying to get stereo ports
  EXPECT_THROW (midi_fader_->get_stereo_in_port (), std::runtime_error);
  EXPECT_THROW (midi_fader_->get_stereo_out_port (), std::runtime_error);
}

TEST_F (FaderTest, DbStringGetter)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 0
  };

  // Test default value (0 dB)
  audio_fader_->process_block (time_nfo, *mock_transport_);
  std::string db_str = audio_fader_->db_string_getter ();
  EXPECT_TRUE (db_str.find ("0.0") != std::string::npos);

  // Test -6 dB
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (0.5f));
  audio_fader_->process_block (time_nfo, *mock_transport_);
  db_str = audio_fader_->db_string_getter ();
  EXPECT_TRUE (db_str.find ("-6.0") != std::string::npos);

  // Test +6 dB
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (2.0f));
  audio_fader_->process_block (time_nfo, *mock_transport_);
  db_str = audio_fader_->db_string_getter ();
  EXPECT_TRUE (db_str.find ("6.0") != std::string::npos);
}

TEST_F (FaderTest, HardLimitingFunctionality)
{
  // Create fader with hard limiting enabled
  auto hard_limit_fader = std::make_unique<Fader> (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, true, true, [] { return u8"Test Track Hard Limit"; },
    [] (bool solo_status) { return false; });
  EXPECT_TRUE (hard_limit_fader->hard_limiting_enabled ());

  hard_limit_fader->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = hard_limit_fader->get_stereo_in_port ();
  auto &stereo_out = hard_limit_fader->get_stereo_out_port ();

  // Fill input buffers with values that would exceed the limits
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Set high gain to amplify beyond limits
  hard_limit_fader->gain ()->setBaseValue (
    hard_limit_fader->gain ()->range ().convertTo0To1 (3.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Process multiple blocks to allow smoothing to reach target
  for (int block = 0; block < 15; block++)
    {
      fill_inputs (5.f, -5.f);
      hard_limit_fader->process_block (time_nfo, *mock_transport_);
    }

  // Verify hard limiting is applied
  for (int i = 0; i < 512; i++)
    {
      EXPECT_LE (stereo_out.buffers ()->getSample (0, i), 2.0f);
      EXPECT_GE (stereo_out.buffers ()->getSample (0, i), -2.0f);
      EXPECT_LE (stereo_out.buffers ()->getSample (1, i), 2.0f);
      EXPECT_GE (stereo_out.buffers ()->getSample (1, i), -2.0f);
    }

  // Test that values within limits are not affected

  hard_limit_fader->gain ()->setBaseValue (
    hard_limit_fader->gain ()->range ().convertTo0To1 (1.0f));

  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, -1.f);
      hard_limit_fader->process_block (time_nfo, *mock_transport_);
    }

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), -1.0f, 0.05f);
    }
}

TEST_F (FaderTest, GainSmoothing)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Start with 0 gain
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (0.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // Process until we reach silence
  for (int block = 0; block < 10; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // Verify we're at 0
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.0f, 0.01f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.0f, 0.01f);
    }

  // Now set to 1.0 and test gradual increase
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  // Test that gain increases gradually over multiple blocks
  std::vector<float> first_block_values;
  fill_inputs (1.f, 1.f);
  audio_fader_->process_block (time_nfo, *mock_transport_);
  for (int i = 0; i < 512; i++)
    {
      first_block_values.push_back (stereo_out.buffers ()->getSample (0, i));
    }

  // After first block, should be between 0 and 1 due to smoothing
  EXPECT_GT (first_block_values[0], 0.0f);
  EXPECT_LT (first_block_values[0], 1.0f);

  // Process more blocks until we reach close to target
  for (int block = 0; block < 20; block++)
    {
      fill_inputs (1.f, 1.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // After sufficient blocks, should be close to target
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 1.0f, 0.05f);
    }

  // Test rapid changes
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (0.5f));

  // Should not immediately jump to new values
  std::vector<float> prev_values;
  for (int i = 0; i < 512; i++)
    {
      prev_values.push_back (stereo_out.buffers ()->getSample (0, i));
    }
  fill_inputs (1.f, 1.f);
  audio_fader_->process_block (time_nfo, *mock_transport_);
  EXPECT_FLOAT_EQ (stereo_out.buffers ()->getSample (0, 0), prev_values[0]);
  for (int i = 1; i < 10; ++i)
    {
      EXPECT_NE (stereo_out.buffers ()->getSample (0, i), 0.5f);
      EXPECT_LT (stereo_out.buffers ()->getSample (0, i), prev_values[i]);
    }
}

TEST_F (FaderTest, InputBufferClearedBetweenProcessCalls)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Set gain to 1.0 for direct pass-through
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  const auto fill_input_bufs = [&] (float left_val, float right_val) {
    // Fill with test data
    for (int i = 0; i < 512; i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Process and wait for smoothing
  for (int block = 0; block < 10; block++)
    {
      fill_input_bufs (1.f, 2.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // Verify first cycle processed correctly
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 2.0f, 0.05f);
    }

  // Process second cycle
  for (int block = 0; block < 10; block++)
    {
      fill_input_bufs (3.f, 4.f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // Verify second cycle processed correctly
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 3.0f, 0.05f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 4.0f, 0.05f);
    }

  // Third processing cycle - verify no accumulation from previous cycles
  // Fill with zeroes to test clearing behavior

  // Process third cycle
  for (int block = 0; block < 10; block++)
    {
      fill_input_bufs (0.1f, 0.1f);
      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // Verify third cycle processed correctly (should be silent)
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.1f, 1e-6f);
      EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.1f, 1e-6f);
    }

  // Verify input buffers are cleared after processing
  // This tests the buffer clearing behavior between process calls
  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (stereo_in.buffers ()->getSample (0, i), 0.0f, 1e-6f)
        << "Input buffer not cleared at left channel index " << i;
      EXPECT_NEAR (stereo_in.buffers ()->getSample (1, i), 0.0f, 1e-6f)
        << "Input buffer not cleared at right channel index " << i;
    }
}

TEST_F (FaderTest, BufferOverflowWithNonZeroOffset)
{
  audio_fader_->prepare_for_processing (
    nullptr, sample_rate_, max_block_length_);

  auto &stereo_in = audio_fader_->get_stereo_in_port ();
  auto &stereo_out = audio_fader_->get_stereo_out_port ();

  // Fill input buffers with known values
  const auto fill_inputs = [&] (float left_val, float right_val) {
    for (int i = 0; i < static_cast<int> (max_block_length_); i++)
      {
        stereo_in.buffers ()->setSample (0, i, left_val);
        stereo_in.buffers ()->setSample (1, i, right_val);
      }
  };

  // Set gain to 1.0 for direct pass-through
  audio_fader_->gain ()->setBaseValue (
    audio_fader_->gain ()->range ().convertTo0To1 (1.0f));

  // Test with non-zero offset that would cause buffer overflow
  const nframes_t offset = 256;  // Non-zero offset
  const nframes_t nframes = 256; // Process half buffer

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = offset,
    .local_offset_ = offset,
    .nframes_ = nframes
  };

  // Clear output buffer first
  for (int i = 0; i < static_cast<int> (max_block_length_); i++)
    {
      stereo_out.buffers ()->setSample (0, i, 0.0f);
      stereo_out.buffers ()->setSample (1, i, 0.0f);
    }

  // Process multiple blocks to allow smoothing to reach target
  for (int block = 0; block < 10; block++)
    {
      // Fill entire input buffer with test pattern
      fill_inputs (1.0f, 2.0f);

      audio_fader_->process_block (time_nfo, *mock_transport_);
    }

  // Verify that only the intended range (offset to offset + nframes) is
  // processed and no buffer overflow occurred
  for (int i = 0; i < static_cast<int> (max_block_length_); i++)
    {
      if (
        i >= static_cast<int> (offset)
        && i < static_cast<int> (offset + nframes))
        {
          // This range should be processed
          EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 1.0f, 0.05f)
            << "Left channel not processed correctly at index " << i;
          EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 2.0f, 0.05f)
            << "Right channel not processed correctly at index " << i;
        }
      else
        {
          // This range should remain 0 (no processing should occur here)
          EXPECT_NEAR (stereo_out.buffers ()->getSample (0, i), 0.0f, 1e-6f)
            << "Left channel unexpectedly modified at index " << i
            << " (possible buffer overflow)";
          EXPECT_NEAR (stereo_out.buffers ()->getSample (1, i), 0.0f, 1e-6f)
            << "Right channel unexpectedly modified at index " << i
            << " (possible buffer overflow)";
        }
    }
}

} // namespace zrythm::structure::tracks
