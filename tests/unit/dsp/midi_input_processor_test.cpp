// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_input_processor.h"
#include "dsp/midi_port.h"
#include "utils/object_registry.h"

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
  }

  void TearDown () override { processor_->release_resources (); }

  void setup_processor_with_buffer (const juce::MidiBuffer &buffer)
  {
    current_buffer_ = buffer;

    auto provider = [this] () -> const juce::MidiBuffer & {
      return current_buffer_;
    };

    processor_ = std::make_unique<MidiInputProcessor> (provider, *registry_);
    processor_->prepare_for_processing (
      nullptr, sample_rate_, max_block_length_);
  }

  std::unique_ptr<utils::ObjectRegistry>     registry_;
  units::sample_rate_t                       sample_rate_;
  units::sample_u32_t                        max_block_length_;
  std::unique_ptr<graph_test::MockTransport> mock_transport_;
  std::unique_ptr<dsp::TempoMap>             tempo_map_;
  std::unique_ptr<MidiInputProcessor>        processor_;
  juce::MidiBuffer                           current_buffer_;
};

// ========================================================================
// Construction
// ========================================================================

TEST_F (MidiInputProcessorTest, CreatesSingleOutputPort)
{
  juce::MidiBuffer empty;
  auto provider = [&empty] () -> const juce::MidiBuffer & { return empty; };
  processor_ = std::make_unique<MidiInputProcessor> (provider, *registry_);
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  const auto &outputs = processor_->get_output_ports ();
  EXPECT_EQ (outputs.size (), 1);
}

TEST_F (MidiInputProcessorTest, OutputPortIsMidiPort)
{
  juce::MidiBuffer empty;
  auto provider = [&empty] () -> const juce::MidiBuffer & { return empty; };
  processor_ = std::make_unique<MidiInputProcessor> (provider, *registry_);
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.type (), PortType::Midi);
  EXPECT_EQ (port.flow (), PortFlow::Output);
}

// ========================================================================
// Process block — empty buffer
// ========================================================================

TEST_F (MidiInputProcessorTest, ProcessBlockWithEmptyBufferIsNoOp)
{
  juce::MidiBuffer empty_buffer;
  setup_processor_with_buffer (empty_buffer);

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
  juce::MidiBuffer buffer;
  buffer.addEvent (juce::MidiMessage::noteOn (1, 60, (uint8_t) 100), 0);

  setup_processor_with_buffer (buffer);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.midi_events_.queued_events_.size (), 1);
}

TEST_F (MidiInputProcessorTest, ProcessBlockCopiesMultipleEvents)
{
  juce::MidiBuffer buffer;
  buffer.addEvent (juce::MidiMessage::noteOn (1, 60, (uint8_t) 100), 0);
  buffer.addEvent (juce::MidiMessage::noteOn (1, 64, (uint8_t) 80), 10);
  buffer.addEvent (juce::MidiMessage::controllerEvent (1, 74, 64), 20);

  setup_processor_with_buffer (buffer);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.midi_events_.queued_events_.size (), 3);
}

TEST_F (MidiInputProcessorTest, ProcessBlockCopiesPitchBend)
{
  juce::MidiBuffer buffer;
  buffer.addEvent (juce::MidiMessage::pitchWheel (1, 8192), 0);

  setup_processor_with_buffer (buffer);

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
  auto provider = [this] () -> const juce::MidiBuffer & {
    return current_buffer_;
  };

  processor_ = std::make_unique<MidiInputProcessor> (provider, *registry_);
  processor_->prepare_for_processing (nullptr, sample_rate_, max_block_length_);

  auto time_nfo = dsp::graph::ProcessBlockInfo::from_position_and_nframes (
    units::samples (0), max_block_length_);

  juce::MidiBuffer buf1;
  buf1.addEvent (juce::MidiMessage::noteOn (1, 60, (uint8_t) 100), 0);
  current_buffer_ = buf1;
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  juce::MidiBuffer buf2;
  buf2.addEvent (juce::MidiMessage::noteOn (1, 64, (uint8_t) 80), 0);
  current_buffer_ = buf2;
  processor_->process_block (time_nfo, *mock_transport_, *tempo_map_);

  auto &port = processor_->get_output_port ();
  EXPECT_EQ (port.midi_events_.queued_events_.size (), 2);
}

// ========================================================================
// get_output_port
// ========================================================================

TEST_F (MidiInputProcessorTest, GetOutputPortReturnsCorrectPort)
{
  juce::MidiBuffer empty;
  auto provider = [&empty] () -> const juce::MidiBuffer & { return empty; };
  processor_ = std::make_unique<MidiInputProcessor> (provider, *registry_);

  auto &port = processor_->get_output_port ();
  EXPECT_TRUE (port.get_label ().view ().contains ("MIDI Input"));
}

} // namespace zrythm::dsp
