// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event_buffer.h"
#include "utils/midi.h"

#include <gtest/gtest.h>

using namespace zrythm;

namespace
{
dsp::MidiEventBuffer
make_reserved_buffer (size_t bytes = 4096)
{
  dsp::MidiEventBuffer buf;
  buf.reserve (bytes);
  return buf;
}
}

TEST (MidiEventBufferTest, PushAndIterateSingleEvent)
{
  auto                             buf = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> raw = { 0x90, 60, 100 };
  buf.push_back (units::samples (42u), raw);

  EXPECT_EQ (buf.size (), 1u);
  EXPECT_FALSE (buf.empty ());

  int count = 0;
  for (const auto &ev : buf)
    {
      ++count;
      EXPECT_EQ (ev.time (), units::samples (42u));
      EXPECT_EQ (ev.data ().size (), 3u);
      EXPECT_EQ (ev.data ()[0], 0x90);
      EXPECT_EQ (ev.data ()[1], 60);
      EXPECT_EQ (ev.data ()[2], 100);
    }
  EXPECT_EQ (count, 1);
}

TEST (MidiEventBufferTest, PushMultipleEvents)
{
  auto                             buf = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> note_on = { 0x90, 60, 100 };
  const std::array<midi_byte_t, 3> note_off = { 0x80, 60, 0 };

  buf.push_back (units::samples (0u), note_on);
  buf.push_back (units::samples (100u), note_off);

  EXPECT_EQ (buf.size (), 2u);

  auto it = buf.begin ();
  EXPECT_EQ ((*it).time (), units::samples (0u));
  ++it;
  EXPECT_EQ ((*it).time (), units::samples (100u));
  ++it;
  EXPECT_EQ (it, buf.end ());
}

TEST (MidiEventBufferTest, LargeEventSysEx)
{
  auto                        buf = make_reserved_buffer ();
  std::array<midi_byte_t, 16> sysex = {
    0xF0, 0x7E, 0x7F, 0x09, 0x01, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xF7
  };

  buf.push_back (units::samples (0u), sysex);
  EXPECT_EQ (buf.size (), 1u);

  for (const auto &ev : buf)
    {
      EXPECT_EQ (ev.data ().size (), 16u);
      EXPECT_EQ (ev.data ()[0], 0xF0);
      EXPECT_EQ (ev.data ()[15], 0xF7);
    }
}

TEST (MidiEventBufferTest, ClearAndReuse)
{
  auto                             buf = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> raw = { 0x90, 60, 100 };

  buf.push_back (units::samples (0u), raw);
  EXPECT_EQ (buf.size (), 1u);

  buf.clear ();
  EXPECT_EQ (buf.size (), 0u);
  EXPECT_TRUE (buf.empty ());
  EXPECT_EQ (buf.begin (), buf.end ());

  buf.push_back (units::samples (10u), raw);
  EXPECT_EQ (buf.size (), 1u);
  for (const auto &ev : buf)
    {
      EXPECT_EQ (ev.time (), units::samples (10u));
    }
}

TEST (MidiEventBufferTest, SortNoteOffBeforeNoteOn)
{
  auto                             buf = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> note_on = { 0x90, 60, 100 };
  const std::array<midi_byte_t, 3> note_off = { 0x80, 60, 0 };

  buf.push_back (units::samples (10u), note_on);
  buf.push_back (units::samples (10u), note_off);

  dsp::midi_event::sort_with_note_off_priority (buf);

  auto it = buf.begin ();
  EXPECT_EQ ((*it).data ()[0] & 0xF0, 0x80);
  ++it;
  EXPECT_EQ ((*it).data ()[0] & 0xF0, 0x90);
}

TEST (MidiEventBufferTest, AppendInRange)
{
  auto                             src = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> ev1 = { 0x90, 60, 100 };
  const std::array<midi_byte_t, 3> ev2 = { 0x90, 62, 90 };
  const std::array<midi_byte_t, 3> ev3 = { 0x90, 64, 80 };

  src.push_back (units::samples (5u), ev1);
  src.push_back (units::samples (15u), ev2);
  src.push_back (units::samples (25u), ev3);

  dsp::MidiEventBuffer dst;
  dst.reserve (4096);
  dsp::midi_event::append_in_range (
    dst, src, std::pair{ units::samples (10u), units::samples (20u) });

  EXPECT_EQ (dst.size (), 1u);
  for (const auto &ev : dst)
    {
      EXPECT_EQ (ev.time (), units::samples (15u));
    }
}

TEST (MidiEventBufferTest, EmptyBufferIteration)
{
  dsp::MidiEventBuffer buf;
  EXPECT_TRUE (buf.empty ());
  EXPECT_EQ (buf.size (), 0u);
  for (const auto &ev : buf)
    {
      (void) ev;
      FAIL () << "Should not iterate empty buffer";
    }
}

TEST (MidiEventBufferTest, ViewDataSpan)
{
  auto                             buf = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> raw = { 0x90, 60, 100 };
  buf.push_back (units::samples (0u), raw);

  auto view = *buf.begin ();
  auto span = view.data ();
  EXPECT_EQ (span.size (), 3u);
  EXPECT_EQ (span[0], 0x90);
}

TEST (MidiEventBufferTest, SizeInBytesAndCapacity)
{
  auto buf = make_reserved_buffer (1024);
  EXPECT_EQ (buf.size (), 0u);
  EXPECT_EQ (buf.size_in_bytes (), 0u);
  EXPECT_EQ (buf.capacity (), 1024u);

  const std::array<midi_byte_t, 3> raw = { 0x90, 60, 100 };
  buf.push_back (units::samples (0u), raw);
  EXPECT_EQ (buf.size (), 1u);
  EXPECT_EQ (buf.size_in_bytes (), dsp::MidiEventBuffer::kHeaderSize + 3);
  EXPECT_EQ (buf.capacity (), 1024u);

  buf.push_back (units::samples (10u), raw);
  EXPECT_EQ (buf.size (), 2u);
  EXPECT_EQ (buf.size_in_bytes (), 2 * (dsp::MidiEventBuffer::kHeaderSize + 3));
}

TEST (MidiEventBufferTest, RemoveIf)
{
  auto                             buf = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> note_on_60 = { 0x90, 60, 100 };
  const std::array<midi_byte_t, 3> note_on_62 = { 0x90, 62, 90 };
  const std::array<midi_byte_t, 3> note_off_60 = { 0x80, 60, 0 };

  buf.push_back (units::samples (0u), note_on_60);
  buf.push_back (units::samples (10u), note_on_62);
  buf.push_back (units::samples (20u), note_off_60);
  ASSERT_EQ (buf.size (), 3u);

  buf.remove_if ([] (const dsp::MidiEventView &ev) {
    return ev.data ().size () >= 2 && ev.data ()[1] == 60;
  });

  EXPECT_EQ (buf.size (), 1u);
  auto it = buf.begin ();
  EXPECT_EQ ((*it).data ()[1], 62);
  EXPECT_EQ ((*it).time (), units::samples (10u));
}

TEST (MidiEventBufferTest, RemoveIfNoMatch)
{
  auto                             buf = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> raw = { 0x90, 60, 100 };

  buf.push_back (units::samples (0u), raw);
  buf.remove_if ([] (const dsp::MidiEventView &) { return false; });
  EXPECT_EQ (buf.size (), 1u);
}

TEST (MidiEventBufferTest, RemoveIfEmpty)
{
  dsp::MidiEventBuffer buf;
  buf.reserve (1024);
  buf.remove_if ([] (const dsp::MidiEventView &) { return true; });
  EXPECT_EQ (buf.size (), 0u);
}

TEST (MidiEventBufferTest, Swap)
{
  auto                             buf_a = make_reserved_buffer ();
  auto                             buf_b = make_reserved_buffer ();
  const std::array<midi_byte_t, 3> ev1 = { 0x90, 60, 100 };
  const std::array<midi_byte_t, 3> ev2 = { 0x90, 62, 90 };

  buf_a.push_back (units::samples (0u), ev1);
  buf_b.push_back (units::samples (5u), ev2);
  ASSERT_EQ (buf_a.size (), 1u);
  ASSERT_EQ (buf_b.size (), 1u);

  buf_a.swap (buf_b);

  EXPECT_EQ (buf_a.size (), 1u);
  EXPECT_EQ (buf_b.size (), 1u);

  auto a_it = buf_a.begin ();
  EXPECT_EQ ((*a_it).time (), units::samples (5u));
  EXPECT_EQ ((*a_it).data ()[1], 62);

  auto b_it = buf_b.begin ();
  EXPECT_EQ ((*b_it).time (), units::samples (0u));
  EXPECT_EQ ((*b_it).data ()[1], 60);
}
