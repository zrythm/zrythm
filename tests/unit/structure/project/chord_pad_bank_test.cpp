// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/musical_scale.h"
#include "structure/project/chord_pad_bank.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace zrythm::structure::project
{

using namespace zrythm::dsp;

class ChordPadBankTest : public ::testing::Test
{
protected:
  void SetUp () override { bank_ = std::make_unique<ChordPadBank> (); }

  void TearDown () override { bank_.reset (); }

  static void expect_chord_matches (
    const ChordDescriptor     &chord,
    MusicalNote                expected_root,
    ChordType                  expected_type,
    ChordAccent                expected_accent = ChordAccent::None,
    int                        expected_inversion = 0,
    std::optional<MusicalNote> expected_bass = std::nullopt)
  {
    EXPECT_EQ (chord.rootNote (), expected_root);
    EXPECT_EQ (chord.chordType (), expected_type);
    EXPECT_EQ (chord.chordAccent (), expected_accent);
    EXPECT_EQ (chord.inversion (), expected_inversion);
    EXPECT_EQ (chord.hasBass (), expected_bass.has_value ());
    if (expected_bass)
      EXPECT_EQ (chord.bassNote (), *expected_bass);
  }

  std::unique_ptr<ChordPadBank> bank_;
};

TEST_F (ChordPadBankTest, InitiallyEmpty)
{
  EXPECT_EQ (bank_->rowCount (), 0);
}

TEST_F (ChordPadBankTest, AddChordIncrementsRowCount)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  EXPECT_EQ (bank_->rowCount (), 1);

  bank_->addChord (MusicalNote::D, ChordType::Minor);
  EXPECT_EQ (bank_->rowCount (), 2);
}

TEST_F (ChordPadBankTest, AddChordStoresCorrectProperties)
{
  bank_->addChord (
    MusicalNote::G, ChordType::Minor, ChordAccent::Seventh, 2, MusicalNote::D);
  auto * chord = bank_->chord_at (0);
  ASSERT_NE (chord, nullptr);
  expect_chord_matches (
    *chord, MusicalNote::G, ChordType::Minor, ChordAccent::Seventh, 2,
    MusicalNote::D);
}

TEST_F (ChordPadBankTest, AddChordWithoutOptionalParams)
{
  bank_->addChord (MusicalNote::A, ChordType::Diminished);
  auto * chord = bank_->chord_at (0);
  ASSERT_NE (chord, nullptr);
  expect_chord_matches (*chord, MusicalNote::A, ChordType::Diminished);
  EXPECT_FALSE (chord->hasBass ());
}

TEST_F (ChordPadBankTest, DataReturnsChordDescriptor)
{
  bank_->addChord (MusicalNote::E, ChordType::Augmented);
  auto index = bank_->index (0);
  auto variant = bank_->data (index, ChordPadBank::ChordDescriptorRole);
  ASSERT_TRUE (variant.isValid ());
  auto * chord = variant.value<ChordDescriptor *> ();
  ASSERT_NE (chord, nullptr);
  EXPECT_EQ (chord->rootNote (), MusicalNote::E);
  EXPECT_EQ (chord->chordType (), ChordType::Augmented);
}

TEST_F (ChordPadBankTest, DataInvalidIndexReturnsInvalid)
{
  auto index = bank_->index (99);
  auto variant = bank_->data (index, ChordPadBank::ChordDescriptorRole);
  EXPECT_FALSE (variant.isValid ());
}

TEST_F (ChordPadBankTest, DataInvalidRoleReturnsInvalid)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  auto index = bank_->index (0);
  auto variant = bank_->data (index, Qt::DisplayRole);
  EXPECT_FALSE (variant.isValid ());
}

TEST_F (ChordPadBankTest, RemoveChordDecrementsCount)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::D, ChordType::Minor);
  bank_->addChord (MusicalNote::E, ChordType::Augmented);

  bank_->removeChord (1);
  EXPECT_EQ (bank_->rowCount (), 2);

  EXPECT_EQ (bank_->chord_at (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chord_at (1)->rootNote (), MusicalNote::E);
}

TEST_F (ChordPadBankTest, RemoveChordInvalidIndexIsNoOp)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->removeChord (-1);
  bank_->removeChord (5);
  EXPECT_EQ (bank_->rowCount (), 1);
}

TEST_F (ChordPadBankTest, TransposeUp)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::G, ChordType::Major);
  bank_->transposeChords (true);

  EXPECT_EQ (bank_->chord_at (0)->rootNote (), MusicalNote::CSharp);
  EXPECT_EQ (bank_->chord_at (1)->rootNote (), MusicalNote::GSharp);
}

TEST_F (ChordPadBankTest, TransposeDown)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->transposeChords (false);
  EXPECT_EQ (bank_->chord_at (0)->rootNote (), MusicalNote::B);
}

TEST_F (ChordPadBankTest, TransposeWrapsAround)
{
  bank_->addChord (MusicalNote::B, ChordType::Major);
  bank_->transposeChords (true);
  EXPECT_EQ (bank_->chord_at (0)->rootNote (), MusicalNote::C);
}

TEST_F (ChordPadBankTest, TransposeAlsoTransposesBass)
{
  bank_->addChord (
    MusicalNote::C, ChordType::Major, ChordAccent::None, 0, MusicalNote::G);
  bank_->transposeChords (true);

  EXPECT_EQ (bank_->chord_at (0)->rootNote (), MusicalNote::CSharp);
  EXPECT_EQ (bank_->chord_at (0)->bassNote (), MusicalNote::GSharp);
}

TEST_F (ChordPadBankTest, TransposeEmptyBankIsNoOp)
{
  bank_->transposeChords (true);
  EXPECT_EQ (bank_->rowCount (), 0);
}

TEST_F (ChordPadBankTest, GetChordFromNoteNumberInRange)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::D, ChordType::Minor);

  auto * chord0 = bank_->get_chord_from_note_number (60);
  ASSERT_NE (chord0, nullptr);
  EXPECT_EQ (chord0->rootNote (), MusicalNote::C);

  auto * chord1 = bank_->get_chord_from_note_number (61);
  ASSERT_NE (chord1, nullptr);
  EXPECT_EQ (chord1->rootNote (), MusicalNote::D);
}

TEST_F (ChordPadBankTest, GetChordFromNoteNumberOutOfRange)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);

  EXPECT_EQ (bank_->get_chord_from_note_number (59), nullptr);
  EXPECT_EQ (bank_->get_chord_from_note_number (72), nullptr);
  EXPECT_EQ (bank_->get_chord_from_note_number (0), nullptr);
  EXPECT_EQ (bank_->get_chord_from_note_number (127), nullptr);
}

TEST_F (ChordPadBankTest, GetChordFromNoteNumberBeyondChordCount)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);

  EXPECT_NE (bank_->get_chord_from_note_number (60), nullptr);
  EXPECT_EQ (bank_->get_chord_from_note_number (61), nullptr);
}

TEST_F (ChordPadBankTest, ApplyPresetFromScaleCMajor)
{
  bank_->applyPresetFromScale (MusicalScale::ScaleType::Ionian, MusicalNote::C);

  EXPECT_EQ (bank_->rowCount (), 7);
  expect_chord_matches (*bank_->chord_at (0), MusicalNote::C, ChordType::Major);
  expect_chord_matches (*bank_->chord_at (1), MusicalNote::D, ChordType::Minor);
  expect_chord_matches (*bank_->chord_at (2), MusicalNote::E, ChordType::Minor);
  expect_chord_matches (*bank_->chord_at (3), MusicalNote::F, ChordType::Major);
  expect_chord_matches (*bank_->chord_at (4), MusicalNote::G, ChordType::Major);
  expect_chord_matches (*bank_->chord_at (5), MusicalNote::A, ChordType::Minor);
  expect_chord_matches (
    *bank_->chord_at (6), MusicalNote::B, ChordType::Diminished);
}

TEST_F (ChordPadBankTest, ApplyPresetReplacesExistingChords)
{
  bank_->addChord (MusicalNote::C, ChordType::Augmented);
  bank_->addChord (MusicalNote::F, ChordType::Augmented);

  bank_->applyPresetFromScale (MusicalScale::ScaleType::Ionian, MusicalNote::C);

  EXPECT_EQ (bank_->rowCount (), 7);
  EXPECT_EQ (bank_->chord_at (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chord_at (0)->chordType (), ChordType::Major);
}

TEST_F (ChordPadBankTest, ApplyPresetFromMinorScale)
{
  bank_->applyPresetFromScale (MusicalScale::ScaleType::Aeolian, MusicalNote::A);

  EXPECT_EQ (bank_->rowCount (), 7);
  expect_chord_matches (*bank_->chord_at (0), MusicalNote::A, ChordType::Minor);
  expect_chord_matches (
    *bank_->chord_at (1), MusicalNote::B, ChordType::Diminished);
  expect_chord_matches (*bank_->chord_at (2), MusicalNote::C, ChordType::Major);
  expect_chord_matches (*bank_->chord_at (3), MusicalNote::D, ChordType::Minor);
  expect_chord_matches (*bank_->chord_at (4), MusicalNote::E, ChordType::Minor);
  expect_chord_matches (*bank_->chord_at (5), MusicalNote::F, ChordType::Major);
  expect_chord_matches (*bank_->chord_at (6), MusicalNote::G, ChordType::Major);
}

TEST_F (ChordPadBankTest, JsonRoundTripEmpty)
{
  nlohmann::json j;
  to_json (j, *bank_);

  auto restored = std::make_unique<ChordPadBank> ();
  from_json (j, *restored);

  EXPECT_EQ (restored->rowCount (), 0);
}

TEST_F (ChordPadBankTest, JsonRoundTripWithChords)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (
    MusicalNote::D, ChordType::Minor, ChordAccent::Seventh, 1, MusicalNote::A);

  nlohmann::json j;
  to_json (j, *bank_);

  auto restored = std::make_unique<ChordPadBank> ();
  from_json (j, *restored);

  ASSERT_EQ (restored->rowCount (), 2);
  expect_chord_matches (
    *restored->chord_at (0), MusicalNote::C, ChordType::Major);
  expect_chord_matches (
    *restored->chord_at (1), MusicalNote::D, ChordType::Minor,
    ChordAccent::Seventh, 1, MusicalNote::A);
}

TEST_F (ChordPadBankTest, JsonRoundTripPreservesChordCount)
{
  bank_->applyPresetFromScale (MusicalScale::ScaleType::Ionian, MusicalNote::C);

  nlohmann::json j;
  to_json (j, *bank_);

  auto restored = std::make_unique<ChordPadBank> ();
  from_json (j, *restored);

  EXPECT_EQ (restored->rowCount (), bank_->rowCount ());
  for (int i = 0; i < bank_->rowCount (); ++i)
    {
      EXPECT_EQ (
        bank_->chord_at (i)->rootNote (), restored->chord_at (i)->rootNote ());
      EXPECT_EQ (
        bank_->chord_at (i)->chordType (), restored->chord_at (i)->chordType ());
    }
}

TEST_F (ChordPadBankTest, JsonFromEmptyObjectIsNoOp)
{
  nlohmann::json j = nlohmann::json::object ();
  auto           restored = std::make_unique<ChordPadBank> ();
  EXPECT_NO_THROW (from_json (j, *restored));
  EXPECT_EQ (restored->rowCount (), 0);
}

TEST_F (ChordPadBankTest, RoleNamesContainsChordDescriptor)
{
  auto roles = bank_->roleNames ();
  ASSERT_TRUE (roles.contains (ChordPadBank::ChordDescriptorRole));
  EXPECT_EQ (roles[ChordPadBank::ChordDescriptorRole], "chordDescriptor");
}

}
