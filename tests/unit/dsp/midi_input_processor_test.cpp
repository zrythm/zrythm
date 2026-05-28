// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_device_buffer.h"
#include "dsp/midi_input_processor.h"
#include "dsp/midi_port.h"
#include "utils/object_registry.h"
#include "utils/registry_utils.h"

#include "unit/dsp/graph_helpers.h"
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class MidiInputProcessorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    registry_ = std::make_unique<utils::ObjectRegistry> ();
    sample_rate_ = units::sample_rate (48000);
    max_block_length_ = units::samples (256);
    mock_transport_ = std::make_unique<graph_test::MockTransport> ();
    tempo_map_ = std::make_unique<dsp::TempoMap> (sample_rate_);
    device_buffer_ = std::make_shared<MidiDeviceBuffer> ();
  }

  void TearDown () override { processor_->release_resources (); }

  void create_processor ()
  {
    processor_ =
      std::make_unique<MidiInputProcessor> (device_buffer_, *registry_);
    processor_->prepare_for_processing (
      nullptr, sample_rate_, max_block_length_);
  }

  void push_event (const juce::MidiMessage &msg) { device_buffer_->push (msg); }

  std::unique_ptr<utils::ObjectRegistry>     registry_;
  units::sample_rate_t                       sample_rate_;
  units::sample_u32_t                        max_block_length_;
  std::unique_ptr<graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>             tempo_map_;
  std::shared_ptr<MidiDeviceBuffer>          device_buffer_;
  std::unique_ptr<MidiInputProcessor>        processor_;
};

// ========================================================================
// Construction
// ========================================================================

TEST_F (MidiInputProcessorTest, CreatesSingleOutputPort)
{
  create_processor ();
  const auto &outputs = processor_->get_output_ports ();
  EXPECT_EQ (outputs.size (), 1);
}

TEST_F (MidiInputProcessorTest, OutputPortIsMidiPort)
{
  create_processor ();
  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.type (), PortType::Midi);
  EXPECT_EQ (port.flow (), PortFlow::Output);
}

// ========================================================================
// Process block — empty buffer
// ========================================================================

TEST_F (MidiInputProcessorTest, ProcessBlockWithEmptyBufferIsNoOp)
{
  create_processor ();
  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  EXPECT_TRUE (port.midi_events_.queued_events_.empty ());
}

// ========================================================================
// Process block — event copying
// ========================================================================

TEST_F (MidiInputProcessorTest, ProcessBlockCopiesNoteOnEvents)
{
  auto msg = juce::MidiMessage::noteOn (1, 60, (uint8_t) 100);
  msg.setTimeStamp (1.0);
  push_event (msg);

  create_processor ();

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  ASSERT_EQ (port.midi_events_.queued_events_.size (), 1);
  EXPECT_EQ (port.midi_events_.queued_events_.at (0).raw_buffer_[0], 0x90);
}

TEST_F (MidiInputProcessorTest, ProcessBlockCopiesMultipleEvents)
{
  auto msg1 = juce::MidiMessage::noteOn (1, 60, (uint8_t) 100);
  msg1.setTimeStamp (1.0);
  push_event (msg1);

  auto msg2 = juce::MidiMessage::noteOn (1, 64, (uint8_t) 80);
  msg2.setTimeStamp (1.0 + 10.0 / 48000.0);
  push_event (msg2);

  auto msg3 = juce::MidiMessage::controllerEvent (1, 74, 64);
  msg3.setTimeStamp (1.0 + 20.0 / 48000.0);
  push_event (msg3);

  create_processor ();

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.midi_events_.queued_events_.size (), 3);
}

TEST_F (MidiInputProcessorTest, ProcessBlockCopiesPitchBend)
{
  auto msg = juce::MidiMessage::pitchWheel (1, 8192);
  msg.setTimeStamp (1.0);
  push_event (msg);

  create_processor ();

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.midi_events_.queued_events_.size (), 1);
}

// ========================================================================
// Process block — accumulates across calls
// ========================================================================

TEST_F (MidiInputProcessorTest, ProcessBlockAccumulatesAcrossCalls)
{
  create_processor ();

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);

  auto msg1 = juce::MidiMessage::noteOn (1, 60, (uint8_t) 100);
  msg1.setTimeStamp (1.0);
  push_event (msg1);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto msg2 = juce::MidiMessage::noteOn (1, 64, (uint8_t) 80);
  msg2.setTimeStamp (2.0);
  push_event (msg2);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.midi_events_.queued_events_.size (), 2);
}

// ========================================================================
// get_output_port
// ========================================================================

TEST_F (MidiInputProcessorTest, GetOutputPortReturnsCorrectPort)
{
  create_processor ();
  auto &port = processor_->get_output_port ();
  EXPECT_TRUE (port.get_label ().view ().contains ("MIDI Input"));
}

} // namespace zrythm::dsp
