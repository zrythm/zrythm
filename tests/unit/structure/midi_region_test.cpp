// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "structure/arrangement/arranger_object_all.h"

#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class MidiRegionTest : public ::testing::Test
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

TEST_F (MidiRegionTest, InitialState)
{
  EXPECT_EQ (region->type (), ArrangerObject::Type::MidiRegion);
  EXPECT_EQ (region->position ()->ticks (), 100);
  EXPECT_NE (region->regionMixin (), nullptr);
  EXPECT_EQ (region->get_children_vector ().size (), 0);
}

TEST_F (MidiRegionTest, AddEventsSimple)
{
  // Add a note within the region
  add_midi_note (60, 90, 100, 50);

  dsp::MidiEventVector events;
  region->add_midi_region_events (
    events, std::nullopt, std::nullopt, true, false);

  // Verify events
  EXPECT_EQ (events.size (), 2);                         // Note on + note off
  EXPECT_EQ (events.at (0).raw_buffer_[0] & 0xF0, 0x90); // Note on status byte
  EXPECT_EQ (events.at (0).raw_buffer_[1], 60);          // Pitch
  EXPECT_EQ (events.at (0).raw_buffer_[2], 90);          // Velocity
  EXPECT_EQ (events.at (0).time_, 200); // 100 (position) + 100 (region start)

  EXPECT_EQ (events.at (1).raw_buffer_[0] & 0xF0, 0x80); // Note off status byte
  EXPECT_EQ (events.at (1).raw_buffer_[1], 60);          // Pitch
  EXPECT_EQ (events.at (1).time_, 250); // 100 (position) + 50 (length) + 100
                                        // (region start)
}

TEST_F (MidiRegionTest, AddEventsWithLooping)
{
  // Add a note within the loop range
  add_midi_note (60, 90, 50, 50);

  dsp::MidiEventVector events;
  region->add_midi_region_events (
    events, std::nullopt, std::nullopt, true, true);

  // Should create multiple events due to looping
  // Loop length = 100 ticks (150-50)
  // Region length = 200 ticks
  // Number of repeats = ceil((200 - 50 + 0) / 100) = ceil(150/100) = 2
  EXPECT_EQ (events.size (), 4); // Two sets of note on/off

  // First occurrence
  EXPECT_EQ (events.at (0).raw_buffer_[0] & 0xF0, 0x90); // Note on
  EXPECT_EQ (events.at (0).time_, 150); // 50 + 100 region start

  // Second occurrence (loop repeat)
  EXPECT_EQ (events.at (2).raw_buffer_[0] & 0xF0, 0x90); // Note on
  EXPECT_EQ (
    events.at (2).time_, 250); // 150 (first loop end) + 100 region start
}

TEST_F (MidiRegionTest, AddEventsWithStartEndConstraints)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);

  dsp::MidiEventVector events;
  region->add_midi_region_events (events, 150.0, 250.0, true, false);

  // Verify events
  EXPECT_EQ (events.size (), 2);                         // Note on and note off
  EXPECT_EQ (events.at (0).raw_buffer_[0] & 0xF0, 0x90); // Note on
  EXPECT_EQ (events.at (0).time_, 50); // 200 - 150 (start constraint)
  EXPECT_EQ (events.at (1).raw_buffer_[0] & 0xF0, 0x80); // Note off
  EXPECT_EQ (events.at (1).time_, 100); // 250 - 150 (start constraint)
}

TEST_F (MidiRegionTest, MutedNoteNotAdded)
{
  // Add a note and mute it
  add_midi_note (60, 90, 100, 50);
  region->get_children_view ()[0]->mute ()->setMuted (true);

  dsp::MidiEventVector events;
  region->add_midi_region_events (
    events, std::nullopt, std::nullopt, true, true);

  // No events should be added for muted note
  EXPECT_EQ (events.size (), 0);
}

TEST_F (MidiRegionTest, NoteOutsideLoopRangeNotAdded)
{
  // Adjust clip start
  region->regionMixin ()->loopRange ()->clipStartPosition ()->setTicks (50);

  // Add note before clip start and loop range
  add_midi_note (60, 90, 40, 10); // At 40-50 ticks, loop starts at 50

  dsp::MidiEventVector events;
  region->add_midi_region_events (
    events, std::nullopt, std::nullopt, true, true);

  // Should not be added when full=true
  EXPECT_EQ (events.size (), 0);
}

TEST_F (MidiRegionTest, Serialization)
{
  // Add a note
  add_midi_note (60, 90, 100, 50);
  const auto midi_note_id = region->get_children_view ()[0]->get_uuid ();

  // Serialize
  nlohmann::json j;
  to_json (j, *region);

  // Create new region
  auto new_region = std::make_unique<MidiRegion> (
    *tempo_map, registry, file_audio_source_registry, nullptr);
  from_json (j, *new_region);

  // Verify deserialization
  EXPECT_EQ (new_region->get_children_vector ().size (), 1);
  auto note = new_region->get_children_view ()[0];
  EXPECT_EQ (note->pitch (), 60);
  EXPECT_EQ (note->velocity (), 90);
  EXPECT_EQ (midi_note_id, note->get_uuid ());
}

} // namespace zrythm::structure::arrangement
