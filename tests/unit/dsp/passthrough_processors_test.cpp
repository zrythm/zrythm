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
    dependencies_ = std::make_unique<ProcessorBase::ProcessorBaseDependencies> (
      *port_registry_, *param_registry_);
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

  std::unique_ptr<PortRegistry>                             port_registry_;
  std::unique_ptr<ProcessorParameterRegistry>               param_registry_;
  std::unique_ptr<ProcessorBase::ProcessorBaseDependencies> dependencies_;
  sample_rate_t                                             sample_rate_;
  nframes_t                                                 max_block_length_;
  std::unique_ptr<MidiPassthroughProcessor>                 midi_proc_;
  std::unique_ptr<StereoPassthroughProcessor>               audio_proc_;
};

TEST_F (PassthroughProcessorsTest, MidiPassthroughBasic)
{
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (*dependencies_);
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
  ASSERT_EQ (midi_out.midi_events_.queued_events_.size (), 1);
  EXPECT_EQ (
    midi_out.midi_events_.queued_events_.at (0).raw_buffer_, event.raw_buffer_);
}

TEST_F (PassthroughProcessorsTest, AudioPassthroughBasic)
{
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (*dependencies_);
  audio_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  // Get ports through public accessors
  auto &in = audio_proc_->get_audio_in_port ();
  auto &out = audio_proc_->get_audio_out_port ();

  // Fill buffers with test data
  for (int i = 0; i < 512; i++)
    {
      in.buffers ()->setSample (0, i, static_cast<float> (i) * 0.01f);
      in.buffers ()->setSample (1, i, static_cast<float> (i) * -0.01f);
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (0, i), 0.f);
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (1, i), 0.f);
    }

  // Process
  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };
  audio_proc_->process_block (time_nfo);

  // Verify
  for (int i = 0; i < 512; i++)
    {
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (0, i), i * 0.01f);
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (1, i), i * -0.01f);
    }
}

TEST_F (PassthroughProcessorsTest, ResourceManagement)
{
  // Test prepare/release cycle
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (*dependencies_);
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (*dependencies_);

  // Prepare resources
  midi_proc_->prepare_for_processing (sample_rate_, max_block_length_);
  audio_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  // Verify buffers are allocated
  auto &midi_in = midi_proc_->get_midi_in_port (0);
  auto &audio_in = audio_proc_->get_audio_in_port ();
  EXPECT_NE (midi_in.midi_ring_, nullptr);
  EXPECT_GE (audio_in.buffers ()->getNumSamples (), max_block_length_);

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
    MidiPassthroughProcessor orig (*dependencies_);
    orig.prepare_for_processing (sample_rate_, max_block_length_);

    nlohmann::json           j = orig;
    MidiPassthroughProcessor deserialized (*dependencies_);
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
    StereoPassthroughProcessor orig (*dependencies_);
    orig.prepare_for_processing (sample_rate_, max_block_length_);

    nlohmann::json             j = orig;
    StereoPassthroughProcessor deserialized (*dependencies_);
    from_json (j, deserialized);

    // Reinitialize after deserialization
    deserialized.prepare_for_processing (sample_rate_, max_block_length_);

    // Verify ports
    EXPECT_EQ (
      orig.get_audio_in_port ().get_uuid (),
      deserialized.get_audio_in_port ().get_uuid ());
    EXPECT_EQ (
      orig.get_audio_out_port ().get_uuid (),
      deserialized.get_audio_out_port ().get_uuid ());
  }
}

TEST_F (PassthroughProcessorsTest, PortInitialization)
{
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (*dependencies_);
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (*dependencies_);

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
  auto &audio_in = audio_proc_->get_audio_in_port ();
  auto &audio_out = audio_proc_->get_audio_out_port ();

  EXPECT_EQ (audio_in.flow (), PortFlow::Input);
  EXPECT_EQ (audio_out.flow (), PortFlow::Output);
  EXPECT_EQ (audio_in.get_label (), u8"Audio Passthrough In");
  EXPECT_EQ (audio_out.get_label (), u8"Audio Passthrough Out");
}

TEST_F (PassthroughProcessorsTest, ZeroFramesProcessing)
{
  midi_proc_ = std::make_unique<MidiPassthroughProcessor> (*dependencies_);
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (*dependencies_);

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
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (*dependencies_);
  audio_proc_->prepare_for_processing (sample_rate_, large_size);

  // Get ports
  auto &in = audio_proc_->get_audio_in_port ();
  auto &out = audio_proc_->get_audio_out_port ();

  // Fill with pattern
  for (int i = 0; i < large_size; i++)
    {
      in.buffers ()->setSample (0, i, sinf (static_cast<float> (i) * 0.1f));
      in.buffers ()->setSample (1, i, cosf (static_cast<float> (i) * 0.1f));
    }

  EngineProcessTimeInfo time_nfo{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = large_size
  };
  audio_proc_->process_block (time_nfo);

  // Verify
  for (int i = 0; i < large_size; i++)
    {
      EXPECT_NEAR (out.buffers ()->getSample (0, i), sinf (i * 0.1f), 1e-5f);
      EXPECT_NEAR (out.buffers ()->getSample (1, i), cosf (i * 0.1f), 1e-5f);
    }
}

TEST_F (PassthroughProcessorsTest, MultipleProcessingCallsStereo)
{
  audio_proc_ = std::make_unique<StereoPassthroughProcessor> (*dependencies_);
  audio_proc_->prepare_for_processing (sample_rate_, max_block_length_);

  // Get ports through public accessors
  auto &in = audio_proc_->get_audio_in_port ();
  auto &out = audio_proc_->get_audio_out_port ();

  // Fill buffers with test data for first call
  for (int i = 0; i < 256; i++)
    {
      in.buffers ()->setSample (0, i, static_cast<float> (i) * 0.01f);
      in.buffers ()->setSample (1, i, static_cast<float> (i) * -0.01f);
    }

  // First process call
  EngineProcessTimeInfo time_nfo1{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 256
  };
  audio_proc_->process_block (time_nfo1);

  // Verify first processing
  for (int i = 0; i < 256; i++)
    {
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (0, i), i * 0.01f);
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (1, i), i * -0.01f);
    }

  // Clear input buffer and fill with new data for second call
  in.buffers ()->clear ();
  for (int i = 0; i < 256; i++)
    {
      in.buffers ()->setSample (0, i, static_cast<float> (i) * 0.02f);
      in.buffers ()->setSample (1, i, static_cast<float> (i) * -0.02f);
    }

  // Second process call
  EngineProcessTimeInfo time_nfo2{
    .g_start_frame_ = 256,
    .g_start_frame_w_offset_ = 256,
    .local_offset_ = 0,
    .nframes_ = 256
  };
  audio_proc_->process_block (time_nfo2);

  // Verify second processing (should not accumulate with first)
  for (int i = 0; i < 256; i++)
    {
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (0, i), i * 0.02f);
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (1, i), i * -0.02f);
    }

  // Third call with different offset
  in.buffers ()->clear ();
  for (int i = 0; i < 128; i++)
    {
      in.buffers ()->setSample (0, i, static_cast<float> (i) * 0.03f);
      in.buffers ()->setSample (1, i, static_cast<float> (i) * -0.03f);
    }

  EngineProcessTimeInfo time_nfo3{
    .g_start_frame_ = 512,
    .g_start_frame_w_offset_ = 512,
    .local_offset_ = 0,
    .nframes_ = 128
  };
  audio_proc_->process_block (time_nfo3);

  // Verify third processing
  for (int i = 0; i < 128; i++)
    {
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (0, i), i * 0.03f);
      EXPECT_FLOAT_EQ (out.buffers ()->getSample (1, i), i * -0.03f);
    }

  // Verify that previous output samples are not affected (no accumulation).
  // Either values are equal to previous run, or zero, depending on the
  // implementation.
  for (int i = 128; i < 256; i++)
    {
      EXPECT_FLOAT_EQ (
        out.buffers ()->getSample (0, i), static_cast<float> (i) * 0.02f);
      EXPECT_FLOAT_EQ (
        out.buffers ()->getSample (1, i), static_cast<float> (i) * -0.02f);
    }
}
