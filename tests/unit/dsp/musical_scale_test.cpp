// SPDX-FileCopyrightText: © 2024-2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/musical_scale.h"

#include <QSignalSpy>

#include <gtest/gtest.h>

using namespace zrythm::dsp;

TEST (MusicalScaleTest, BasicScales)
{
  // Test major scale
  MusicalScale major (MusicalScale::ScaleType::Major, MusicalNote::C);
  // Notes in scale
  EXPECT_TRUE (major.containsNote (MusicalNote::C));
  EXPECT_TRUE (major.containsNote (MusicalNote::D));
  EXPECT_TRUE (major.containsNote (MusicalNote::E));
  EXPECT_TRUE (major.containsNote (MusicalNote::F));
  EXPECT_TRUE (major.containsNote (MusicalNote::G));
  EXPECT_TRUE (major.containsNote (MusicalNote::A));
  EXPECT_TRUE (major.containsNote (MusicalNote::B));
  // Notes not in scale
  EXPECT_FALSE (major.containsNote (MusicalNote::CSharp));
  EXPECT_FALSE (major.containsNote (MusicalNote::DSharp));
  EXPECT_FALSE (major.containsNote (MusicalNote::FSharp));
  EXPECT_FALSE (major.containsNote (MusicalNote::GSharp));
  EXPECT_FALSE (major.containsNote (MusicalNote::ASharp));
  EXPECT_EQ (major.to_string (), "C Major");

  // Test natural minor scale
  MusicalScale minor (MusicalScale::ScaleType::Minor, MusicalNote::A);
  // Notes in scale
  EXPECT_TRUE (minor.containsNote (MusicalNote::A));
  EXPECT_TRUE (minor.containsNote (MusicalNote::B));
  EXPECT_TRUE (minor.containsNote (MusicalNote::C));
  EXPECT_TRUE (minor.containsNote (MusicalNote::D));
  EXPECT_TRUE (minor.containsNote (MusicalNote::E));
  EXPECT_TRUE (minor.containsNote (MusicalNote::F));
  EXPECT_TRUE (minor.containsNote (MusicalNote::G));
  // Notes not in scale
  EXPECT_FALSE (minor.containsNote (MusicalNote::GSharp));
  EXPECT_FALSE (minor.containsNote (MusicalNote::ASharp));
  EXPECT_FALSE (minor.containsNote (MusicalNote::CSharp));
  EXPECT_FALSE (minor.containsNote (MusicalNote::DSharp));
  EXPECT_FALSE (minor.containsNote (MusicalNote::FSharp));
  EXPECT_EQ (minor.to_string (), "A Minor");
}

TEST (MusicalScaleTest, ChordContainment)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);

  // Test major chord in scale
  ChordDescriptor c_maj (
    MusicalNote::C, false, MusicalNote::C, ChordType::Major, ChordAccent::None,
    0);
  EXPECT_TRUE (c_major.contains_chord (c_maj));

  // Test minor chord in scale
  ChordDescriptor d_min (
    MusicalNote::D, false, MusicalNote::D, ChordType::Minor, ChordAccent::None,
    0);
  EXPECT_TRUE (c_major.contains_chord (d_min));

  // Test chord not in scale
  ChordDescriptor c_sharp_maj (
    MusicalNote::CSharp, false, MusicalNote::CSharp, ChordType::Major,
    ChordAccent::None, 0);
  EXPECT_FALSE (c_major.contains_chord (c_sharp_maj));
}

TEST (MusicalScaleTest, ScaleTriads)
{
  auto triads = MusicalScale::get_triad_types_for_type (
    MusicalScale::ScaleType::Major, true);
  EXPECT_EQ (triads[0], ChordType::Major);
  EXPECT_EQ (triads[1], ChordType::Minor);
  EXPECT_EQ (triads[2], ChordType::Minor);
  EXPECT_EQ (triads[3], ChordType::Major);
  EXPECT_EQ (triads[4], ChordType::Major);
  EXPECT_EQ (triads[5], ChordType::Minor);
  EXPECT_EQ (triads[6], ChordType::Diminished);
}

TEST (MusicalScaleTest, AccentInScale)
{
  MusicalScale c_major (MusicalScale::ScaleType::Major, MusicalNote::C);

  // Test major seventh in scale (B is in C major)
  EXPECT_TRUE (c_major.is_accent_in_scale (
    MusicalNote::C, ChordType::Major, ChordAccent::MajorSeventh));

  // Test seventh not in scale (Bb is not in C major)
  EXPECT_FALSE (c_major.is_accent_in_scale (
    MusicalNote::C, ChordType::Major, ChordAccent::Seventh));

  // Test ninth not in scale (requires seventh)
  EXPECT_FALSE (c_major.is_accent_in_scale (
    MusicalNote::C, ChordType::Major, ChordAccent::Ninth));
}

TEST (MusicalScaleTest, ExoticScales)
{
  // Test harmonic minor
  MusicalScale harmonic_minor (
    MusicalScale::ScaleType::HarmonicMinor, MusicalNote::A);
  EXPECT_TRUE (harmonic_minor.containsNote (MusicalNote::A));
  EXPECT_TRUE (harmonic_minor.containsNote (MusicalNote::B));
  EXPECT_TRUE (harmonic_minor.containsNote (MusicalNote::C));
  EXPECT_TRUE (harmonic_minor.containsNote (MusicalNote::D));
  EXPECT_TRUE (harmonic_minor.containsNote (MusicalNote::E));
  EXPECT_TRUE (harmonic_minor.containsNote (MusicalNote::F));
  EXPECT_TRUE (harmonic_minor.containsNote (MusicalNote::GSharp));
  EXPECT_FALSE (harmonic_minor.containsNote (MusicalNote::G));

  // Test whole tone scale
  MusicalScale whole_tone (MusicalScale::ScaleType::WholeTone, MusicalNote::C);
  EXPECT_TRUE (whole_tone.containsNote (MusicalNote::C));
  EXPECT_TRUE (whole_tone.containsNote (MusicalNote::D));
  EXPECT_TRUE (whole_tone.containsNote (MusicalNote::E));
  EXPECT_TRUE (whole_tone.containsNote (MusicalNote::FSharp));
  EXPECT_TRUE (whole_tone.containsNote (MusicalNote::GSharp));
  EXPECT_TRUE (whole_tone.containsNote (MusicalNote::ASharp));
}

TEST (MusicalScaleTest, Serialization)
{
  MusicalScale scale1 (MusicalScale::ScaleType::Major, MusicalNote::C);

  // Serialize to JSON
  nlohmann::json j = scale1;

  // Create new object and deserialize
  MusicalScale scale2;
  from_json (j, scale2);

  // Verify all fields match
  EXPECT_EQ (scale1.scaleType (), scale2.scaleType ());
  EXPECT_EQ (scale1.rootKey (), scale2.rootKey ());
}

// QML interface
TEST (MusicalScaleTest, QmlProperties)
{
  MusicalScale scale;

  // Test scaleType property
  QSignalSpy scaleTypeSpy (&scale, &MusicalScale::scaleTypeChanged);
  scale.setScaleType (MusicalScale::ScaleType::Minor);
  EXPECT_EQ (scale.scaleType (), MusicalScale::ScaleType::Minor);
  EXPECT_EQ (scaleTypeSpy.count (), 1);

  // Test rootKey property
  QSignalSpy rootKeySpy (&scale, &MusicalScale::rootKeyChanged);
  scale.setRootKey (MusicalNote::D);
  EXPECT_EQ (scale.rootKey (), MusicalNote::D);
  EXPECT_EQ (rootKeySpy.count (), 1);

  // Test clamping of values
  scale.setScaleType (static_cast<MusicalScale::ScaleType> (1000));
  EXPECT_NE (scale.scaleType (), static_cast<MusicalScale::ScaleType> (1000));
  EXPECT_GE (scaleTypeSpy.count (), 1); // At least one change

  scale.setRootKey (static_cast<MusicalNote> (100));
  EXPECT_NE (scale.rootKey (), static_cast<MusicalNote> (100));
  EXPECT_GE (rootKeySpy.count (), 1); // At least one change
}

TEST (MusicalScaleTest, QmlMethods)
{
  MusicalScale scale (MusicalScale::ScaleType::Major, MusicalNote::C);

  // Test containsNote method
  EXPECT_TRUE (scale.containsNote (MusicalNote::C));
  EXPECT_TRUE (scale.containsNote (MusicalNote::G));
  EXPECT_FALSE (scale.containsNote (MusicalNote::CSharp));

  // Test toString method
  EXPECT_EQ (scale.toString (), QString ("C Major"));

  // Test with different scale
  MusicalScale harmonicMinor (
    MusicalScale::ScaleType::HarmonicMinor, MusicalNote::A);
  EXPECT_EQ (harmonicMinor.toString (), QString ("A Harmonic Minor"));
}

TEST (MusicalScaleTest, PropertySignals)
{
  MusicalScale scale;

  // Test scaleTypeChanged signal
  QSignalSpy scaleTypeSpy (&scale, &MusicalScale::scaleTypeChanged);
  scale.setScaleType (MusicalScale::ScaleType::Dorian);
  EXPECT_EQ (scaleTypeSpy.count (), 1);
  EXPECT_EQ (
    scaleTypeSpy[0][0].value<MusicalScale::ScaleType> (),
    MusicalScale::ScaleType::Dorian);

  // Test rootKeyChanged signal
  QSignalSpy rootKeySpy (&scale, &MusicalScale::rootKeyChanged);
  scale.setRootKey (MusicalNote::E);
  EXPECT_EQ (rootKeySpy.count (), 1);
  EXPECT_EQ (rootKeySpy[0][0].value<MusicalNote> (), MusicalNote::E);

  // Test no signal when value doesn't change
  scale.setScaleType (MusicalScale::ScaleType::Dorian);
  EXPECT_EQ (scaleTypeSpy.count (), 1);

  scale.setRootKey (MusicalNote::E);
  EXPECT_EQ (rootKeySpy.count (), 1);
}

TEST (MusicalScaleTest, NoteContainmentEdgeCases)
{
  // Test chromatic scale contains all notes
  MusicalScale chromatic (MusicalScale::ScaleType::Chromatic, MusicalNote::C);
  for (int i = 0; i <= static_cast<int> (MusicalNote::B); i++)
    {
      EXPECT_TRUE (chromatic.containsNote (static_cast<MusicalNote> (i)));
    }

  // Test empty scale (defaults to A minor)
  MusicalScale empty;
  EXPECT_TRUE (empty.containsNote (MusicalNote::A));
  EXPECT_FALSE (empty.containsNote (MusicalNote::ASharp));

  // Test invalid note
  EXPECT_FALSE (chromatic.containsNote (static_cast<MusicalNote> (100)));
}

TEST (MusicalScaleTest, InitialState)
{
  MusicalScale scale;

  // Test default values
  EXPECT_EQ (scale.scaleType (), MusicalScale::ScaleType::Aeolian);
  EXPECT_EQ (scale.rootKey (), MusicalNote::A);
}
