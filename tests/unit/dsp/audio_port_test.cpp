// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/panning.h"
#include "dsp/port_all.h"
#include "utils/icloneable.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class AudioPortTest : public ::testing::Test
{
protected:
  static constexpr auto      SAMPLE_RATE = units::sample_rate (44100);
  static constexpr nframes_t BLOCK_LENGTH = 256;

  void SetUp () override
  {
    // Create ports
    mono_input = std::make_unique<AudioPort> (
      u8"MonoIn", PortFlow::Input, AudioPort::BusLayout::Mono, 1);
    stereo_output = std::make_unique<AudioPort> (
      u8"StereoOut", PortFlow::Output, AudioPort::BusLayout::Stereo, 2);

    // Prepare for processing
    mono_input->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);
    stereo_output->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);

    // Fill with test data
    for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
      {
        float sample =
          0.5f
          * std::sin (
            2.0f * std::numbers::pi_v<float> * 440.0f * static_cast<float> (i)
            / static_cast<float> (SAMPLE_RATE.in (units::sample_rate)));

        // For mono input, write to channel 0
        mono_input->buffers ()->setSample (0, static_cast<int> (i), sample);

        // For stereo output, initialize both channels to silence
        stereo_output->buffers ()->setSample (0, static_cast<int> (i), 0.0f);
        stereo_output->buffers ()->setSample (1, static_cast<int> (i), 0.0f);
      }
  }

  std::unique_ptr<AudioPort> mono_input;
  std::unique_ptr<AudioPort> stereo_output;
};

TEST_F (AudioPortTest, BasicProperties)
{
  EXPECT_TRUE (mono_input->is_audio ());
  EXPECT_TRUE (mono_input->is_input ());
  EXPECT_EQ (mono_input->layout (), AudioPort::BusLayout::Mono);
  EXPECT_EQ (mono_input->purpose (), AudioPort::Purpose::Main);
  EXPECT_EQ (mono_input->num_channels (), 1);

  EXPECT_TRUE (stereo_output->is_audio ());
  EXPECT_TRUE (stereo_output->is_output ());
  EXPECT_EQ (stereo_output->layout (), AudioPort::BusLayout::Stereo);
  EXPECT_EQ (stereo_output->purpose (), AudioPort::Purpose::Main);
  EXPECT_EQ (stereo_output->num_channels (), 2);
}

TEST_F (AudioPortTest, ResourceManagement)
{
  // Verify buffers were allocated
  EXPECT_FALSE (mono_input->audio_ring_buffers ().empty ());
  EXPECT_FALSE (stereo_output->audio_ring_buffers ().empty ());
  EXPECT_EQ (mono_input->audio_ring_buffers ().size (), 1u);
  EXPECT_EQ (stereo_output->audio_ring_buffers ().size (), 2u);
  EXPECT_NE (mono_input->buffers ()->getArrayOfReadPointers (), nullptr);
  EXPECT_NE (stereo_output->buffers ()->getArrayOfReadPointers (), nullptr);

  // Release resources
  mono_input->release_resources ();
  stereo_output->release_resources ();

  // Verify buffers were released
  EXPECT_TRUE (mono_input->audio_ring_buffers ().empty ());
  EXPECT_TRUE (stereo_output->audio_ring_buffers ().empty ());
  EXPECT_EQ (mono_input->buffers (), nullptr);
  EXPECT_EQ (stereo_output->buffers (), nullptr);
}

TEST_F (AudioPortTest, StereoPortsHelper)
{
  // Test the StereoPorts::get_name_and_symbols helper function
  auto [left_name, left_symbol] =
    StereoPorts::get_name_and_symbols (true, u8"Stereo", u8"stereo");
  auto [right_name, right_symbol] =
    StereoPorts::get_name_and_symbols (false, u8"Stereo", u8"stereo");

  EXPECT_EQ (left_name.str (), "Stereo L");
  EXPECT_EQ (right_name.str (), "Stereo R");
  EXPECT_EQ (left_symbol.str (), "stereo_l");
  EXPECT_EQ (right_symbol.str (), "stereo_r");
}

TEST_F (AudioPortTest, LimitingFunctionality)
{
  auto port = std::make_unique<AudioPort> (
    u8"TestPort", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  port->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);

  // Initially should not require limiting
  EXPECT_FALSE (port->requires_limiting ());

  // Enable limiting
  port->mark_as_requires_limiting ();
  EXPECT_TRUE (port->requires_limiting ());
}

TEST_F (AudioPortTest, DifferentBusLayouts)
{
  // Test creating ports with different bus layouts
  auto mono_port = std::make_unique<AudioPort> (
    u8"Mono", PortFlow::Output, AudioPort::BusLayout::Mono, 1);
  auto stereo_port = std::make_unique<AudioPort> (
    u8"Stereo", PortFlow::Output, AudioPort::BusLayout::Stereo, 2);

  EXPECT_EQ (mono_port->layout (), AudioPort::BusLayout::Mono);
  EXPECT_EQ (stereo_port->layout (), AudioPort::BusLayout::Stereo);

  EXPECT_EQ (mono_port->num_channels (), 1);
  EXPECT_EQ (stereo_port->num_channels (), 2);
}

TEST_F (AudioPortTest, DifferentPurposes)
{
  // Test creating ports with different purposes
  auto main_port = std::make_unique<AudioPort> (
    u8"Main", PortFlow::Output, AudioPort::BusLayout::Mono, 1,
    AudioPort::Purpose::Main);
  auto sidechain_port = std::make_unique<AudioPort> (
    u8"Sidechain", PortFlow::Output, AudioPort::BusLayout::Mono, 1,
    AudioPort::Purpose::Sidechain);

  EXPECT_EQ (main_port->purpose (), AudioPort::Purpose::Main);
  EXPECT_EQ (sidechain_port->purpose (), AudioPort::Purpose::Sidechain);
}

} // namespace zrythm::dsp
