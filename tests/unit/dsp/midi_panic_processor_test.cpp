// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_panic_processor.h"
#include "utils/object_registry.h"

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
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    sample_rate_ = units::sample_rate (48000);
    max_block_length_ = units::samples (1024);

    panic_proc_ = std::make_unique<MidiPanicProcessor> (*registry_, nullptr);
    panic_proc_->prepare_for_processing (
      nullptr, sample_rate_, max_block_length_);

    midi_out_ =
      panic_proc_->get_output_ports ().front ().get_object_as<dsp::MidiPort> ();

    // Set up mock transport
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
  }

  void TearDown () override
  {
    if (panic_proc_)
      panic_proc_->release_resources ();
  }

  std::unique_ptr<utils::ObjectRegistry>     registry_;
  units::sample_rate_t                       sample_rate_;
  units::sample_u32_t                        max_block_length_;
  std::unique_ptr<MidiPanicProcessor>        panic_proc_;
  dsp::MidiPort *                            midi_out_;
  std::unique_ptr<graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>             tempo_map_;
};

TEST_F (MidiPanicProcessorTest, BasicPanicOperation)
{
  panic_proc_->request_panic ();
  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (512));
  panic_proc_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  // Verify 16 channels worth of All Notes Off messages
  ASSERT_EQ (midi_out_->buffer_.size (), 16);

  for (
    const auto [ch, ev] :
    std::views::zip (std::views::iota (0), midi_out_->buffer_))
    {
      const auto data = ev.data ();
      EXPECT_EQ (data[0], 0xB0 | ch); // CC on expected channel
      EXPECT_EQ (data[1], 0x7B);      // All Notes Off
      EXPECT_EQ (data[2], 0x00);      // Value
      EXPECT_EQ (ev.time (), units::samples (0));
    }
}

TEST_F (MidiPanicProcessorTest, MultiplePanicRequests)
{
  const auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (512));

  // First panic request
  panic_proc_->request_panic ();
  panic_proc_->process_block (time_nfo, *mock_transport_, *tempo_map_);
  ASSERT_EQ (midi_out_->buffer_.size (), 16);

  // Second request without processing
  panic_proc_->request_panic ();
  panic_proc_->request_panic ();
  panic_proc_->process_block (time_nfo, *mock_transport_, *tempo_map_);
  ASSERT_EQ (midi_out_->buffer_.size (), 16);

  // No new requests - should be empty
  panic_proc_->process_block (time_nfo, *mock_transport_, *tempo_map_);
  EXPECT_TRUE (midi_out_->buffer_.empty ());
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
  panic_proc_->process_block (
    { .transport_position_ = units::samples (0),
      .buffer_offset_ = units::samples (0),
      .nframes_ = units::samples (512) },
    *mock_transport_, *tempo_map_);
  EXPECT_EQ (midi_out_->buffer_.size (), 16);
}

TEST_F (MidiPanicProcessorTest, ZeroFramesProcessing)
{
  panic_proc_->request_panic ();
  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), units::samples (0));
  panic_proc_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  // Should still process panic even with 0 frames
  ASSERT_EQ (midi_out_->buffer_.size (), 16);
}

TEST_F (MidiPanicProcessorTest, LargeBufferHandling)
{
  constexpr auto large_size = units::samples (8192);
  panic_proc_->prepare_for_processing (nullptr, sample_rate_, large_size);

  panic_proc_->request_panic ();
  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), large_size);
  panic_proc_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  // Verify all 16 channels processed
  ASSERT_EQ (midi_out_->buffer_.size (), 16);
  for (const auto &ev : midi_out_->buffer_)
    {
      EXPECT_EQ (ev.data ()[1], 0x7B); // All Notes Off
    }
}
}
