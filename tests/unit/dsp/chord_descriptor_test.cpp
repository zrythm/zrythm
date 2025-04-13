// SPDX-FileCopyrightText: © 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/chord_descriptor.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

TEST (ChordDescriptorTest, BasicChords)
{
  // Test major triad
  ChordDescriptor major (
    MusicalNote::C, false, MusicalNote::C, ChordType::Major, ChordAccent::None,
    0);
  EXPECT_TRUE (major.is_key_in_chord (MusicalNote::C));
  EXPECT_TRUE (major.is_key_in_chord (MusicalNote::E));
  EXPECT_TRUE (major.is_key_in_chord (MusicalNote::G));
  EXPECT_FALSE (major.is_key_in_chord (MusicalNote::B));
  EXPECT_EQ (major.to_string (), "CMaj");

  // Test minor triad
  ChordDescriptor minor (
    MusicalNote::A, false, MusicalNote::A, ChordType::Minor, ChordAccent::None,
    0);
  EXPECT_TRUE (minor.is_key_in_chord (MusicalNote::A));
  EXPECT_TRUE (minor.is_key_in_chord (MusicalNote::C));
  EXPECT_TRUE (minor.is_key_in_chord (MusicalNote::E));
  EXPECT_EQ (minor.to_string (), "Amin");
}

TEST (ChordDescriptorTest, ChordInversions)
{
  ChordDescriptor chord (
    MusicalNote::C, false, MusicalNote::C, ChordType::Major, ChordAccent::None,
    1);
  EXPECT_TRUE (chord.is_key_in_chord (MusicalNote::C));
  EXPECT_TRUE (chord.is_key_in_chord (MusicalNote::E));
  EXPECT_TRUE (chord.is_key_in_chord (MusicalNote::G));
  EXPECT_EQ (chord.to_string (), "CMaj i1");
}

TEST (ChordDescriptorTest, ChordAccents)
{
  ChordDescriptor seventh (
    MusicalNote::D, false, MusicalNote::D, ChordType::Major,
    ChordAccent::Seventh, 0);
  EXPECT_TRUE (seventh.is_key_in_chord (MusicalNote::D));
  EXPECT_TRUE (seventh.is_key_in_chord (MusicalNote::FSharp));
  EXPECT_TRUE (seventh.is_key_in_chord (MusicalNote::A));
  EXPECT_TRUE (seventh.is_key_in_chord (MusicalNote::C));
  EXPECT_EQ (seventh.to_string (), "DMaj 7");
}

TEST (ChordDescriptorTest, BassNotes)
{
  ChordDescriptor with_bass (
    MusicalNote::C, true, MusicalNote::G, ChordType::Major, ChordAccent::None,
    0);
  EXPECT_TRUE (with_bass.is_key_bass (MusicalNote::G));
  EXPECT_FALSE (with_bass.is_key_bass (MusicalNote::C));
  EXPECT_EQ (with_bass.to_string (), "CMaj/G");
}

TEST (ChordDescriptorTest, MaxInversions)
{
  ChordDescriptor triad (
    MusicalNote::C, false, MusicalNote::C, ChordType::Major, ChordAccent::None,
    0);
  EXPECT_EQ (triad.get_max_inversion (), 2);

  ChordDescriptor seventh (
    MusicalNote::C, false, MusicalNote::C, ChordType::Major,
    ChordAccent::Seventh, 0);
  EXPECT_EQ (seventh.get_max_inversion (), 3);

  ChordDescriptor extended (
    MusicalNote::C, false, MusicalNote::C, ChordType::Major,
    ChordAccent::SixthThirteenth, 0);
  EXPECT_EQ (extended.get_max_inversion (), 4);
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
    MusicalNote::C, true, MusicalNote::G, ChordType::Major,
    ChordAccent::Seventh, 1);
  chord1.update_notes ();

  // Serialize to JSON
  auto json_str = chord1.serialize_to_json_string ();

  // Create new object and deserialize
  ChordDescriptor chord2;
  chord2.deserialize_from_json_string (json_str.c_str ());

  // Verify all fields match
  EXPECT_EQ (chord1.has_bass_, chord2.has_bass_);
  EXPECT_EQ (chord1.root_note_, chord2.root_note_);
  EXPECT_EQ (chord1.bass_note_, chord2.bass_note_);
  EXPECT_EQ (chord1.type_, chord2.type_);
  EXPECT_EQ (chord1.accent_, chord2.accent_);
  EXPECT_EQ (chord1.notes_, chord2.notes_);
  EXPECT_EQ (chord1.inversion_, chord2.inversion_);
}
