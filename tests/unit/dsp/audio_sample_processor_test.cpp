// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/audio_port.h"
#include "dsp/audio_sample_processor.h"
#include "dsp/port.h"
#include "dsp/processor_base.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class AudioSampleProcessorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<PortRegistry> ();
    param_registry_ =
      std::make_unique<ProcessorParameterRegistry> (*port_registry_);

    processor_ = std::make_unique<
      AudioSampleProcessor> (ProcessorBase::ProcessorBaseDependencies{
      .port_registry_ = *port_registry_, .param_registry_ = *param_registry_ });

    sample_rate_ = 48000;
    max_block_length_ = 1024;

    // Set up mock transport
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
  }

  void TearDown () override
  {
    if (processor_)
      {
        processor_->release_resources ();
      }
  }

  void prepare_processor ()
  {
    processor_->prepare_for_processing (sample_rate_, max_block_length_);
  }

  std::unique_ptr<PortRegistry>               port_registry_;
  std::unique_ptr<ProcessorParameterRegistry> param_registry_;
  std::unique_ptr<AudioSampleProcessor>       processor_;
  sample_rate_t                               sample_rate_;
  nframes_t                                   max_block_length_;
  std::unique_ptr<graph_test::MockTransport>  mock_transport_;
};

TEST_F (AudioSampleProcessorTest, ConstructionAndBasicProperties)
{
  // Test that processor is properly constructed
  EXPECT_NE (processor_, nullptr);

  // Test port configuration
  EXPECT_EQ (processor_->get_output_ports ().size (), 1);

  auto output_port = processor_->get_output_audio_port_non_rt ();
  EXPECT_EQ (
    output_port->get_node_name (), u8"Audio Sample Processor/Stereo Out");
}

TEST_F (AudioSampleProcessorTest, PortConfiguration)
{
  auto output_ports = processor_->get_output_ports ();
  EXPECT_EQ (output_ports.size (), 1);

  const auto port = processor_->get_output_audio_port_non_rt ();

  EXPECT_NE (port, nullptr);
}

TEST_F (AudioSampleProcessorTest, PlayableSampleSingleChannelConstruction)
{
  std::vector<float> test_buffer = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };

  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    AudioSampleProcessor::PlayableSampleSingleChannel::UnmutableSampleSpan (
      test_buffer),
    0,    // channel_index
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  EXPECT_EQ (sample.buf_.size (), 5);
  EXPECT_EQ (sample.channel_index_, 0);
  EXPECT_EQ (sample.volume_, 1.0f);
  EXPECT_EQ (sample.start_offset_, 0);
  EXPECT_EQ (sample.offset_, 0);
}

TEST_F (AudioSampleProcessorTest, AddSampleToProcess)
{
  prepare_processor ();

  std::vector<float> test_buffer = { 0.1f, 0.2f, 0.3f, 0.4f, 0.5f };

  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    AudioSampleProcessor::PlayableSampleSingleChannel::UnmutableSampleSpan (
      test_buffer),
    0,    // channel_index
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  // Should not throw
  EXPECT_NO_THROW (processor_->add_sample_to_process (sample));
}

TEST_F (AudioSampleProcessorTest, BasicAudioProcessing)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  // Create a simple test sample
  std::vector<float> test_buffer = { 0.5f, 0.5f, 0.5f, 0.5f, 0.5f };

  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    0,    // left channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // Check that the sample was processed
  for (size_t i = 0; i < 5; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.5f, 0.001f);
      EXPECT_NEAR (port->buffers ()->getSample (1, i), 0.0f, 0.001f);
    }
}

TEST_F (AudioSampleProcessorTest, VolumeScaling)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  std::vector<float> test_buffer = { 1.0f, 1.0f, 1.0f };

  // Test with 0.5 volume
  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    0,    // left channel
    0.5f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 3
  };

  processor_->process_block (time_nfo, *mock_transport_);

  for (size_t i = 0; i < 3; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.5f, 0.001f);
    }
}

TEST_F (AudioSampleProcessorTest, StartOffset)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  std::vector<float> test_buffer = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };

  // Test with start offset of 2 frames
  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    0,    // left channel
    1.0f, // volume
    2,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // First 2 frames should be 0 (due to start offset)
  EXPECT_NEAR (port->buffers ()->getSample (0, 0), 0.0f, 0.001f);
  EXPECT_NEAR (port->buffers ()->getSample (0, 1), 0.0f, 0.001f);

  // Next 3 frames should have the sample data
  EXPECT_NEAR (port->buffers ()->getSample (0, 2), 1.0f, 0.001f);
  EXPECT_NEAR (port->buffers ()->getSample (0, 3), 1.0f, 0.001f);
  EXPECT_NEAR (port->buffers ()->getSample (0, 4), 1.0f, 0.001f);
}

TEST_F (AudioSampleProcessorTest, MultipleSamples)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  // Add sample to left channel
  std::vector<float> left_buffer = { 0.3f, 0.3f, 0.3f };
  AudioSampleProcessor::PlayableSampleSingleChannel left_sample (
    std::span<const float> (left_buffer),
    0,    // left channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  // Add sample to right channel
  std::vector<float> right_buffer = { 0.7f, 0.7f, 0.7f };
  AudioSampleProcessor::PlayableSampleSingleChannel right_sample (
    std::span<const float> (right_buffer),
    1,    // right channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (left_sample);
  processor_->add_sample_to_process (right_sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 3
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // Check both channels
  for (size_t i = 0; i < 3; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.3f, 0.001f);
      EXPECT_NEAR (port->buffers ()->getSample (1, i), 0.7f, 0.001f);
    }
}

TEST_F (AudioSampleProcessorTest, SampleSpanningMultipleCycles)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  // Create a longer sample that spans multiple processing cycles
  std::vector<float> test_buffer (10, 0.8f); // 10 samples

  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    0,    // left channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  // First cycle - process 5 frames
  EngineProcessTimeInfo time_nfo1{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo1, *mock_transport_);

  // Check first 5 frames
  for (size_t i = 0; i < 5; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.8f, 0.001f);
    }

  // Second cycle - process remaining 5 frames
  EngineProcessTimeInfo time_nfo2{
    .g_start_frame_ = 5,
    .g_start_frame_w_offset_ = 5,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo2, *mock_transport_);

  // Check next 5 frames
  for (size_t i = 0; i < 5; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.8f, 0.001f);
    }
}

TEST_F (AudioSampleProcessorTest, LargeStartOffset)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  std::vector<float> test_buffer = { 1.0f, 1.0f, 1.0f };

  // Test with start offset larger than cycle size
  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    0,    // left channel
    1.0f, // volume
    10,   // start_offset (larger than cycle size)
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // All frames should be 0 (sample hasn't started yet)
  for (size_t i = 0; i < 5; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.0f, 0.001f);
    }

  // Process another cycle - frames 5-9, sample still hasn't started (starts at
  // 10)
  EngineProcessTimeInfo time_nfo2{
    .g_start_frame_ = 5,
    .g_start_frame_w_offset_ = 5,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo2, *mock_transport_);

  // All frames should still be 0 (sample starts at frame 10)
  for (size_t i = 0; i < 5; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.0f, 0.001f);
    }

  // Process a third cycle - frames 10-14, sample should start here
  EngineProcessTimeInfo time_nfo3{
    .g_start_frame_ = 10,
    .g_start_frame_w_offset_ = 10,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo3, *mock_transport_);

  // Now we should have sample data starting from frame 10
  EXPECT_NEAR (port->buffers ()->getSample (0, 0), 1.0f, 0.001f);
  EXPECT_NEAR (port->buffers ()->getSample (0, 1), 1.0f, 0.001f);
  EXPECT_NEAR (port->buffers ()->getSample (0, 2), 1.0f, 0.001f);
  EXPECT_NEAR (port->buffers ()->getSample (0, 3), 0.0f, 0.001f);
  EXPECT_NEAR (port->buffers ()->getSample (0, 4), 0.0f, 0.001f);
}

TEST_F (AudioSampleProcessorTest, ChannelIndexValidation)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  std::vector<float> test_buffer = { 1.0f, 1.0f, 1.0f };

  // Test with invalid channel index (should be ignored)
  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    5,    // invalid channel index
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 3
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // Should not crash and should not modify output
  for (size_t i = 0; i < 3; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.0f, 0.001f);
    }
}

TEST_F (AudioSampleProcessorTest, EmptyBuffer)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  // Test with empty buffer
  std::vector<float> empty_buffer;

  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (empty_buffer),
    0,    // left channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // Should not crash and should not modify output
  for (size_t i = 0; i < 5; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.0f, 0.001f);
    }
}

TEST_F (AudioSampleProcessorTest, ZeroFrames)
{
  prepare_processor ();

  std::vector<float> test_buffer = { 1.0f, 1.0f, 1.0f };

  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    0,    // left channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 0 // Zero frames
  };

  // Should not crash
  EXPECT_NO_THROW (processor_->process_block (time_nfo, *mock_transport_));
}

TEST_F (AudioSampleProcessorTest, PrepareForProcessing)
{
  // Add some samples first
  std::vector<float> test_buffer = { 1.0f, 1.0f, 1.0f };

  AudioSampleProcessor::PlayableSampleSingleChannel sample (
    std::span<const float> (test_buffer),
    0,    // left channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  processor_->add_sample_to_process (sample);

  // Prepare should clear the queue
  processor_->prepare_for_processing (sample_rate_, max_block_length_);

  // After prepare, the queue should be empty
  // We can verify this by processing and checking no samples are played
  const auto * port = processor_->get_output_audio_port_non_rt ();

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 5
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // Should not have any sample data
  for (size_t i = 0; i < 5; ++i)
    {
      EXPECT_NEAR (port->buffers ()->getSample (0, i), 0.0f, 0.001f);
    }
}

TEST_F (AudioSampleProcessorTest, ConcurrentSampleProcessing)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  // Add multiple samples that overlap
  std::vector<float> buffer1 = { 0.5f, 0.5f, 0.5f };
  std::vector<float> buffer2 = { 0.3f, 0.3f, 0.3f };

  AudioSampleProcessor::PlayableSampleSingleChannel sample1 (
    std::span<const float> (buffer1),
    0,    // left channel
    1.0f, // volume
    0,    // start_offset
    std::source_location::current ());

  AudioSampleProcessor::PlayableSampleSingleChannel sample2 (
    std::span<const float> (buffer2),
    0,    // left channel
    1.0f, // volume
    1,    // start_offset (delayed by 1 frame)
    std::source_location::current ());

  processor_->add_sample_to_process (sample1);
  processor_->add_sample_to_process (sample2);

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 4
  };

  processor_->process_block (time_nfo, *mock_transport_);

  // Check mixed results
  EXPECT_NEAR (port->buffers ()->getSample (0, 0), 0.5f, 0.001f); // Only sample1
  EXPECT_NEAR (
    port->buffers ()->getSample (0, 1), 0.8f, 0.001f); // sample1 + sample2
  EXPECT_NEAR (
    port->buffers ()->getSample (0, 2), 0.8f, 0.001f); // sample1 + sample2
  EXPECT_NEAR (port->buffers ()->getSample (0, 3), 0.3f, 0.001f); // Only sample2
}

TEST_F (AudioSampleProcessorTest, ResourceCleanup)
{
  prepare_processor ();

  auto * const port = processor_->get_output_audio_port_non_rt ();

  // Verify ports are prepared
  EXPECT_GE (port->buffers ()->getNumSamples (), max_block_length_);

  // Release resources
  processor_->release_resources ();

  // Verify buffers are cleared
  EXPECT_EQ (port->buffers (), nullptr);
}

} // namespace zrythm::dsp
