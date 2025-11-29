// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_panic_processor.h"

#include <QObject>

#include "unit/dsp/graph_helpers.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class MidiPanicProcessorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    port_registry_ = std::make_unique<PortRegistry> ();
    param_registry_ =
      std::make_unique<ProcessorParameterRegistry> (*port_registry_);
    dependencies_ = std::make_unique<ProcessorBase::ProcessorBaseDependencies> (
      *port_registry_, *param_registry_);
    sample_rate_ = units::sample_rate (48000);
    max_block_length_ = 1024;

    panic_proc_ = std::make_unique<MidiPanicProcessor> (*dependencies_, nullptr);
    panic_proc_->prepare_for_processing (
      nullptr, sample_rate_, max_block_length_);

    midi_out_ =
      panic_proc_->get_output_ports ().front ().get_object_as<dsp::MidiPort> ();

    // Set up mock transport
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
  }

  void TearDown () override
  {
    if (panic_proc_)
      panic_proc_->release_resources ();
  }

  std::unique_ptr<PortRegistry>                             port_registry_;
  std::unique_ptr<ProcessorParameterRegistry>               param_registry_;
  std::unique_ptr<ProcessorBase::ProcessorBaseDependencies> dependencies_;
  units::sample_rate_t                                      sample_rate_;
  nframes_t                                                 max_block_length_;
  std::unique_ptr<MidiPanicProcessor>                       panic_proc_;
  dsp::MidiPort *                                           midi_out_;
  std::unique_ptr<graph_test::MockTransport>                mock_transport_;
};

TEST_F (MidiPanicProcessorTest, BasicPanicOperation)
{
  // Request panic and process block
  panic_proc_->request_panic ();
  EngineProcessTimeInfo time_nfo{ 0, 0, 0, 512 };
  panic_proc_->process_block (time_nfo, *mock_transport_);

  // Verify 16 channels worth of All Notes Off messages
  ASSERT_EQ (midi_out_->midi_events_.queued_events_.size (), 16);

  for (int i = 0; i < 16; i++)
    {
      const auto &ev = midi_out_->midi_events_.queued_events_.at (i);
      EXPECT_EQ (ev.raw_buffer_[0] & 0xF0, 0xB0); // Control Change
      EXPECT_EQ (ev.raw_buffer_[1], 0x7B);        // All Notes Off
      EXPECT_EQ (ev.raw_buffer_[2], 0x00);        // Value
      EXPECT_EQ (ev.time_, 0);
    }
}

TEST_F (MidiPanicProcessorTest, MultiplePanicRequests)
{
  const EngineProcessTimeInfo time_nfo = {
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = 512
  };

  // First panic request
  panic_proc_->request_panic ();
  panic_proc_->process_block (time_nfo, *mock_transport_);
  midi_out_->process_block (time_nfo, *mock_transport_);
  ASSERT_EQ (midi_out_->midi_events_.active_events_.size (), 16);

  // Second request without processing
  panic_proc_->request_panic ();
  panic_proc_->request_panic ();
  panic_proc_->process_block (time_nfo, *mock_transport_);
  midi_out_->process_block (time_nfo, *mock_transport_);
  ASSERT_EQ (midi_out_->midi_events_.active_events_.size (), 16);

  // No new requests - should be empty
  panic_proc_->process_block (time_nfo, *mock_transport_);
  midi_out_->process_block (time_nfo, *mock_transport_);
  EXPECT_TRUE (midi_out_->midi_events_.active_events_.empty ());
}

TEST_F (MidiPanicProcessorTest, ResourceManagement)
{
  // Test prepare/release cycle
  panic_proc_->release_resources ();
  panic_proc_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  // Should handle multiple releases
  panic_proc_->release_resources ();
  panic_proc_->release_resources ();

  // Reinitialize and test functionality
  panic_proc_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);
  panic_proc_->request_panic ();
  panic_proc_->process_block ({ 0, 0, 0, 512 }, *mock_transport_);
  EXPECT_EQ (midi_out_->midi_events_.queued_events_.size (), 16);
}

TEST_F (MidiPanicProcessorTest, ZeroFramesProcessing)
{
  panic_proc_->request_panic ();
  EngineProcessTimeInfo time_nfo{ 0, 0, 0, 0 };
  panic_proc_->process_block (time_nfo, *mock_transport_);

  // Should still process panic even with 0 frames
  ASSERT_EQ (midi_out_->midi_events_.queued_events_.size (), 16);
}

TEST_F (MidiPanicProcessorTest, LargeBufferHandling)
{
  const int large_size = 8192;
  panic_proc_->prepare_for_processing (nullptr, sample_rate_, large_size);

  panic_proc_->request_panic ();
  EngineProcessTimeInfo time_nfo{ 0, 0, 0, large_size };
  panic_proc_->process_block (time_nfo, *mock_transport_);

  // Verify all 16 channels processed
  ASSERT_EQ (midi_out_->midi_events_.queued_events_.size (), 16);
  for (const auto &ev : midi_out_->midi_events_.queued_events_)
    {
      EXPECT_EQ (ev.raw_buffer_[1], 0x7B); // All Notes Off
    }
}
}
