// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/port.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::dsp
{

class TestPort : public Port
{
public:
  TestPort (utils::Utf8String label, PortType type, PortFlow flow)
      : Port (std::move (label), type, flow)
  {
  }

  void clear_buffer (std::size_t) override { }
  void process_block (EngineProcessTimeInfo) override { }
};

class PortTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    audio_in = std::make_unique<TestPort> (
      u8"AudioIn", PortType::Audio, PortFlow::Input);
    cv_out = std::make_unique<TestPort> (
      u8"ControlOut", PortType::CV, PortFlow::Output);
    midi_in =
      std::make_unique<TestPort> (u8"MIDIIn", PortType::Midi, PortFlow::Input);
  }

  std::unique_ptr<TestPort> audio_in;
  std::unique_ptr<TestPort> cv_out;
  std::unique_ptr<TestPort> midi_in;
};

TEST_F (PortTest, BasicProperties)
{
  EXPECT_EQ (audio_in->get_label (), "AudioIn");
  EXPECT_TRUE (audio_in->is_audio ());
  EXPECT_TRUE (audio_in->is_input ());
}

TEST_F (PortTest, OwnerInteraction)
{
  audio_in->set_full_designation_provider ([] (const auto &) {
    return utils::Utf8String (u8"Track1/AudioIn");
  });
  EXPECT_EQ (audio_in->get_full_designation (), "Track1/AudioIn");
}

TEST_F (PortTest, TypeDetection)
{
  EXPECT_FALSE (cv_out->is_audio ());
  EXPECT_TRUE (cv_out->is_output ());
  EXPECT_TRUE (midi_in->is_midi ());
}

TEST_F (PortTest, UuidUniqueness)
{
  TestPort port1{ u8"label", PortType::Audio, PortFlow::Input };
  TestPort port2{ u8"label", PortType::Audio, PortFlow::Input };
  EXPECT_NE (port1.get_uuid (), port2.get_uuid ());
}

} // namespace zrythm::dsp
