// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/graph.h"
#include "dsp/graph_builder.h"
#include "dsp/parameter.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"
#include "structure/tracks/channel_send.h"

#include <QObject>

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class ChannelSendTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<dsp::PortRegistry> ();
    param_registry_ =
      std::make_unique<dsp::ProcessorParameterRegistry> (*port_registry_);

    audio_send_ = std::make_unique<ChannelSend> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Audio, 0, true);

    midi_send_ = std::make_unique<ChannelSend> (
      dsp::ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
      dsp::PortType::Event, 1, false);
  }

  void TearDown () override
  {
    if (audio_send_)
      {
        audio_send_->release_resources ();
      }
    if (midi_send_)
      {
        midi_send_->release_resources ();
      }
  }

  std::unique_ptr<dsp::PortRegistry>               port_registry_;
  std::unique_ptr<dsp::ProcessorParameterRegistry> param_registry_;
  sample_rate_t                                    sample_rate_{ 48000 };
  nframes_t                                        max_block_length_{ 1024 };
  std::unique_ptr<ChannelSend>                     audio_send_;
  std::unique_ptr<ChannelSend>                     midi_send_;
};

TEST_F (ChannelSendTest, ConstructionAndBasicProperties)
{
  // Test audio send
  EXPECT_EQ (audio_send_->is_audio (), true);
  EXPECT_EQ (audio_send_->is_midi (), false);
  EXPECT_EQ (audio_send_->is_prefader (), true);
  EXPECT_NE (audio_send_->amountParam (), nullptr);

  // Test MIDI send
  EXPECT_EQ (midi_send_->is_audio (), false);
  EXPECT_EQ (midi_send_->is_midi (), true);
  EXPECT_EQ (midi_send_->is_prefader (), false);
  EXPECT_NE (midi_send_->amountParam (), nullptr);
}

TEST_F (ChannelSendTest, PortConfiguration)
{
  // Test audio send ports
  EXPECT_EQ (audio_send_->get_input_ports ().size (), 2);
  EXPECT_EQ (audio_send_->get_output_ports ().size (), 2);

  auto [left_in, right_in] = audio_send_->get_stereo_in_ports ();
  auto [left_out, right_out] = audio_send_->get_stereo_out_ports ();

  EXPECT_EQ (left_in.get_node_name (), u8"Channel Send 1/Audio input L");
  EXPECT_EQ (right_in.get_node_name (), u8"Channel Send 1/Audio input R");
  EXPECT_EQ (left_out.get_node_name (), u8"Channel Send 1/Audio output L");
  EXPECT_EQ (right_out.get_node_name (), u8"Channel Send 1/Audio output R");

  // Test MIDI send ports
  EXPECT_EQ (midi_send_->get_input_ports ().size (), 1);
  EXPECT_EQ (midi_send_->get_output_ports ().size (), 1);

  auto &midi_in = midi_send_->get_midi_in_port ();
  auto &midi_out = midi_send_->get_midi_out_port ();

  EXPECT_EQ (midi_in.get_node_name (), u8"Channel Send 2/MIDI input");
  EXPECT_EQ (midi_out.get_node_name (), u8"Channel Send 2/MIDI output");
}

TEST_F (ChannelSendTest, ParameterFunctionality)
{
  auto * amount_param = audio_send_->amountParam ();
  EXPECT_NE (amount_param, nullptr);

  // Test parameter range
  EXPECT_NEAR (amount_param->range ().minf_, 0.0f, 0.001f);
  EXPECT_NEAR (amount_param->range ().maxf_, 2.0f, 0.001f);
  EXPECT_NEAR (amount_param->range ().deff_, 1.0f, 0.001f);

  // Test parameter value setting
  amount_param->setBaseValue (0.5f);
  EXPECT_NEAR (amount_param->baseValue (), 0.5f, 0.001f);

  // Test clamping
  amount_param->setBaseValue (1.5f);
  EXPECT_NEAR (amount_param->baseValue (), 1.f, 0.001f);
}

TEST_F (ChannelSendTest, PrepareAndReleaseResources)
{
  audio_send_->prepare_for_processing (sample_rate_, max_block_length_);

  auto [left_in, right_in] = audio_send_->get_stereo_in_ports ();
  auto [left_out, right_out] = audio_send_->get_stereo_out_ports ();

  EXPECT_GE (left_in.buf_.size (), max_block_length_);
  EXPECT_GE (right_in.buf_.size (), max_block_length_);
  EXPECT_GE (left_out.buf_.size (), max_block_length_);
  EXPECT_GE (right_out.buf_.size (), max_block_length_);

  audio_send_->release_resources ();

  EXPECT_EQ (left_in.buf_.size (), 0);
  EXPECT_EQ (right_in.buf_.size (), 0);
  EXPECT_EQ (left_out.buf_.size (), 0);
  EXPECT_EQ (right_out.buf_.size (), 0);
}

TEST_F (ChannelSendTest, AudioProcessing)
{
  audio_send_->prepare_for_processing (sample_rate_, max_block_length_);

  auto [left_in, right_in] = audio_send_->get_stereo_in_ports ();
  auto [left_out, right_out] = audio_send_->get_stereo_out_ports ();

  // Fill input buffers
  for (int i = 0; i < 512; i++)
    {
      left_in.buf_[i] = 1.0f;
      right_in.buf_[i] = 2.0f;
    }

  // Test passthrough (gain = 1.0)
  audio_send_->amountParam ()->setBaseValue (
    audio_send_->amountParam ()->range ().convertTo0To1 (1.0f));
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  audio_send_->process_block (time_nfo);

  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (left_out.buf_[i], 1.0f);
      EXPECT_FLOAT_EQ (right_out.buf_[i], 2.0f);
    }

  // Test gain reduction (gain = 0.5)
  audio_send_->amountParam ()->setBaseValue (
    audio_send_->amountParam ()->range ().convertTo0To1 (0.5f));
  audio_send_->process_block (time_nfo);

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (left_out.buf_[i], 0.5f, 1e-5);
      EXPECT_NEAR (right_out.buf_[i], 1.0f, 1e-5);
    }

  // Test boost (gain = 2.0)
  audio_send_->amountParam ()->setBaseValue (
    audio_send_->amountParam ()->range ().convertTo0To1 (2.0f));
  audio_send_->process_block (time_nfo);

  for (int i = 0; i < 512; i++)
    {
      EXPECT_NEAR (left_out.buf_[i], 2.0f, 1e-5);
      EXPECT_NEAR (right_out.buf_[i], 4.0f, 1e-5);
    }
}

TEST_F (ChannelSendTest, MidiProcessing)
{
  midi_send_->prepare_for_processing (sample_rate_, max_block_length_);

  auto &midi_in = midi_send_->get_midi_in_port ();
  auto &midi_out = midi_send_->get_midi_out_port ();

  // Add MIDI events to input
  midi_in.midi_events_.active_events_.add_note_on (1, 60, 100, 0);
  midi_in.midi_events_.active_events_.add_note_on (1, 64, 90, 10);
  midi_in.midi_events_.active_events_.add_note_off (1, 60, 100);

  // Set amount parameter
  midi_send_->amountParam ()->setBaseValue (0.8f);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  midi_send_->process_block (time_nfo);

  // Verify MIDI events are copied
  EXPECT_EQ (midi_out.midi_events_.queued_events_.size (), 3);
}

TEST_F (ChannelSendTest, JsonSerializationRoundtrip)
{
  audio_send_->prepare_for_processing (sample_rate_, max_block_length_);

  // Set some parameter values
  audio_send_->amountParam ()->setBaseValue (0.75f);

  // Serialize
  nlohmann::json j = *audio_send_;

  // Create new send from JSON
  ChannelSend deserialized (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, 2, false);

  from_json (j, deserialized);

  // Reinitialize after deserialization
  deserialized.prepare_for_processing (sample_rate_, max_block_length_);

  // Verify ports & params
  EXPECT_EQ (
    audio_send_->get_stereo_in_ports ().first.get_uuid (),
    deserialized.get_stereo_in_ports ().first.get_uuid ());
  EXPECT_EQ (
    audio_send_->amountParam ()->get_uuid (),
    deserialized.amountParam ()->get_uuid ());

  // Verify properties
  EXPECT_EQ (deserialized.is_audio (), true);
  EXPECT_EQ (deserialized.is_midi (), false);
  EXPECT_EQ (deserialized.is_prefader (), true);
  EXPECT_NEAR (deserialized.amountParam ()->baseValue (), 0.75f, 0.001f);
}

TEST_F (ChannelSendTest, JsonSerializationMidiRoundtrip)
{
  midi_send_->prepare_for_processing (sample_rate_, max_block_length_);

  // Set some parameter values
  midi_send_->amountParam ()->setBaseValue (0.7f);

  // Serialize
  nlohmann::json j = *midi_send_;

  // Create new send from JSON
  ChannelSend deserialized (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Event, 3, true);

  from_json (j, deserialized);

  // Reinitialize after deserialization
  deserialized.prepare_for_processing (sample_rate_, max_block_length_);

  // Verify properties
  EXPECT_EQ (
    midi_send_->get_midi_out_port ().get_uuid (),
    deserialized.get_midi_out_port ().get_uuid ());
  EXPECT_EQ (deserialized.is_audio (), false);
  EXPECT_EQ (deserialized.is_midi (), true);
  EXPECT_EQ (deserialized.is_prefader (), false);
  EXPECT_NEAR (deserialized.amountParam ()->baseValue (), 0.7f, 0.001f);
}

TEST_F (ChannelSendTest, DifferentSlotNumbers)
{
  // Test different slot numbers create different parameter IDs
  ChannelSend send1 (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, 0, true);

  ChannelSend send2 (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, 1, true);

  ChannelSend send3 (
    dsp::ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ },
    dsp::PortType::Audio, 2, true);

  EXPECT_NE (
    send1.amountParam ()->get_unique_id (),
    send2.amountParam ()->get_unique_id ());
  EXPECT_NE (
    send2.amountParam ()->get_unique_id (),
    send3.amountParam ()->get_unique_id ());
  EXPECT_NE (
    send1.amountParam ()->get_unique_id (),
    send3.amountParam ()->get_unique_id ());
}

TEST_F (ChannelSendTest, EdgeCases)
{
  // Test with zero frames
  audio_send_->prepare_for_processing (sample_rate_, max_block_length_);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 0
  };
  EXPECT_NO_THROW (audio_send_->process_block (time_nfo));

  // Test with maximum amount
  audio_send_->amountParam ()->setBaseValue (1.0f);
  EXPECT_NO_THROW (audio_send_->process_block (time_nfo));

  // Test with zero amount (silence)
  audio_send_->amountParam ()->setBaseValue (0.0f);

  auto [left_in, right_in] = audio_send_->get_stereo_in_ports ();
  auto [left_out, right_out] = audio_send_->get_stereo_out_ports ();

  for (int i = 0; i < 10; i++)
    {
      left_in.buf_[i] = 1.0f;
      right_in.buf_[i] = 1.0f;
    }

  time_nfo.nframes_ = 10;
  audio_send_->process_block (time_nfo);

  for (int i = 0; i < 10; i++)
    {
      EXPECT_NEAR (left_out.buf_[i], 0.0f, 1e-6);
      EXPECT_NEAR (right_out.buf_[i], 0.0f, 1e-6);
    }
}

} // namespace zrythm::structure::tracks
