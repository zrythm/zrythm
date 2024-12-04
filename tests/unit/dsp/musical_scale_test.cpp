#include "dsp/musical_scale.h"
#include "utils/gtest_wrapper.h"

using namespace zrythm::dsp;

TEST (MusicalScaleTest, BasicScales)
{
  // Test major scale
  MusicalScale major (MusicalScale::Type::Major, MusicalNote::C);
  // Notes in scale
  EXPECT_TRUE (major.contains_note (MusicalNote::C));
  EXPECT_TRUE (major.contains_note (MusicalNote::D));
  EXPECT_TRUE (major.contains_note (MusicalNote::E));
  EXPECT_TRUE (major.contains_note (MusicalNote::F));
  EXPECT_TRUE (major.contains_note (MusicalNote::G));
  EXPECT_TRUE (major.contains_note (MusicalNote::A));
  EXPECT_TRUE (major.contains_note (MusicalNote::B));
  // Notes not in scale
  EXPECT_FALSE (major.contains_note (MusicalNote::CSharp));
  EXPECT_FALSE (major.contains_note (MusicalNote::DSharp));
  EXPECT_FALSE (major.contains_note (MusicalNote::FSharp));
  EXPECT_FALSE (major.contains_note (MusicalNote::GSharp));
  EXPECT_FALSE (major.contains_note (MusicalNote::ASharp));
  EXPECT_EQ (major.to_string (), "C Major");

  // Test natural minor scale
  MusicalScale minor (MusicalScale::Type::Minor, MusicalNote::A);
  // Notes in scale
  EXPECT_TRUE (minor.contains_note (MusicalNote::A));
  EXPECT_TRUE (minor.contains_note (MusicalNote::B));
  EXPECT_TRUE (minor.contains_note (MusicalNote::C));
  EXPECT_TRUE (minor.contains_note (MusicalNote::D));
  EXPECT_TRUE (minor.contains_note (MusicalNote::E));
  EXPECT_TRUE (minor.contains_note (MusicalNote::F));
  EXPECT_TRUE (minor.contains_note (MusicalNote::G));
  // Notes not in scale
  EXPECT_FALSE (minor.contains_note (MusicalNote::GSharp));
  EXPECT_FALSE (minor.contains_note (MusicalNote::ASharp));
  EXPECT_FALSE (minor.contains_note (MusicalNote::CSharp));
  EXPECT_FALSE (minor.contains_note (MusicalNote::DSharp));
  EXPECT_FALSE (minor.contains_note (MusicalNote::FSharp));
  EXPECT_EQ (minor.to_string (), "A Minor");
}

TEST (MusicalScaleTest, ChordContainment)
{
  MusicalScale c_major (MusicalScale::Type::Major, MusicalNote::C);

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
  auto triads =
    MusicalScale::get_triad_types_for_type (MusicalScale::Type::Major, true);
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
  MusicalScale c_major (MusicalScale::Type::Major, MusicalNote::C);

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
    MusicalScale::Type::HarmonicMinor, MusicalNote::A);
  EXPECT_TRUE (harmonic_minor.contains_note (MusicalNote::A));
  EXPECT_TRUE (harmonic_minor.contains_note (MusicalNote::B));
  EXPECT_TRUE (harmonic_minor.contains_note (MusicalNote::C));
  EXPECT_TRUE (harmonic_minor.contains_note (MusicalNote::D));
  EXPECT_TRUE (harmonic_minor.contains_note (MusicalNote::E));
  EXPECT_TRUE (harmonic_minor.contains_note (MusicalNote::F));
  EXPECT_TRUE (harmonic_minor.contains_note (MusicalNote::GSharp));
  EXPECT_FALSE (harmonic_minor.contains_note (MusicalNote::G));

  // Test whole tone scale
  MusicalScale whole_tone (MusicalScale::Type::WholeTone, MusicalNote::C);
  EXPECT_TRUE (whole_tone.contains_note (MusicalNote::C));
  EXPECT_TRUE (whole_tone.contains_note (MusicalNote::D));
  EXPECT_TRUE (whole_tone.contains_note (MusicalNote::E));
  EXPECT_TRUE (whole_tone.contains_note (MusicalNote::FSharp));
  EXPECT_TRUE (whole_tone.contains_note (MusicalNote::GSharp));
  EXPECT_TRUE (whole_tone.contains_note (MusicalNote::ASharp));
}

TEST (MusicalScaleTest, Serialization)
{
  MusicalScale scale1 (MusicalScale::Type::Major, MusicalNote::C);

  // Serialize to JSON
  auto json_str = scale1.serialize_to_json_string ();

  // Create new object and deserialize
  MusicalScale scale2;
  scale2.deserialize_from_json_string (json_str.c_str ());

  // Verify all fields match
  EXPECT_EQ (scale1.type_, scale2.type_);
  EXPECT_EQ (scale1.root_key_, scale2.root_key_);
}
