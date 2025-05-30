// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <thread>

#include "dsp/chord_descriptor.h"
#include "dsp/midi_event.h"
#include "utils/gtest_wrapper.h"

namespace zrythm::dsp
{

class MidiEventTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create test chord descriptors
    chord_c_major_ = ChordDescriptor (
      MusicalNote::C, false, MusicalNote::C, ChordType::Major,
      ChordAccent::None, 0);
    chord_a_minor_ = ChordDescriptor (
      MusicalNote::A, false, MusicalNote::A, ChordType::Minor,
      ChordAccent::None, 0);
  }

  static MidiEvent create_note_on (
    midi_byte_t channel,
    midi_byte_t note,
    midi_byte_t velocity,
    midi_time_t time)
  {
    return MidiEvent (
      (midi_byte_t) (utils::midi::MIDI_CH1_NOTE_ON | (channel - 1)), note,
      velocity, time);
  }

  static MidiEvent
  create_note_off (midi_byte_t channel, midi_byte_t note, midi_time_t time)
  {
    return MidiEvent (
      (midi_byte_t) (utils::midi::MIDI_CH1_NOTE_OFF | (channel - 1)), note, 0,
      time);
  }

  ChordDescriptor chord_c_major_;
  ChordDescriptor chord_a_minor_;
};

TEST_F (MidiEventTest, EventConstruction)
{
  MidiEvent ev (0x90, 60, 100, 42);
  EXPECT_EQ (ev.raw_buffer_[0], 0x90);
  EXPECT_EQ (ev.raw_buffer_[1], 60);
  EXPECT_EQ (ev.raw_buffer_[2], 100);
  EXPECT_EQ (ev.time_, 42);
}

TEST_F (MidiEventTest, EventComparison)
{
  MidiEvent ev1 (0x90, 60, 100, 42);
  MidiEvent ev2 (0x90, 60, 100, 42);
  MidiEvent ev3 (0x80, 60, 0, 42);

  EXPECT_TRUE (ev1 == ev2);
  EXPECT_FALSE (ev1 == ev3);
}

TEST_F (MidiEventTest, VectorBasicOperations)
{
  MidiEventVector vec;
  EXPECT_TRUE (vec.empty ());

  // Add events
  vec.push_back (create_note_on (1, 60, 100, 0));
  vec.push_back (create_note_on (1, 62, 90, 10));
  EXPECT_EQ (vec.size (), 2);

  // Test iteration
  int count = 0;
  vec.foreach_event ([&count] (const MidiEvent &) { count++; });
  EXPECT_EQ (count, 2);

  // Test removal
  MidiEvent ev = vec.front ();
  vec.remove (ev);
  EXPECT_EQ (vec.size (), 1);
  EXPECT_EQ (vec.front ().raw_buffer_[1], 62); // Remaining note
}

TEST_F (MidiEventTest, ThreadSafety)
{
  MidiEventVector vec;
  constexpr int   NUM_THREADS = 4;
  constexpr int   EVENTS_PER_THREAD = 100;

  auto worker = [&vec] {
    for (int i = 0; i < EVENTS_PER_THREAD; ++i)
      {
        vec.push_back (create_note_on (1, 60 + i, 100, i));
      }
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < NUM_THREADS; ++i)
    {
      threads.emplace_back (worker);
    }

  for (auto &t : threads)
    {
      t.join ();
    }

  EXPECT_EQ (vec.size (), NUM_THREADS * EVENTS_PER_THREAD);
}

TEST_F (MidiEventTest, EventFiltering)
{
  MidiEventVector src;
  src.push_back (create_note_on (1, 60, 100, 5)); // Channel 1
  src.push_back (create_note_on (2, 62, 90, 15)); // Channel 2
  src.push_back (create_note_on (1, 64, 80, 25)); // Channel 1

  MidiEventVector      dest;
  std::array<bool, 16> channels{};
  channels[0] = true; // Only allow channel 1

  dest.append_w_filter (src, channels, 0, 30);
  EXPECT_EQ (dest.size (), 2);
  EXPECT_EQ (dest.at (0).raw_buffer_[0] & 0x0F, 0); // Channel 1
  EXPECT_EQ (dest.at (1).raw_buffer_[0] & 0x0F, 0); // Channel 1
}

TEST_F (MidiEventTest, ChordTransformation)
{
  MidiEventVector src;
  src.push_back (create_note_on (1, 48, 100, 0)); // C3 (root for C major)
  src.push_back (create_note_off (1, 48, 10));

  MidiEventVector dest;
  auto chord_mapper = [this] (midi_byte_t note) -> const ChordDescriptor * {
    return (note == 48) ? &chord_c_major_ : nullptr;
  };

  dest.transform_chord_and_append (src, chord_mapper, 100, 0, 20);

  // C major chord should have 3 notes (C, E, G)
  EXPECT_EQ (dest.size (), 6); // 3 note-ons + 3 note-offs
}

TEST_F (MidiEventTest, EventSorting)
{
  MidiEventVector vec;
  vec.push_back (create_note_on (1, 60, 100, 20));
  vec.push_back (create_note_off (1, 60, 10));
  vec.push_back (create_note_on (1, 62, 90, 5));

  vec.sort ();

  // Should be sorted by time: 5, 10, 20
  EXPECT_EQ (vec.at (0).time_, 5);
  EXPECT_EQ (vec.at (1).time_, 10);
  EXPECT_EQ (vec.at (2).time_, 20);

  // Verify note-on has precedence at same time
  MidiEvent same_time_note_off = create_note_off (1, 64, 10);
  MidiEvent same_time_note_on = create_note_on (1, 64, 80, 10);
  vec.push_back (same_time_note_off);
  vec.push_back (same_time_note_on);
  vec.sort ();

  // Note-on should come before note-off at same time
  EXPECT_TRUE (utils::midi::midi_is_note_on (vec.at (1).raw_buffer_));
  EXPECT_TRUE (utils::midi::midi_is_note_off (vec.at (2).raw_buffer_));
}

TEST_F (MidiEventTest, ChannelSetting)
{
  MidiEventVector vec;
  vec.push_back (create_note_on (1, 60, 100, 0));
  vec.push_back (create_note_on (2, 62, 90, 0));

  vec.set_channel (3);

  for (const auto &ev : vec)
    {
      EXPECT_EQ (ev.raw_buffer_[0] & 0x0F, 2); // Channel 3 (zero-indexed 2)
    }
}

TEST_F (MidiEventTest, PanicFunction)
{
  MidiEventVector vec;
  vec.push_back (create_note_on (1, 60, 100, 0));
  vec.push_back (create_note_on (2, 62, 90, 0));

  vec.panic ();

  // Should have all-notes-off for all channels
  EXPECT_GE (vec.size (), 16);
  bool found_all_notes_off = false;
  vec.foreach_event ([&found_all_notes_off] (const MidiEvent &ev) {
    if (utils::midi::midi_is_all_notes_off (ev.raw_buffer_))
      found_all_notes_off = true;
  });
  EXPECT_TRUE (found_all_notes_off);
}

TEST_F (MidiEventTest, ClearDuplicates)
{
  MidiEventVector vec;
  MidiEvent       ev1 = create_note_on (1, 60, 100, 0);
  MidiEvent       ev2 = create_note_on (1, 60, 100, 0); // Duplicate

  vec.push_back (ev1);
  vec.push_back (ev2);
  vec.push_back (create_note_on (1, 62, 90, 0));

  vec.clear_duplicates ();
  EXPECT_EQ (vec.size (), 2);
}

TEST_F (MidiEventTest, DequeueOperation)
{
  MidiEvents events;
  events.queued_events_.push_back (create_note_on (1, 60, 100, 5));
  events.queued_events_.push_back (create_note_on (1, 62, 90, 25));

  // Only dequeue events between time 0-20
  events.dequeue (0, 20);

  EXPECT_EQ (events.active_events_.size (), 1);
  EXPECT_EQ (events.queued_events_.size (), 1);
  EXPECT_EQ (events.active_events_.front ().time_, 5);
}

} // namespace zrythm::dsp
