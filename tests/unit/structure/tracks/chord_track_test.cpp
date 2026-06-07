// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "dsp/midi_event.h"
#include "structure/tracks/chord_track.h"

#include <gtest/gtest.h>

namespace zrythm::structure::tracks
{

class ChordTrackTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    chord_c_major_ = dsp::ChordDescriptor (
      dsp::MusicalNote::C, false, dsp::MusicalNote::C, dsp::ChordType::Major,
      dsp::ChordAccent::None, 0);
  }

  dsp::ChordDescriptor chord_c_major_;
};

TEST_F (ChordTrackTest, TransformChordExpandsSingleNoteToChord)
{
  auto src = dsp::MidiEventBuffer::make_reserved ();
  // C3 (root for C major)
  {
    const auto _ev =
      dsp::midi_event::make_note_on (0, 48, 100, units::samples (0));
    src.push_back (_ev.time_, _ev.data ());
  }
  {
    const auto _ev = dsp::midi_event::make_note_off (0, 48, units::samples (10));
    src.push_back (_ev.time_, _ev.data ());
  }

  auto dest = dsp::MidiEventBuffer::make_reserved ();
  auto chord_mapper = [this] (midi_byte_t note) -> const dsp::ChordDescriptor * {
    return (note == 48) ? &chord_c_major_ : nullptr;
  };

  ChordTrack::transform_chord_and_append (
    dest, src, chord_mapper, 100,
    std::pair{ units::samples (0u), units::samples (20u) });

  // C major chord should have 3 notes (C, E, G): 3 note-ons + 3 note-offs
  ASSERT_EQ (dest.size (), 6);

  std::vector<midi_byte_t> note_on_pitches;
  std::vector<midi_byte_t> note_off_pitches;
  for (const auto &ev : dest)
    {
      auto d = ev.data ();
      if (utils::midi::midi_is_note_on (d))
        note_on_pitches.push_back (utils::midi::midi_get_note_number (d));
      else if (utils::midi::midi_is_note_off (d))
        note_off_pitches.push_back (utils::midi::midi_get_note_number (d));
    }

  ASSERT_EQ (note_on_pitches.size (), 3u);
  ASSERT_EQ (note_off_pitches.size (), 3u);

  std::ranges::sort (note_on_pitches);
  EXPECT_EQ (note_on_pitches[0], 48);
  EXPECT_EQ (note_on_pitches[1], 52);
  EXPECT_EQ (note_on_pitches[2], 55);

  std::ranges::sort (note_off_pitches);
  EXPECT_EQ (note_off_pitches[0], 48);
  EXPECT_EQ (note_off_pitches[1], 52);
  EXPECT_EQ (note_off_pitches[2], 55);
}

} // namespace zrythm::structure::tracks
