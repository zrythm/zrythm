// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/cv_port.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class CVPortTest : public ::testing::Test
{
protected:
  static constexpr sample_rate_t SAMPLE_RATE = 44100;
  static constexpr nframes_t     BLOCK_LENGTH = 256;

  void SetUp () override
  {
    input_port = std::make_unique<CVPort> (
      utils::Utf8String::from_utf8_encoded_string ("CV In"), PortFlow::Input);
    output_port = std::make_unique<CVPort> (
      utils::Utf8String::from_utf8_encoded_string ("CV Out"), PortFlow::Output);

    input_port->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);
    output_port->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);

    // Fill output buffer with test signal
    for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
      {
        output_port->buf_[i] =
          0.5f
          * std::cos (
            2.0f * std::numbers::pi_v<float>
            * 1.0f * static_cast<float> (i) / BLOCK_LENGTH);
      }
  }

  std::unique_ptr<CVPort> input_port;
  std::unique_ptr<CVPort> output_port;
};

TEST_F (CVPortTest, BasicProperties)
{
  EXPECT_TRUE (input_port->is_cv ());
  EXPECT_TRUE (input_port->is_input ());
  EXPECT_EQ (input_port->get_label (), "CV In");

  EXPECT_TRUE (output_port->is_cv ());
  EXPECT_TRUE (output_port->is_output ());
}

TEST_F (CVPortTest, BufferClearing)
{
  // Add some data to input buffer
  std::fill (input_port->buf_.begin (), input_port->buf_.end (), 1.0f);
  input_port->clear_buffer (0, BLOCK_LENGTH);

  for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
    {
      EXPECT_EQ (input_port->buf_[i], 0.0f);
    }
}

TEST_F (CVPortTest, SignalProcessing)
{
  // Process output port (signal generation)
  EngineProcessTimeInfo time_info{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  output_port->process_block (time_info);

  // Verify signal was processed
  EXPECT_NE (output_port->buf_[0], 0.0f);
  EXPECT_NE (output_port->audio_ring_, nullptr);
}

TEST_F (CVPortTest, RingBufferWriting)
{
  // Enable ring buffer writing
  RingBufferOwningPortMixin::RingBufferReader reader (*output_port);

  EngineProcessTimeInfo time_info{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  output_port->process_block (time_info);

  // Check ring buffer contents
  float sample = 0.0f;
  EXPECT_TRUE (output_port->audio_ring_->read (sample) > 0);
  EXPECT_NE (sample, 0.0f);
}

TEST_F (CVPortTest, ResourceManagement)
{
  EXPECT_NE (output_port->audio_ring_, nullptr);
  EXPECT_FALSE (output_port->buf_.empty ());

  output_port->release_resources ();

  EXPECT_EQ (output_port->audio_ring_, nullptr);
  EXPECT_TRUE (output_port->buf_.empty ());
}

} // namespace zrythm::dsp
