// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/passthrough_processors.h"
#include "utils/gtest_wrapper.h"

#include <QObject>

#include <gmock/gmock.h>

using namespace testing;
using namespace zrythm::dsp;

class PassthroughProcessorsTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<PortRegistry> ();
    param_registry_ =
      std::make_unique<ProcessorParameterRegistry> (*port_registry_);
    sample_rate_ = 48000;
    max_block_length_ = 1024;
  }

  void TearDown () override
  {
    // Release resources after each test
    if (midi_proc_)
      midi_proc_->release_resources ();
    if (audio_proc_)
      audio_proc_->release_resources ();
  }

  std::unique_ptr<PortRegistry>               port_registry_;
  std::unique_ptr<ProcessorParameterRegistry> param_registry_;
  sample_rate_t                               sample_rate_;
  nframes_t                                   max_block_length_;
  std::unique_ptr<MidiPassthroughProcessor>   midi_proc_;
  std::unique_ptr<StereoPassthroughProcessor> audio_proc_;
};

TEST_F (PassthroughProcessorsTest, MidiPassthroughBasic)
{
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (
    *port_registry_, *param_registry_);
  midi_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  // Get ports through public accessors
  auto &midi_in = midi_proc_->get_midi_in_port (0);
  auto &midi_out = midi_proc_->get_midi_out_port (0);

  // Create test event
  midi_in.midi_events_.active_events_.add_note_on (1, 0x3C, 0x7F, 3);
  const auto event = midi_in.midi_events_.active_events_.at (0);

  // Process
  EngineProcessTimeInfo time_nfo{ 0, 0, 0, 512 };
  midi_proc_->process_block (time_nfo);

  // Verify
  ASSERT_EQ (midi_out.midi_events_.active_events_.size (), 1);
  EXPECT_EQ (
    midi_out.midi_events_.active_events_.at (0).raw_buffer_, event.raw_buffer_);
}

TEST_F (PassthroughProcessorsTest, AudioPassthroughBasic)
{
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (
    *port_registry_, *param_registry_);
  audio_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  // Get ports through public accessors
  auto &in_l = audio_proc_->get_first_stereo_in_pair ().first;
  auto &in_r = audio_proc_->get_first_stereo_in_pair ().second;
  auto &out_l = audio_proc_->get_first_stereo_out_pair ().first;
  auto &out_r = audio_proc_->get_first_stereo_out_pair ().second;

  // Fill buffers with test data
  for (int i = 0; i < 512; i++)
    {
      in_l.buf_[i] = i * 0.01f;
      in_r.buf_[i] = i * -0.01f;
    }

  // Process
  EngineProcessTimeInfo time_nfo{ 0, 0, 0, 512 };
  audio_proc_->process_block (time_nfo);

  // Verify
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (out_l.buf_[i], i * 0.01f);
      EXPECT_FLOAT_EQ (out_r.buf_[i], i * -0.01f);
    }
}

TEST_F (PassthroughProcessorsTest, ResourceManagement)
{
  // Test prepare/release cycle
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (
    *port_registry_, *param_registry_);
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (
    *port_registry_, *param_registry_);

  // Prepare resources
  midi_proc_->prepare_for_processing (sample_rate_, max_block_length_);
  audio_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  // Verify buffers are allocated
  auto &midi_in = midi_proc_->get_midi_in_port (0);
  auto &audio_in = audio_proc_->get_audio_in_port (0);
  EXPECT_NE (midi_in.midi_ring_, nullptr);
  EXPECT_GE (audio_in.buf_.size (), max_block_length_);

  // Release resources
  midi_proc_->release_resources ();
  audio_proc_->release_resources ();

  // Should handle multiple releases safely
  midi_proc_->release_resources ();
  audio_proc_->release_resources ();
}

TEST_F (PassthroughProcessorsTest, JsonSerializationRoundtrip)
{
  // MIDI processor
  {
    MidiPassthroughProcessor orig (*port_registry_, *param_registry_);
    orig.prepare_for_processing (sample_rate_, max_block_length_);

    nlohmann::json           j = orig;
    MidiPassthroughProcessor deserialized (*port_registry_, *param_registry_);
    from_json (j, deserialized);

    // Reinitialize after deserialization
    deserialized.prepare_for_processing (sample_rate_, max_block_length_);

    // Verify ports
    EXPECT_EQ (
      orig.get_midi_in_port (0).get_uuid (),
      deserialized.get_midi_in_port (0).get_uuid ());
    EXPECT_EQ (
      orig.get_midi_out_port (0).get_uuid (),
      deserialized.get_midi_out_port (0).get_uuid ());
  }

  // Audio processor
  {
    StereoPassthroughProcessor orig (*port_registry_, *param_registry_);
    orig.prepare_for_processing (sample_rate_, max_block_length_);

    nlohmann::json             j = orig;
    StereoPassthroughProcessor deserialized (*port_registry_, *param_registry_);
    from_json (j, deserialized);

    // Reinitialize after deserialization
    deserialized.prepare_for_processing (sample_rate_, max_block_length_);

    // Verify ports
    EXPECT_EQ (
      orig.get_audio_in_port (0).get_uuid (),
      deserialized.get_audio_in_port (0).get_uuid ());
    EXPECT_EQ (
      orig.get_audio_out_port (1).get_uuid (),
      deserialized.get_audio_out_port (1).get_uuid ());
  }
}

TEST_F (PassthroughProcessorsTest, PortInitialization)
{
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (
    *port_registry_, *param_registry_);
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (
    *port_registry_, *param_registry_);

  midi_proc_->prepare_for_processing (sample_rate_, max_block_length_);
  audio_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  // Verify MIDI ports
  auto &midi_in = midi_proc_->get_midi_in_port (0);
  auto &midi_out = midi_proc_->get_midi_out_port (0);
  EXPECT_EQ (midi_in.flow (), PortFlow::Input);
  EXPECT_EQ (midi_out.flow (), PortFlow::Output);
  EXPECT_EQ (midi_in.get_label (), u8"MIDI Passthrough In");
  EXPECT_EQ (midi_out.get_label (), u8"MIDI Passthrough Out");

  // Verify audio ports
  auto &audio_in_l = audio_proc_->get_audio_in_port (0);
  auto &audio_out_l = audio_proc_->get_audio_out_port (0);

  EXPECT_EQ (audio_in_l.flow (), PortFlow::Input);
  EXPECT_EQ (audio_out_l.flow (), PortFlow::Output);
  EXPECT_EQ (audio_in_l.get_label (), u8"Stereo Passthrough In L");
  EXPECT_EQ (audio_out_l.get_label (), u8"Stereo Passthrough Out L");
}

TEST_F (PassthroughProcessorsTest, ZeroFramesProcessing)
{
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (
    *port_registry_, *param_registry_);
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (
    *port_registry_, *param_registry_);

  midi_proc_->prepare_for_processing (sample_rate_, max_block_length_);
  audio_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  EngineProcessTimeInfo time_nfo{ 0, 0, 0, 0 };

  // Should handle without crashing
  midi_proc_->process_block (time_nfo);
  audio_proc_->process_block (time_nfo);
}

TEST_F (PassthroughProcessorsTest, LargeBufferHandling)
{
  const int large_size = 8192;
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (
    *port_registry_, *param_registry_);
  audio_proc_->prepare_for_processing (sample_rate_, large_size);

  // Get ports
  auto &in_l = audio_proc_->get_audio_in_port (0);
  auto &in_r = audio_proc_->get_audio_in_port (1);
  auto &out_l = audio_proc_->get_audio_out_port (0);
  auto &out_r = audio_proc_->get_audio_out_port (1);

  // Fill with pattern
  for (int i = 0; i < large_size; i++)
    {
      in_l.buf_[i] = sinf (i * 0.1f);
      in_r.buf_[i] = cosf (i * 0.1f);
    }

  EngineProcessTimeInfo time_nfo{ 0, 0, 0, large_size };
  audio_proc_->process_block (time_nfo);

  // Verify
  for (int i = 0; i < large_size; i++)
    {
      EXPECT_FLOAT_EQ (out_l.buf_[i], sinf (i * 0.1f));
      EXPECT_FLOAT_EQ (out_r.buf_[i], cosf (i * 0.1f));
    }
}
