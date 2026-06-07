// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "utils/format_qt.h"
#include <fmt/std.h>

#include "dsp/midi_event.h"
#include "dsp/midi_port.h"
#include "utils/format_boost.h"
#include "utils/icloneable.h"
#include "utils/logger.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{
static_assert (fmt::formattable<MidiPort>);

class MidiPortTest : public ::testing::Test
{
protected:
  static constexpr auto SAMPLE_RATE = units::sample_rate (44100);
  static constexpr auto BLOCK_LENGTH = units::samples (256);

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
    tempo_map_ = std::make_unique<dsp::TempoMap> (SAMPLE_RATE);
  }

  std::unique_ptr<MidiPort>                  input_port;
  std::unique_ptr<MidiPort>                  output_port;
  std::unique_ptr<graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>             tempo_map_;
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

TEST_F (MidiPortTest, InputPortClearsOnProcessWithNoSources)
{
  // Add event to input port
  auto ev = dsp::midi_event::make_note_on (0, 60, 100, units::samples (0));
  input_port->buffer_.push_back (ev.time_, ev.data ());

  // Process input port (no sources connected)
  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), BLOCK_LENGTH);
  input_port->process_block (time_info, *mock_transport_, *tempo_map_);

  // Input port clears and aggregates from sources — with no sources, buffer
  // is empty
  EXPECT_TRUE (input_port->buffer_.empty ());
}

TEST_F (MidiPortTest, ResourceManagement)
{
  // Release resources
  input_port->release_resources ();
  output_port->release_resources ();
}

TEST_F (MidiPortTest, OutputPortPreservesEventsThroughProcessBlock)
{
  auto ev = dsp::midi_event::make_note_on (0, 60, 100, units::samples (10));
  output_port->buffer_.push_back (ev.time_, ev.data ());

  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), BLOCK_LENGTH);
  output_port->process_block (time_info, *mock_transport_, *tempo_map_);

  ASSERT_EQ (output_port->buffer_.size (), 1);
  auto out_data = (*output_port->buffer_.begin ()).data ();
  auto ev_data = ev.data ();
  ASSERT_EQ (out_data.size (), ev_data.size ());
  EXPECT_TRUE (std::ranges::equal (out_data, ev_data));
}

TEST_F (MidiPortTest, InputPortClearsAndAggregatesFromSources)
{
  // Set up: output_port feeds into input_port
  input_port->set_port_sources (std::array<MidiPort *, 1>{ output_port.get () });

  auto ev = dsp::midi_event::make_note_on (0, 64, 80, units::samples (5));
  output_port->buffer_.push_back (ev.time_, ev.data ());

  auto time_info = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), BLOCK_LENGTH);
  input_port->process_block (time_info, *mock_transport_, *tempo_map_);

  ASSERT_EQ (input_port->buffer_.size (), 1);
  auto out_data = (*input_port->buffer_.begin ()).data ();
  auto ev_data = ev.data ();
  ASSERT_EQ (out_data.size (), ev_data.size ());
  EXPECT_TRUE (std::ranges::equal (out_data, ev_data));
}

} // namespace zrythm::dsp
