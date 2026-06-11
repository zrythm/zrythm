// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/midi_event.h"
#include "utils/midi.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class MidiEventTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    chord_c_major_.setRootNote (MusicalNote::C);
    chord_c_major_.setChordType (ChordType::Major);

    chord_a_minor_.setRootNote (MusicalNote::A);
    chord_a_minor_.setChordType (ChordType::Minor);
  }

  ChordDescriptor chord_c_major_;
  ChordDescriptor chord_a_minor_;
};

TEST_F (MidiEventTest, EventConstruction)
{
  auto ev = dsp::midi_event::make_note_on (0, 60, 100, units::samples (42));
  auto data = ev.data ();
  EXPECT_EQ (data[0] & 0xF0, 0x90);
  EXPECT_EQ (data[1], 60);
  EXPECT_EQ (data[2], 100);
  EXPECT_EQ (ev.time_, units::samples (42));
}

TEST_F (MidiEventTest, EventComparison)
{
  auto ev1 = dsp::midi_event::make_note_on (0, 60, 100, units::samples (42));
  auto ev2 = dsp::midi_event::make_note_on (0, 60, 100, units::samples (42));
  auto ev3 = dsp::midi_event::make_note_off (0, 60, units::samples (42));

  EXPECT_TRUE (ev1 == ev2);
  EXPECT_FALSE (ev1 == ev3);
}

TEST_F (MidiEventTest, SortWithNoteOffPriorityNoteOffBeforeNoteOn)
{
  std::vector<SampleBasedMidiEvent> events;
  events.push_back (
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (10)));
  events.push_back (dsp::midi_event::make_note_off (0, 60, units::samples (10)));
  events.push_back (
    dsp::midi_event::make_note_on (0, 64, 80, units::samples (10)));
  events.push_back (dsp::midi_event::make_note_off (0, 64, units::samples (20)));

  dsp::midi_event::sort_with_note_off_priority (events);

  EXPECT_TRUE (utils::midi::midi_is_note_off (events[0].data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[1].data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[2].data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_off (events[3].data ()));
}

TEST_F (MidiEventTest, SortWithNoteOffPriorityAllNotesOffBeforeNoteOn)
{
  std::vector<SampleBasedMidiEvent> events;
  events.push_back (
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (10)));
  events.push_back (
    dsp::midi_event::make_all_notes_off (0, units::samples (10)));
  events.push_back (
    dsp::midi_event::make_note_on (0, 64, 80, units::samples (10)));

  dsp::midi_event::sort_with_note_off_priority (events);

  EXPECT_TRUE (utils::midi::midi_is_all_notes_off (events[0].data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[1].data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[2].data ()));
}

TEST_F (MidiEventTest, SortWithNoteOffPriorityMixedOffTypesBeforeNoteOn)
{
  std::vector<SampleBasedMidiEvent> events;
  events.push_back (
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (10)));
  events.push_back (dsp::midi_event::make_note_off (0, 64, units::samples (10)));
  events.push_back (
    dsp::midi_event::make_note_on (0, 67, 90, units::samples (10)));
  events.push_back (
    dsp::midi_event::make_all_notes_off (0, units::samples (10)));

  dsp::midi_event::sort_with_note_off_priority (events);

  EXPECT_TRUE (utils::midi::midi_is_note_off (events[0].data ()));
  EXPECT_TRUE (utils::midi::midi_is_all_notes_off (events[1].data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[2].data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[3].data ()));
}

TEST_F (MidiEventTest, SortWithNoteOffPriorityDifferentTimestampsPreserved)
{
  std::vector<SampleBasedMidiEvent> events;
  events.push_back (
    dsp::midi_event::make_note_on (0, 64, 80, units::samples (20)));
  events.push_back (dsp::midi_event::make_note_off (0, 60, units::samples (10)));
  events.push_back (
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (10)));

  dsp::midi_event::sort_with_note_off_priority (events);

  EXPECT_EQ (events[0].time_, units::samples (10));
  EXPECT_TRUE (utils::midi::midi_is_note_off (events[0].data ()));
  EXPECT_EQ (events[1].time_, units::samples (10));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[1].data ()));
  EXPECT_EQ (events[2].time_, units::samples (20));
  EXPECT_TRUE (utils::midi::midi_is_note_on (events[2].data ()));
}

TEST_F (MidiEventTest, VectorBasicOperations)
{
  std::vector<SampleBasedMidiEvent> vec;
  EXPECT_TRUE (vec.empty ());

  // Add events
  vec.push_back (dsp::midi_event::make_note_on (0, 60, 100, units::samples (0)));
  vec.push_back (dsp::midi_event::make_note_on (0, 62, 90, units::samples (10)));
  EXPECT_EQ (vec.size (), 2);

  // Test iteration
  int count = 0;
  for (const auto &ev : vec)
    {
      (void) ev;
      count++;
    }
  EXPECT_EQ (count, 2);

  // Test removal
  vec.erase (vec.begin ());
  EXPECT_EQ (vec.size (), 1);
  EXPECT_EQ (vec.front ().data ()[1], 62); // Remaining note
}

TEST_F (MidiEventTest, EventFiltering)
{
  std::vector<SampleBasedMidiEvent> src;
  src.push_back (
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (5))); // Channel 0
  src.push_back (
    dsp::midi_event::make_note_on (1, 62, 90, units::samples (15))); // Channel 1
  src.push_back (
    dsp::midi_event::make_note_on (0, 64, 80, units::samples (25))); // Channel 0

  std::vector<SampleBasedMidiEvent> dest;
  std::array<bool, 16>              channels{};
  channels[0] = true; // Only allow channel 0

  for (const auto &ev : src)
    {
      if (ev.time_ >= units::samples (0) && ev.time_ < units::samples (30))
        {
          if (channels[ev.data ()[0] & 0x0F])
            dest.push_back (ev);
        }
    }
  EXPECT_EQ (dest.size (), 2);
  EXPECT_EQ (dest.at (0).data ()[0] & 0x0F, 0); // Channel 0
  EXPECT_EQ (dest.at (1).data ()[0] & 0x0F, 0); // Channel 0
}

TEST_F (MidiEventTest, EventSorting)
{
  std::vector<SampleBasedMidiEvent> vec;
  vec.push_back (
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (20)));
  vec.push_back (dsp::midi_event::make_note_off (0, 60, units::samples (10)));
  vec.push_back (dsp::midi_event::make_note_on (0, 62, 90, units::samples (5)));

  dsp::midi_event::sort (vec);

  // Should be sorted by time: 5, 10, 20
  EXPECT_EQ (vec.at (0).time_, units::samples (5));
  EXPECT_EQ (vec.at (1).time_, units::samples (10));
  EXPECT_EQ (vec.at (2).time_, units::samples (20));

  // Verify stable sort preserves insertion order at same time
  auto same_time_note_off =
    dsp::midi_event::make_note_off (0, 64, units::samples (10));
  auto same_time_note_on =
    dsp::midi_event::make_note_on (0, 64, 80, units::samples (10));
  vec.push_back (same_time_note_off);
  vec.push_back (same_time_note_on);
  dsp::midi_event::sort (vec);

  // Events at time=10: original note-off(60), then note-off(64), then note-on(64)
  ASSERT_EQ (vec.size (), 5u);
  EXPECT_EQ (vec.at (0).time_, units::samples (5));
  EXPECT_EQ (vec.at (1).time_, units::samples (10));
  EXPECT_TRUE (utils::midi::midi_is_note_off (vec.at (1).data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_off (vec.at (2).data ()));
  EXPECT_TRUE (utils::midi::midi_is_note_on (vec.at (3).data ()));
  EXPECT_EQ (vec.at (4).time_, units::samples (20));
}

TEST_F (MidiEventTest, ChannelSetting)
{
  std::vector<SampleBasedMidiEvent> vec;
  vec.push_back (dsp::midi_event::make_note_on (0, 60, 100, units::samples (0)));
  vec.push_back (dsp::midi_event::make_note_on (1, 62, 90, units::samples (0)));

  dsp::midi_event::set_channel (vec, 2);

  for (const auto &ev : vec)
    {
      EXPECT_EQ (ev.data ()[0] & 0x0F, 2); // Channel 2
    }
}

TEST_F (MidiEventTest, PanicFunction)
{
  std::vector<SampleBasedMidiEvent> vec;
  for (midi_byte_t ch = 0; ch < 16; ++ch)
    {
      auto ev = dsp::midi_event::make_all_notes_off<units::sample_t> (
        ch, units::samples (0));
      EXPECT_TRUE (utils::midi::midi_is_all_notes_off (ev.data ()));
      EXPECT_EQ (utils::midi::midi_get_channel_0_to_15 (ev.data ()), ch);
      vec.push_back (ev);
    }
  EXPECT_EQ (vec.size (), 16);

  for (const auto &ev : vec)
    {
      auto d = ev.data ();
      EXPECT_EQ (d.size (), 3u);
      EXPECT_EQ (d[1], utils::midi::MIDI_ALL_NOTES_OFF);
      EXPECT_EQ (d[2], 0);
    }
}

TEST_F (MidiEventTest, ClearDuplicates)
{
  std::vector<SampleBasedMidiEvent> vec;
  auto ev1 = dsp::midi_event::make_note_on (0, 60, 100, units::samples (0));
  auto ev2 =
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (0)); // Duplicate

  vec.push_back (ev1);
  vec.push_back (ev2);
  vec.push_back (dsp::midi_event::make_note_on (0, 62, 90, units::samples (0)));

  std::ranges::sort (vec, {}, &SampleBasedMidiEvent::time_);
  auto dup = std::ranges::unique (vec);
  vec.erase (dup.begin (), dup.end ());
  EXPECT_EQ (vec.size (), 2);
}

TEST_F (MidiEventTest, DequeueOperation)
{
  std::vector<SampleBasedMidiEvent> queued;
  queued.push_back (
    dsp::midi_event::make_note_on (0, 60, 100, units::samples (5)));
  queued.push_back (
    dsp::midi_event::make_note_on (0, 62, 90, units::samples (25)));

  // Only dequeue events between time 0-20
  std::vector<SampleBasedMidiEvent> active;
  auto [first, last] = std::ranges::remove_if (
    queued,
    [&active, end_time = units::samples (20)] (const SampleBasedMidiEvent &ev) {
      if (ev.time_ < end_time)
        {
          active.push_back (ev);
          return true;
        }
      return false;
    });
  queued.erase (first, last);

  EXPECT_EQ (active.size (), 1);
  EXPECT_EQ (queued.size (), 1);
  EXPECT_EQ (active.front ().time_, units::samples (5));
}

TEST (TemplatedMidiEventTest, InlineStorageForChannelMessages)
{
  SampleBasedMidiEvent ev;
  const midi_byte_t    note_on[] = { 0x90, 0x3C, 0x64 };
  ev.set_inline (note_on);
  ev.time_ = units::samples (100);

  ASSERT_TRUE (ev.is_inline ());
  ASSERT_EQ (ev.size_, 3);
  ASSERT_EQ (ev.data ()[0], 0x90);
  ASSERT_EQ (ev.data ()[1], 0x3C);
  ASSERT_EQ (ev.data ()[2], 0x64);
}

TEST (TemplatedMidiEventTest, ExternalStorageForSysex)
{
  auto mutable_data = std::make_shared<midi_byte_t[]> (6);
  mutable_data[0] = 0xF0;
  mutable_data[1] = 0x7E;
  mutable_data[2] = 0x01;
  mutable_data[3] = 0x02;
  mutable_data[4] = 0x03;
  mutable_data[5] = 0xF7;

  SampleBasedMidiEvent ev;
  ev.set_external (std::move (mutable_data), 6);
  ev.time_ = units::samples (200);

  ASSERT_FALSE (ev.is_inline ());
  ASSERT_EQ (ev.size_, 6);
  ASSERT_EQ (ev.data ()[0], 0xF0);
  ASSERT_EQ (ev.data ()[5], 0xF7);
}

TEST (TemplatedMidiEventTest, CopySharesExternalStorage)
{
  auto mutable_data = std::make_shared<midi_byte_t[]> (4);
  mutable_data[0] = 0xF0;
  mutable_data[3] = 0xF7;

  SampleBasedMidiEvent original;
  original.set_external (std::move (mutable_data), 4);

  SampleBasedMidiEvent copy = original;
  ASSERT_EQ (copy.data ()[0], 0xF0);
  ASSERT_EQ (copy.data ()[3], 0xF7);
  ASSERT_EQ (copy.size_, 4);
}

TEST (TemplatedMidiEventTest, InlineStorageSizeMatchesPointerSize)
{
  static_assert (
    SampleBasedMidiEvent::InlineStorage{}.size ()
    == sizeof (const midi_byte_t *));
}

TEST (TemplatedMidiEventTest, TickBasedEventUsesStrongType)
{
  TickBasedMidiEvent ev;
  const midi_byte_t  raw[] = { 0x90, 0x3C, 0x64 };
  ev.set_inline (raw);
  ev.time_ = units::ticks (960);

  ASSERT_EQ (ev.time_.in (units::ticks), 960);
}

TEST (TemplatedMidiEventTest, SampleBasedEventUsesStrongType)
{
  SampleBasedMidiEvent ev;
  const midi_byte_t    raw[] = { 0x90, 0x3C, 0x64 };
  ev.set_inline (raw);
  ev.time_ = units::samples (44100);

  ASSERT_EQ (ev.time_.in (units::samples), 44100);
}

TEST (TemplatedMidiEventTest, CrossDimensionConversionIsRejected)
{
  static_assert (
    !std::constructible_from<SampleBasedMidiEvent, TickBasedMidiEvent>);
  static_assert (
    !std::constructible_from<TickBasedMidiEvent, SampleBasedMidiEvent>);
  static_assert (
    std::constructible_from<SampleBasedMidiEvent, RealtimeMidiEvent>);
}

TEST (TemplatedMidiEventTest, SameDimensionConversionIsAccepted)
{
  static_assert (
    std::constructible_from<
      SampleBasedMidiEvent, MidiEvent<au::Quantity<units::Sample, int>>>);
}

TEST (TemplatedMidiEventTest, ExternalStorageDataIntegrity)
{
  constexpr size_t payload_size = 20;
  auto             data = std::make_shared<midi_byte_t[]> (payload_size);
  const std::array<midi_byte_t, payload_size> expected = {
    0xF0, 0x7E, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
    0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x10, 0x11, 0xF7
  };
  std::ranges::copy (expected, data.get ());

  SampleBasedMidiEvent ev;
  ev.set_external (std::move (data), payload_size);
  ev.time_ = units::samples (300);

  const auto span = ev.data ();
  ASSERT_EQ (span.size (), payload_size);
  for (size_t i = 0; i < payload_size; ++i)
    {
      EXPECT_EQ (span[i], expected[i]) << "Mismatch at byte " << i;
    }
}

TEST (TemplatedMidiEventTest, ExternalStorageMoveSemantics)
{
  auto data = std::make_shared<midi_byte_t[]> (6);
  data[0] = 0xF0;
  data[5] = 0xF7;

  SampleBasedMidiEvent original;
  original.set_external (std::move (data), 6);
  original.time_ = units::samples (100);

  SampleBasedMidiEvent moved = std::move (original);
  ASSERT_FALSE (moved.is_inline ());
  ASSERT_EQ (moved.size_, 6);
  EXPECT_EQ (moved.data ()[0], 0xF0);
  EXPECT_EQ (moved.data ()[5], 0xF7);
  EXPECT_EQ (moved.time_, units::samples (100));
}

TEST (TemplatedMidiEventTest, ExternalStorageSharedOwnership)
{
  auto data = std::make_shared<midi_byte_t[]> (4);
  data[0] = 0xF0;
  data[3] = 0xF7;

  SampleBasedMidiEvent original;
  original.set_external (data, 4);

  EXPECT_EQ (data.use_count (), 2);

  SampleBasedMidiEvent copy = original;
  EXPECT_EQ (data.use_count (), 3);

  const auto orig_span = original.data ();
  const auto copy_span = copy.data ();
  EXPECT_EQ (orig_span[0], copy_span[0]);
  EXPECT_EQ (orig_span[3], copy_span[3]);

  copy = SampleBasedMidiEvent{};
  EXPECT_EQ (data.use_count (), 2);
}

TEST (TemplatedMidiEventTest, SwitchFromExternalToInline)
{
  auto data = std::make_shared<midi_byte_t[]> (6);
  data[0] = 0xF0;
  data[5] = 0xF7;

  SampleBasedMidiEvent ev;
  ev.set_external (std::move (data), 6);
  ASSERT_FALSE (ev.is_inline ());

  const midi_byte_t note_on[] = { 0x90, 0x3C, 0x64 };
  ev.set_inline (note_on);
  ASSERT_TRUE (ev.is_inline ());
  ASSERT_EQ (ev.size_, 3);
  EXPECT_EQ (ev.data ()[0], 0x90);
  EXPECT_EQ (ev.data ()[1], 0x3C);
  EXPECT_EQ (ev.data ()[2], 0x64);
}

TEST (TemplatedMidiEventTest, SwitchFromInlineToExternal)
{
  SampleBasedMidiEvent ev;
  const midi_byte_t    note_on[] = { 0x90, 0x3C, 0x64 };
  ev.set_inline (note_on);
  ASSERT_TRUE (ev.is_inline ());

  auto data = std::make_shared<midi_byte_t[]> (6);
  data[0] = 0xF0;
  data[5] = 0xF7;
  ev.set_external (std::move (data), 6);

  ASSERT_FALSE (ev.is_inline ());
  ASSERT_EQ (ev.size_, 6);
  EXPECT_EQ (ev.data ()[0], 0xF0);
  EXPECT_EQ (ev.data ()[5], 0xF7);
}

TEST (TemplatedMidiEventTest, LargeSysexMessage)
{
  constexpr size_t large_size = 1024;
  auto             data = std::make_shared<midi_byte_t[]> (large_size);
  data[0] = 0xF0;
  for (size_t i = 1; i < large_size - 1; ++i)
    data[i] = static_cast<midi_byte_t> (i & 0x7F);
  data[large_size - 1] = 0xF7;

  SampleBasedMidiEvent ev;
  ev.set_external (data, large_size);
  ev.time_ = units::samples (500);

  ASSERT_FALSE (ev.is_inline ());
  ASSERT_EQ (ev.size_, large_size);
  const auto span = ev.data ();
  EXPECT_EQ (span[0], 0xF0);
  EXPECT_EQ (span[large_size - 1], 0xF7);
  EXPECT_EQ (span[42], 42 & 0x7F);
}

TEST (TemplatedMidiEventTest, ExternalStorageWithTimeConversion)
{
  auto data = std::make_shared<midi_byte_t[]> (10);
  data[0] = 0xF0;
  for (size_t i = 1; i < 9; ++i)
    data[i] = static_cast<midi_byte_t> (i);
  data[9] = 0xF7;

  MidiEvent<au::Quantity<units::Sample, int>> source;
  source.set_external (data, 10);
  source.time_ = units::samples (100);

  SampleBasedMidiEvent converted = source;
  ASSERT_FALSE (converted.is_inline ());
  ASSERT_EQ (converted.size_, 10);
  EXPECT_EQ (converted.data ()[0], 0xF0);
  EXPECT_EQ (converted.data ()[9], 0xF7);
  EXPECT_EQ (converted.time_, units::samples (100));
}

TEST (TemplatedMidiEventTest, MixedInlineAndExternalSortByTime)
{
  std::vector<SampleBasedMidiEvent> events;

  auto sysex = std::make_shared<midi_byte_t[]> (6);
  sysex[0] = 0xF0;
  sysex[5] = 0xF7;

  events.push_back (midi_event::make_note_on (0, 60, 100, units::samples (200)));
  {
    SampleBasedMidiEvent ev;
    ev.set_external (sysex, 6);
    ev.time_ = units::samples (100);
    events.push_back (ev);
  }
  events.push_back (midi_event::make_note_off (0, 60, units::samples (50)));
  events.push_back (midi_event::make_note_on (0, 64, 80, units::samples (150)));

  midi_event::sort (events);

  ASSERT_EQ (events.size (), 4);
  EXPECT_EQ (events[0].time_, units::samples (50));
  EXPECT_EQ (events[1].time_, units::samples (100));
  EXPECT_FALSE (events[1].is_inline ());
  EXPECT_EQ (events[2].time_, units::samples (150));
  EXPECT_EQ (events[3].time_, units::samples (200));
}

TEST (TemplatedMidiEventTest, MultipleExternalEventsInVector)
{
  std::vector<SampleBasedMidiEvent> events;

  for (int i = 0; i < 5; ++i)
    {
      constexpr size_t sysex_size = 10;
      auto             data = std::make_shared<midi_byte_t[]> (sysex_size);
      data[0] = 0xF0;
      data[1] = static_cast<midi_byte_t> (i);
      data[sysex_size - 1] = 0xF7;
      SampleBasedMidiEvent ev;
      ev.set_external (std::move (data), sysex_size);
      ev.time_ = units::samples (i * 100);
      events.push_back (std::move (ev));
    }

  ASSERT_EQ (events.size (), 5);
  for (int i = 0; i < 5; ++i)
    {
      EXPECT_FALSE (events[i].is_inline ());
      EXPECT_EQ (events[i].data ()[1], static_cast<midi_byte_t> (i));
      EXPECT_EQ (events[i].time_, units::samples (i * 100));
    }
}

} // namespace zrythm::dsp
