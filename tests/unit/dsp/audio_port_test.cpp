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
  static constexpr sample_rate_t SAMPLE_RATE = 44100;
  static constexpr nframes_t     BLOCK_LENGTH = 256;

  void SetUp () override
  {
    // Create ports
    mono_input = std::make_unique<AudioPort> (u8"MonoIn", PortFlow::Input);
    stereo_output =
      std::make_unique<AudioPort> (u8"StereoOut", PortFlow::Output, true);

    // Prepare for processing
    mono_input->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);
    stereo_output->prepare_for_processing (SAMPLE_RATE, BLOCK_LENGTH);

    // Fill with test data
    for (nframes_t i = 0; i < BLOCK_LENGTH; i++)
      {
        mono_input->buf_[i] =
          0.5f
          * std::sin (
            2.0f * std::numbers::pi_v<float> * 440.0f * static_cast<float> (i)
            / static_cast<float> (SAMPLE_RATE));
        stereo_output->buf_[i] = 0.0f; // Initialize to silence
      }
  }

  std::unique_ptr<AudioPort> mono_input;
  std::unique_ptr<AudioPort> stereo_output;
};

TEST_F (AudioPortTest, BasicProperties)
{
  EXPECT_TRUE (mono_input->is_audio ());
  EXPECT_TRUE (mono_input->is_input ());
  EXPECT_FALSE (mono_input->is_stereo_port ());

  EXPECT_TRUE (stereo_output->is_audio ());
  EXPECT_TRUE (stereo_output->is_output ());
  EXPECT_TRUE (stereo_output->is_stereo_port ());
}

TEST_F (AudioPortTest, ResourceManagement)
{
  // Verify buffers were allocated
  EXPECT_NE (mono_input->audio_ring_, nullptr);
  EXPECT_NE (stereo_output->audio_ring_, nullptr);

  // Release resources
  mono_input->release_resources ();
  stereo_output->release_resources ();

  // Verify buffers were released
  EXPECT_EQ (mono_input->audio_ring_, nullptr);
  EXPECT_EQ (stereo_output->audio_ring_, nullptr);
  EXPECT_TRUE (mono_input->buf_.empty ());
  EXPECT_TRUE (stereo_output->buf_.empty ());
}

TEST_F (AudioPortTest, StereoPortCreation)
{
  PortRegistry port_registry;
  auto [left_ref, right_ref] = StereoPorts::create_stereo_ports (
    port_registry,
    true, // input
    u8"Stereo", u8"stereo");

  auto left = left_ref.get_object_as<AudioPort> ();
  auto right = right_ref.get_object_as<AudioPort> ();

  EXPECT_EQ (left->get_label ().str (), "Stereo L");
  EXPECT_EQ (right->get_label ().str (), "Stereo R");
  EXPECT_TRUE (left->is_stereo_port ());
  EXPECT_TRUE (right->is_stereo_port ());
}

} // namespace zrythm::dsp
