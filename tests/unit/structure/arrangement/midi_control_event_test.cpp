// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/midi_control_event.h"
#include "utils/object_registry.h"

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::arrangement
{

class MidiControlEventTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map_ = std::make_unique<dsp::TempoMap> (units::sample_rate (44100.0));
    tempo_map_wrapper_ = std::make_unique<dsp::TempoMapWrapper> (*tempo_map_);
  }

  std::unique_ptr<dsp::TempoMap>        tempo_map_;
  std::unique_ptr<dsp::TempoMapWrapper> tempo_map_wrapper_;
  test_helpers::ScopedQCoreApplication  app_;
};

TEST_F (MidiControlEventTest, Construction)
{
  MidiControlEvent ev (*tempo_map_wrapper_);
  EXPECT_EQ (ev.type (), ArrangerObject::Type::MidiControlEvent);
  EXPECT_EQ (ev.controlType (), MidiControlEvent::EventType::ControlChange);
  EXPECT_EQ (ev.channel (), 0);
  EXPECT_EQ (ev.controller (), 0);
  EXPECT_EQ (ev.value (), 0);
}

TEST_F (MidiControlEventTest, PropertiesCC)
{
  MidiControlEvent ev (*tempo_map_wrapper_);
  ev.setControlType (MidiControlEvent::EventType::ControlChange);
  ev.setChannel (2);
  ev.setController (74);
  ev.setValue (100);

  EXPECT_EQ (ev.controlType (), MidiControlEvent::EventType::ControlChange);
  EXPECT_EQ (ev.midiChannel (), 2);
  EXPECT_EQ (ev.midiController (), 74);
  EXPECT_EQ (ev.midiValue (), 100);
}

TEST_F (MidiControlEventTest, PropertiesPitchBend)
{
  MidiControlEvent ev (*tempo_map_wrapper_);
  ev.setControlType (MidiControlEvent::EventType::PitchBend);
  ev.setChannel (0);
  ev.setValue (8192);

  EXPECT_EQ (ev.controlType (), MidiControlEvent::EventType::PitchBend);
  EXPECT_EQ (ev.midiValue (), 8192);
}

TEST_F (MidiControlEventTest, ValueClampedForType)
{
  MidiControlEvent ev (*tempo_map_wrapper_);
  ev.setControlType (MidiControlEvent::EventType::ControlChange);
  ev.setValue (200);
  EXPECT_EQ (ev.midiValue (), 127);

  ev.setControlType (MidiControlEvent::EventType::PitchBend);
  ev.setValue (8192);
  EXPECT_EQ (ev.midiValue (), 8192);
}

TEST_F (MidiControlEventTest, TypeChangeClampsValue)
{
  MidiControlEvent ev (*tempo_map_wrapper_);
  ev.setControlType (MidiControlEvent::EventType::PitchBend);
  ev.setValue (8192);

  ev.setControlType (MidiControlEvent::EventType::ControlChange);
  EXPECT_EQ (ev.midiValue (), 127);
}

TEST_F (MidiControlEventTest, ChannelClamped)
{
  MidiControlEvent ev (*tempo_map_wrapper_);
  ev.setChannel (20);
  EXPECT_EQ (ev.midiChannel (), 15);
}

TEST_F (MidiControlEventTest, ControllerClamped)
{
  MidiControlEvent ev (*tempo_map_wrapper_);
  ev.setController (200);
  EXPECT_EQ (ev.midiController (), 127);
}

TEST_F (MidiControlEventTest, SerializationRoundTrip)
{
  MidiControlEvent original (*tempo_map_wrapper_);
  original.position ()->setTicks (100.0);
  original.setControlType (MidiControlEvent::EventType::ChannelPressure);
  original.setChannel (3);
  original.setValue (64);

  nlohmann::json j;
  to_json (j, original);

  MidiControlEvent deserialized (*tempo_map_wrapper_);
  from_json (j, deserialized);

  EXPECT_EQ (
    deserialized.controlType (), MidiControlEvent::EventType::ChannelPressure);
  EXPECT_EQ (deserialized.midiChannel (), 3);
  EXPECT_EQ (deserialized.midiValue (), 64);
  EXPECT_EQ (deserialized.position ()->ticks (), 100.0);
}

}
