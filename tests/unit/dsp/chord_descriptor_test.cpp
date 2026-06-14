// SPDX-FileCopyrightText: © 2024-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "utils/utf8_string.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

using namespace zrythm::dsp;

TEST (ChordDescriptorTest, BasicChords)
{
  ChordDescriptor major (MusicalNote::C, ChordType::Major);
  EXPECT_TRUE (major.isKeyInChord (MusicalNote::C));
  EXPECT_TRUE (major.isKeyInChord (MusicalNote::E));
  EXPECT_TRUE (major.isKeyInChord (MusicalNote::G));
  EXPECT_FALSE (major.isKeyInChord (MusicalNote::B));
  EXPECT_EQ (major.to_string (), "CMaj");

  ChordDescriptor minor (MusicalNote::A, ChordType::Minor);
  EXPECT_TRUE (minor.isKeyInChord (MusicalNote::A));
  EXPECT_TRUE (minor.isKeyInChord (MusicalNote::C));
  EXPECT_TRUE (minor.isKeyInChord (MusicalNote::E));
  EXPECT_EQ (minor.to_string (), "Amin");
}

TEST (ChordDescriptorTest, ChordInversions)
{
  ChordDescriptor chord (MusicalNote::C, ChordType::Major, ChordAccent::None, 1);
  EXPECT_TRUE (chord.isKeyInChord (MusicalNote::C));
  EXPECT_TRUE (chord.isKeyInChord (MusicalNote::E));
  EXPECT_TRUE (chord.isKeyInChord (MusicalNote::G));
  EXPECT_EQ (chord.to_string (), "CMaj i1");
}

TEST (ChordDescriptorTest, ChordAccents)
{
  ChordDescriptor seventh (
    MusicalNote::D, ChordType::Major, ChordAccent::Seventh);
  EXPECT_TRUE (seventh.isKeyInChord (MusicalNote::D));
  EXPECT_TRUE (seventh.isKeyInChord (MusicalNote::FSharp));
  EXPECT_TRUE (seventh.isKeyInChord (MusicalNote::A));
  EXPECT_TRUE (seventh.isKeyInChord (MusicalNote::C));
  EXPECT_EQ (seventh.to_string (), "DMaj 7");
}

TEST (ChordDescriptorTest, BassNotes)
{
  ChordDescriptor with_bass (
    MusicalNote::C, ChordType::Major, ChordAccent::None, 0, MusicalNote::G);
  EXPECT_TRUE (with_bass.isKeyBass (MusicalNote::G));
  EXPECT_FALSE (with_bass.isKeyBass (MusicalNote::C));
  EXPECT_EQ (with_bass.to_string (), "CMaj/G");
}

TEST (ChordDescriptorTest, MaxInversions)
{
  ChordDescriptor triad (MusicalNote::C, ChordType::Major);
  EXPECT_EQ (triad.maxInversion (), 2);

  ChordDescriptor seventh (
    MusicalNote::C, ChordType::Major, ChordAccent::Seventh);
  EXPECT_EQ (seventh.maxInversion (), 3);

  ChordDescriptor extended (
    MusicalNote::C, ChordType::Major, ChordAccent::SixthThirteenth);
  EXPECT_EQ (extended.maxInversion (), 4);
}

TEST (ChordDescriptorTest, StringRepresentations)
{
  EXPECT_EQ (ChordDescriptor::chord_type_to_string (ChordType::Major), "Maj");
  EXPECT_EQ (ChordDescriptor::chord_type_to_string (ChordType::Minor), "min");
  EXPECT_EQ (
    ChordDescriptor::chord_type_to_string (ChordType::Diminished), "dim");

  EXPECT_EQ (ChordDescriptor::note_to_string (MusicalNote::C), "C");
  EXPECT_EQ (ChordDescriptor::note_to_string (MusicalNote::FSharp), "F♯");
}

TEST (ChordDescriptorTest, Serialization)
{
  ChordDescriptor chord1 (
    MusicalNote::C, ChordType::Major, ChordAccent::Seventh, 1, MusicalNote::G);

  nlohmann::json j = chord1;

  ChordDescriptor chord2;
  from_json (j, chord2);

  EXPECT_TRUE (chord1.isEquivalent (chord2));
}

TEST (ChordDescriptorTest, GetIntervals)
{
  ChordDescriptor major (MusicalNote::C, ChordType::Major);
  auto            intervals = major.getIntervals ();
  EXPECT_EQ (intervals.size (), 3u);
  EXPECT_EQ (intervals[0], 0);
  EXPECT_EQ (intervals[1], 4);
  EXPECT_EQ (intervals[2], 7);

  ChordDescriptor minor7 (
    MusicalNote::A, ChordType::Minor, ChordAccent::Seventh);
  intervals = minor7.getIntervals ();
  EXPECT_EQ (intervals.size (), 4u);
  EXPECT_EQ (intervals[0], 0);
  EXPECT_EQ (intervals[1], 3);
  EXPECT_EQ (intervals[2], 7);
  EXPECT_EQ (intervals[3], 10);
}

TEST (ChordDescriptorTest, GetMidiPitches)
{
  ChordDescriptor c_major (MusicalNote::C, ChordType::Major);
  auto            pitches = c_major.getMidiPitches ();
  ASSERT_EQ (pitches.size (), 3u);
  EXPECT_EQ (pitches[0], 48); // C3
  EXPECT_EQ (pitches[1], 52); // E3
  EXPECT_EQ (pitches[2], 55); // G3

  ChordDescriptor with_bass (
    MusicalNote::C, ChordType::Major, ChordAccent::None, 0, MusicalNote::G);
  pitches = with_bass.getMidiPitches ();
  ASSERT_EQ (pitches.size (), 4u);
  EXPECT_EQ (pitches[0], 43); // G2 (bass)
  EXPECT_EQ (pitches[1], 48); // C3
  EXPECT_EQ (pitches[2], 52); // E3
  EXPECT_EQ (pitches[3], 55); // G3
}
