// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_input_processor.h"
#include "dsp/audio_port.h"
#include "dsp/graph_node.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class AudioInputProcessorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<PortRegistry> (nullptr);
    param_registry_ =
      std::make_unique<ProcessorParameterRegistry> (*port_registry_, nullptr);

    sample_rate_ = units::sample_rate (48000);
    max_block_length_ = units::samples (256);

    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
  }

  void TearDown () override { processor_->release_resources (); }

  /**
   * @brief Creates a processor with 4-channel mock input data.
   *
   * Each channel i is filled with (i+1) * 0.1f for the full block.
   */
  void setup_processor_with_4_channels ()
  {
    static constexpr int num_channels = 4;
    input_data_.resize (num_channels);
    for (auto &ch : input_data_)
      ch.resize (max_block_length_.in<size_t> (units::samples), 0.f);
    input_ptrs_.resize (num_channels);
    num_channels_ = num_channels;

    auto provider = [this] () -> std::span<const float * const> {
      for (int i = 0; i < num_channels_; ++i)
        input_ptrs_[i] = input_data_[i].data ();
      return { input_ptrs_.data (), static_cast<size_t> (num_channels_) };
    };

    processor_ = std::make_unique<AudioInputProcessor> (
      provider, units::channels (num_channels),
      ProcessorBase::ProcessorBaseDependencies{
        .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ });
    processor_->prepare_for_processing (
      nullptr, sample_rate_, max_block_length_);
  }

  std::unique_ptr<PortRegistry>               port_registry_;
  std::unique_ptr<ProcessorParameterRegistry> param_registry_;
  units::sample_rate_t                        sample_rate_;
  units::sample_u32_t                         max_block_length_;
  std::unique_ptr<graph_test::MockTransport>  mock_transport_;
  std::unique_ptr<dsp::TempoMap>              tempo_map_;
  std::unique_ptr<AudioInputProcessor>        processor_;
  std::vector<std::vector<float>>             input_data_;
  std::vector<const float *>                  input_ptrs_;
  int                                         num_channels_ = 0;
};

// ========================================================================
// Construction
// ========================================================================

TEST_F (AudioInputProcessorTest, CreatesCorrectNumberOfOutputPorts)
{
  setup_processor_with_4_channels ();
  // 4 channels: 2 stereo + 4 mono = 6
  const auto &outputs = processor_->get_output_ports ();
  EXPECT_EQ (outputs.size (), 6);
}

TEST_F (AudioInputProcessorTest, StereoPortNames)
{
  setup_processor_with_4_channels ();
  const auto &outputs = processor_->get_output_ports ();
  auto *      port0 = outputs[0].get_object_as<AudioPort> ();
  auto *      port1 = outputs[1].get_object_as<AudioPort> ();
  EXPECT_TRUE (port0->get_label ().view ().contains ("1-2"));
  EXPECT_TRUE (port1->get_label ().view ().contains ("3-4"));
}

TEST_F (AudioInputProcessorTest, MonoPortNames)
{
  setup_processor_with_4_channels ();
  const auto &outputs = processor_->get_output_ports ();
  auto *      mono0 = outputs[2].get_object_as<AudioPort> ();
  EXPECT_TRUE (mono0->get_label ().view ().contains ("1"));
}

TEST_F (AudioInputProcessorTest, OddChannelCountCreatesFewerStereoPorts)
{
  auto provider = [] () -> std::span<const float * const> { return {}; };
  processor_ = std::make_unique<AudioInputProcessor> (
    provider, units::channels (3),
    ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ });
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  const auto &outputs = processor_->get_output_ports ();
  // 3 channels: 1 stereo (1-2) + 3 mono = 4 ports
  EXPECT_EQ (outputs.size (), 4);

  auto * stereo_12 = outputs[0].get_object_as<AudioPort> ();
  EXPECT_TRUE (stereo_12->get_label ().view ().contains ("1-2"));

  auto * mono0 = outputs[1].get_object_as<AudioPort> ();
  EXPECT_TRUE (mono0->get_label ().view ().contains ("1"));
}

TEST_F (AudioInputProcessorTest, ZeroChannelsCreatesNoPorts)
{
  auto provider = [] () -> std::span<const float * const> { return {}; };
  processor_ = std::make_unique<AudioInputProcessor> (
    provider, units::channels (0),
    ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ });
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  const auto &outputs = processor_->get_output_ports ();
  EXPECT_EQ (outputs.size (), 0);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (0),
    .local_offset_ = units::samples (0),
    .nframes_ = max_block_length_
  };
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);
}

// ========================================================================
// Process block — data copying
// ========================================================================

TEST_F (AudioInputProcessorTest, ProcessBlockCopiesDataToStereoPorts)
{
  setup_processor_with_4_channels ();

  for (int ch = 0; ch < num_channels_; ++ch)
    std::fill (
      input_data_[ch].begin (), input_data_[ch].end (),
      static_cast<float> (ch + 1) * 0.1f);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (0),
    .local_offset_ = units::samples (0),
    .nframes_ = max_block_length_
  };
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto &outputs = processor_->get_output_ports ();
  // Stereo port "1-2" (index 0): channels 0 and 1
  auto * stereo_12 = outputs[0].get_object_as<AudioPort> ();
  EXPECT_FLOAT_EQ (stereo_12->buffers ()->getSample (0, 0), 0.1f);
  EXPECT_FLOAT_EQ (stereo_12->buffers ()->getSample (1, 0), 0.2f);

  // Stereo port "3-4" (index 1): channels 2 and 3
  auto * stereo_34 = outputs[1].get_object_as<AudioPort> ();
  EXPECT_FLOAT_EQ (stereo_34->buffers ()->getSample (0, 0), 0.3f);
  EXPECT_FLOAT_EQ (stereo_34->buffers ()->getSample (1, 0), 0.4f);
}

TEST_F (AudioInputProcessorTest, ProcessBlockCopiesDataToMonoPorts)
{
  setup_processor_with_4_channels ();

  for (int ch = 0; ch < num_channels_; ++ch)
    std::fill (
      input_data_[ch].begin (), input_data_[ch].end (),
      static_cast<float> (ch + 1) * 0.1f);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (0),
    .local_offset_ = units::samples (0),
    .nframes_ = max_block_length_
  };
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto &outputs = processor_->get_output_ports ();
  // Mono ports start at index 2: channel 0, 1, 2, 3
  auto * mono0 = outputs[2].get_object_as<AudioPort> ();
  auto * mono1 = outputs[3].get_object_as<AudioPort> ();
  auto * mono2 = outputs[4].get_object_as<AudioPort> ();
  auto * mono3 = outputs[5].get_object_as<AudioPort> ();
  EXPECT_FLOAT_EQ (mono0->buffers ()->getSample (0, 0), 0.1f);
  EXPECT_FLOAT_EQ (mono1->buffers ()->getSample (0, 0), 0.2f);
  EXPECT_FLOAT_EQ (mono2->buffers ()->getSample (0, 0), 0.3f);
  EXPECT_FLOAT_EQ (mono3->buffers ()->getSample (0, 0), 0.4f);
}

TEST_F (AudioInputProcessorTest, ProcessBlockWithEmptyProviderIsNoOp)
{
  auto empty_provider = [] () -> std::span<const float * const> { return {}; };

  processor_ = std::make_unique<AudioInputProcessor> (
    empty_provider, units::channels (4),
    ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ });
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (0),
    .local_offset_ = units::samples (0),
    .nframes_ = max_block_length_
  };
  // Should not crash or modify any buffers
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto &outputs = processor_->get_output_ports ();
  auto *      stereo = outputs[0].get_object_as<AudioPort> ();
  // Output buffer should remain zeroed
  EXPECT_FLOAT_EQ (stereo->buffers ()->getSample (0, 0), 0.f);
  EXPECT_FLOAT_EQ (stereo->buffers ()->getSample (1, 0), 0.f);
}

TEST_F (AudioInputProcessorTest, ProcessBlockWithOffset)
{
  setup_processor_with_4_channels ();
  const size_t block_size = max_block_length_.in<size_t> (units::samples);
  const size_t offset = 64;

  for (int ch = 0; ch < num_channels_; ++ch)
    std::fill (
      input_data_[ch].begin (), input_data_[ch].end (),
      static_cast<float> (ch + 1) * 0.1f);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (offset),
    .local_offset_ = units::samples (offset),
    .nframes_ = units::samples (block_size - offset)
  };
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto &outputs = processor_->get_output_ports ();
  auto *      stereo = outputs[0].get_object_as<AudioPort> ();

  // Before offset: should be 0 (untouched)
  EXPECT_FLOAT_EQ (stereo->buffers ()->getSample (0, 0), 0.f);
  // At offset: should have copied data
  EXPECT_FLOAT_EQ (stereo->buffers ()->getSample (0, offset), 0.1f);
  EXPECT_FLOAT_EQ (stereo->buffers ()->getSample (1, offset), 0.2f);
}

TEST_F (AudioInputProcessorTest, ProcessBlockWithFewerChannelsThanPorts)
{
  // Create processor expecting 4 hw channels, but provider only returns 2
  setup_processor_with_4_channels ();
  num_channels_ = 2;

  for (int ch = 0; ch < 4; ++ch)
    std::fill (
      input_data_[ch].begin (), input_data_[ch].end (),
      static_cast<float> (ch + 1) * 0.1f);

  dsp::graph::EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = units::samples (0),
    .g_start_frame_w_offset_ = units::samples (0),
    .local_offset_ = units::samples (0),
    .nframes_ = max_block_length_
  };
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  const auto &outputs = processor_->get_output_ports ();
  // Stereo "1-2" (channels 0,1): should have data
  auto * stereo_12 = outputs[0].get_object_as<AudioPort> ();
  EXPECT_FLOAT_EQ (stereo_12->buffers ()->getSample (0, 0), 0.1f);
  EXPECT_FLOAT_EQ (stereo_12->buffers ()->getSample (1, 0), 0.2f);

  // Stereo "3-4" (channels 2,3): should be 0 because provider only
  // returned 2 channels
  auto * stereo_34 = outputs[1].get_object_as<AudioPort> ();
  EXPECT_FLOAT_EQ (stereo_34->buffers ()->getSample (0, 0), 0.f);
  EXPECT_FLOAT_EQ (stereo_34->buffers ()->getSample (1, 0), 0.f);
}

} // namespace zrythm::dsp
