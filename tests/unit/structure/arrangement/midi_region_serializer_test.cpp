// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "structure/arrangement/midi_region_serializer.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class MidiRegionSerializerTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    region = std::make_unique<MidiRegion> (
      *tempo_map, registry, file_audio_source_registry, nullptr);

    // Set up region properties
    region->position ()->setTicks (100);
    region->regionMixin ()->bounds ()->length ()->setTicks (200);
    region->regionMixin ()->loopRange ()->loopStartPosition ()->setTicks (50);
    region->regionMixin ()->loopRange ()->loopEndPosition ()->setTicks (150);
    region->regionMixin ()->loopRange ()->clipStartPosition ()->setTicks (0);
  }

  void add_midi_note (
    int    pitch,
    int    velocity,
    double position_ticks,
    double length_ticks)
  {
    // Create MidiNote using registry
    auto note_ref = registry.create_object<MidiNote> (*tempo_map);
    note_ref.get_object_as<MidiNote> ()->setPitch (pitch);
    note_ref.get_object_as<MidiNote> ()->setVelocity (velocity);
    note_ref.get_object_as<MidiNote> ()->position ()->setTicks (position_ticks);
    note_ref.get_object_as<MidiNote> ()->bounds ()->length ()->setTicks (
      length_ticks);

    // Add to region
    region->add_object (note_ref);
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  ArrangerObjectRegistry         registry;
  dsp::FileAudioSourceRegistry   file_audio_source_registry;
  std::unique_ptr<MidiRegion>    region;
};

TEST_F (MidiRegionSerializerTest, SerializeEventsSimple)
{
  // Add a note within the region
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  MidiRegionSerializer::serialize_to_sequence (
    *region, events, std::nullopt, std::nullopt, true, false);

  // Verify events
  EXPECT_EQ (events.getNumEvents (), 2); // Note on + note off

  // First event should be note on
  auto note_on_event = events.getEventPointer (0);
  EXPECT_TRUE (note_on_event->message.isNoteOn ());
  EXPECT_EQ (note_on_event->message.getNoteNumber (), 60);
  EXPECT_EQ (note_on_event->message.getVelocity (), 90);
  EXPECT_EQ (note_on_event->message.getTimeStamp (), 200); // 100 (position) +
                                                           // 100 (region start)

  // Second event should be note off
  auto note_off_event = events.getEventPointer (1);
  EXPECT_TRUE (note_off_event->message.isNoteOff ());
  EXPECT_EQ (note_off_event->message.getNoteNumber (), 60);
  EXPECT_EQ (note_off_event->message.getVelocity (), 90);
  EXPECT_EQ (
    note_off_event->message.getTimeStamp (),
    250); // 100 (position) + 50 (length) + 100 (region start)
}

TEST_F (MidiRegionSerializerTest, SerializeEventsWithLooping)
{
  // Add a note within the loop range
  add_midi_note (60, 90, 50, 50);

  juce::MidiMessageSequence events;
  MidiRegionSerializer::serialize_to_sequence (
    *region, events, std::nullopt, std::nullopt, true, true);

  // Should create multiple events due to looping
  // Loop length = 100 ticks (150-50)
  // Region length = 200 ticks
  // Number of repeats = ceil((200 - 50 + 0) / 100) = ceil(150/100) = 2
  EXPECT_EQ (events.getNumEvents (), 4); // Two sets of note on/off

  // First occurrence
  auto first_note_on = events.getEventPointer (0);
  EXPECT_TRUE (first_note_on->message.isNoteOn ());
  EXPECT_EQ (
    first_note_on->message.getTimeStamp (), 150); // 50 + 100 region start

  // Second occurrence (loop repeat)
  auto second_note_on = events.getEventPointer (2);
  EXPECT_TRUE (second_note_on->message.isNoteOn ());
  EXPECT_EQ (
    second_note_on->message.getTimeStamp (),
    250); // 150 (first loop end) + 100 region start
}

TEST_F (MidiRegionSerializerTest, SerializeEventsWithStartEndConstraints)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  juce::MidiMessageSequence events;
  MidiRegionSerializer::serialize_to_sequence (
    *region, events, 150.0, 250.0, true, false);

  // Verify events
  EXPECT_EQ (events.getNumEvents (), 2); // Note on and note off

  auto note_on_event = events.getEventPointer (0);
  EXPECT_TRUE (note_on_event->message.isNoteOn ());
  EXPECT_EQ (
    note_on_event->message.getTimeStamp (), 50); // 200 - 150 (start constraint)

  auto note_off_event = events.getEventPointer (1);
  EXPECT_TRUE (note_off_event->message.isNoteOff ());
  EXPECT_EQ (note_off_event->message.getTimeStamp (), 100); // 250 - 150 (start
                                                            // constraint)
}

TEST_F (MidiRegionSerializerTest, MutedNoteNotSerialized)
{
  // Add a note and mute it
  add_midi_note (60, 90, 100, 50);
  region->get_children_view ()[0]->mute ()->setMuted (true);

  juce::MidiMessageSequence events;
  MidiRegionSerializer::serialize_to_sequence (
    *region, events, std::nullopt, std::nullopt, true, true);

  // No events should be added for muted note
  EXPECT_EQ (events.getNumEvents (), 0);
}

TEST_F (MidiRegionSerializerTest, NoteOutsideLoopRangeNotSerialized)
{
  // Adjust clip start
  region->regionMixin ()->loopRange ()->clipStartPosition ()->setTicks (50);

  // Add note before clip start and loop range
  add_midi_note (60, 90, 40, 10); // At 40-50 ticks, loop starts at 50

  juce::MidiMessageSequence events;
  MidiRegionSerializer::serialize_to_sequence (
    *region, events, std::nullopt, std::nullopt, true, true);

  // Should not be added when full=true
  EXPECT_EQ (events.getNumEvents (), 0);
}

} // namespace zrythm::structure::arrangement
