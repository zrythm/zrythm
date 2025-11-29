// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "utils/icloneable.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{
static_assert (fmt::is_formattable<MidiPort> ());

class MidiPortTest : public ::testing::Test
{
protected:
  static constexpr auto      SAMPLE_RATE = units::sample_rate (44100);
  static constexpr nframes_t BLOCK_LENGTH = 256;

  void SetUp () override
  {
    // Create input and output MIDI ports
    input_port = std::make_unique<MidiPort> (
      utils::Utf8String (u8"MIDI In"), PortFlow::Input);
    output_port = std::make_unique<MidiPort> (
      utils::Utf8String (u8"MIDI Out"), PortFlow::Output);

    // Prepare for processing
    input_port->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);
    output_port->prepare_for_processing (nullptr, SAMPLE_RATE, BLOCK_LENGTH);

    // Set up mock transport
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
  }

  std::unique_ptr<MidiPort>                  input_port;
  std::unique_ptr<MidiPort>                  output_port;
  std::unique_ptr<graph_test::MockTransport> mock_transport_;
};

TEST_F (MidiPortTest, BasicProperties)
{
  EXPECT_TRUE (input_port->is_midi ());
  EXPECT_TRUE (input_port->is_input ());
  EXPECT_FALSE (input_port->is_output ());

  EXPECT_TRUE (output_port->is_midi ());
  EXPECT_TRUE (output_port->is_output ());
  EXPECT_FALSE (output_port->is_input ());
}

TEST_F (MidiPortTest, MidiEventHandling)
{
  // Add event to input port
  input_port->midi_events_.queued_events_.add_note_on (1, 60, 100, 0);

  // Process input port
  EngineProcessTimeInfo time_info{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  input_port->process_block (time_info, *mock_transport_);

  // Verify event was processed
  EXPECT_TRUE (input_port->midi_events_.queued_events_.empty ());
  EXPECT_FALSE (input_port->midi_events_.active_events_.empty ());
}

TEST_F (MidiPortTest, RingBufferFunctionality)
{
  // Request ring buffer functionality
  RingBufferOwningPortMixin::RingBufferReader reader (*output_port);

  // Add event to output port
  output_port->midi_events_.queued_events_.add_control_change (2, 1, 127, 20);

  // Process output port
  EngineProcessTimeInfo time_info{
    .g_start_frame_ = 0,
    .g_start_frame_w_offset_ = 0,
    .local_offset_ = 0,
    .nframes_ = BLOCK_LENGTH
  };
  output_port->process_block (time_info, *mock_transport_);

  // Verify event was added to ring buffer
  EXPECT_NE (output_port->midi_ring_, nullptr);

  // Read from ring buffer
  MidiEvent read_event;
  EXPECT_TRUE (output_port->midi_ring_->read (read_event));

  // Verify event data
  EXPECT_TRUE (utils::midi::midi_is_controller (read_event.raw_buffer_));
  EXPECT_EQ (utils::midi::midi_get_channel_1_to_16 (read_event.raw_buffer_), 2);
  EXPECT_EQ (
    utils::midi::midi_get_controller_number (read_event.raw_buffer_), 1);
  EXPECT_EQ (
    utils::midi::midi_get_controller_value (read_event.raw_buffer_), 127);
  EXPECT_EQ (read_event.time_, 20);
}

TEST_F (MidiPortTest, ResourceManagement)
{
  // Verify resources were allocated
  EXPECT_NE (input_port->midi_ring_, nullptr);
  EXPECT_NE (output_port->midi_ring_, nullptr);

  // Release resources
  input_port->release_resources ();
  output_port->release_resources ();

  // Verify resources were released
  EXPECT_EQ (input_port->midi_ring_, nullptr);
  EXPECT_EQ (output_port->midi_ring_, nullptr);
}

} // namespace zrythm::dsp
