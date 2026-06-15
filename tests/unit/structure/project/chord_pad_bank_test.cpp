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
      {
        EXPECT_EQ (chord.bassNote (), *expected_bass);
      }
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
  auto * chord = bank_->chordAt (0);
  ASSERT_NE (chord, nullptr);
  expect_chord_matches (
    *chord, MusicalNote::G, ChordType::Minor, ChordAccent::Seventh, 2,
    MusicalNote::D);
}

TEST_F (ChordPadBankTest, AddChordWithoutOptionalParams)
{
  bank_->addChord (MusicalNote::A, ChordType::Diminished);
  auto * chord = bank_->chordAt (0);
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

  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::E);
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

  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::CSharp);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::GSharp);
}

TEST_F (ChordPadBankTest, TransposeDown)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->transposeChords (false);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::B);
}

TEST_F (ChordPadBankTest, TransposeWrapsAround)
{
  bank_->addChord (MusicalNote::B, ChordType::Major);
  bank_->transposeChords (true);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
}

TEST_F (ChordPadBankTest, TransposeAlsoTransposesBass)
{
  bank_->addChord (
    MusicalNote::C, ChordType::Major, ChordAccent::None, 0, MusicalNote::G);
  bank_->transposeChords (true);

  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::CSharp);
  EXPECT_EQ (bank_->chordAt (0)->bassNote (), MusicalNote::GSharp);
}

TEST_F (ChordPadBankTest, TransposeEmptyBankIsNoOp)
{
  bank_->transposeChords (true);
  EXPECT_EQ (bank_->rowCount (), 0);
}

// --- moveChord tests ---

TEST_F (ChordPadBankTest, MoveChordSameIndexIsNoOp)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::D, ChordType::Minor);
  bank_->addChord (MusicalNote::E, ChordType::Diminished);

  bank_->moveChord (1, 1);
  ASSERT_EQ (bank_->rowCount (), 3);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::D);
  EXPECT_EQ (bank_->chordAt (2)->rootNote (), MusicalNote::E);
}

TEST_F (ChordPadBankTest, MoveChordInvalidIndicesAreNoOp)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);

  bank_->moveChord (-1, 0);
  bank_->moveChord (0, 99);
  bank_->moveChord (99, 0);
  bank_->moveChord (-1, -1);
  ASSERT_EQ (bank_->rowCount (), 1);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
}

TEST_F (ChordPadBankTest, MoveChordForward)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::D, ChordType::Minor);
  bank_->addChord (MusicalNote::E, ChordType::Diminished);

  bank_->moveChord (0, 2);
  ASSERT_EQ (bank_->rowCount (), 3);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::D);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::E);
  EXPECT_EQ (bank_->chordAt (2)->rootNote (), MusicalNote::C);
}

TEST_F (ChordPadBankTest, MoveChordBackward)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::D, ChordType::Minor);
  bank_->addChord (MusicalNote::E, ChordType::Diminished);

  bank_->moveChord (2, 0);
  ASSERT_EQ (bank_->rowCount (), 3);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::E);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chordAt (2)->rootNote (), MusicalNote::D);
}

TEST_F (ChordPadBankTest, MoveChordOnEmptyBankIsNoOp)
{
  bank_->moveChord (0, 1);
  EXPECT_EQ (bank_->rowCount (), 0);
}

// --- insertChord tests ---

TEST_F (ChordPadBankTest, InsertChordAtStart)
{
  bank_->addChord (MusicalNote::D, ChordType::Minor);
  bank_->insertChord (0, MusicalNote::C, ChordType::Major);

  ASSERT_EQ (bank_->rowCount (), 2);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::D);
}

TEST_F (ChordPadBankTest, InsertChordInMiddle)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::E, ChordType::Diminished);
  bank_->insertChord (1, MusicalNote::D, ChordType::Minor);

  ASSERT_EQ (bank_->rowCount (), 3);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::D);
  EXPECT_EQ (bank_->chordAt (2)->rootNote (), MusicalNote::E);
}

TEST_F (ChordPadBankTest, InsertChordAtEnd)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->insertChord (bank_->rowCount (), MusicalNote::D, ChordType::Minor);

  ASSERT_EQ (bank_->rowCount (), 2);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chordAt (1)->rootNote (), MusicalNote::D);
}

TEST_F (ChordPadBankTest, InsertChordWithBassAndInversion)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->insertChord (
    0, MusicalNote::G, ChordType::Augmented, ChordAccent::Seventh, 2,
    MusicalNote::B);

  ASSERT_EQ (bank_->rowCount (), 2);
  expect_chord_matches (
    *bank_->chordAt (0), MusicalNote::G, ChordType::Augmented,
    ChordAccent::Seventh, 2, MusicalNote::B);
}

TEST_F (ChordPadBankTest, InsertChordNegativeIndexThrows)
{
  EXPECT_THROW (
    bank_->insertChord (-1, MusicalNote::C, ChordType::Major),
    std::out_of_range);
}

TEST_F (ChordPadBankTest, InsertChordIndexBeyondSizeThrows)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  EXPECT_THROW (
    bank_->insertChord (2, MusicalNote::D, ChordType::Minor), std::out_of_range);
}

TEST_F (ChordPadBankTest, GetPitchesForNoteInRange)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);
  bank_->addChord (MusicalNote::D, ChordType::Minor);

  auto pitches0 = bank_->get_pitches_for_note (60);
  ASSERT_TRUE (pitches0.has_value ());
  EXPECT_FALSE (pitches0->empty ());

  auto pitches1 = bank_->get_pitches_for_note (61);
  ASSERT_TRUE (pitches1.has_value ());
  EXPECT_FALSE (pitches1->empty ());
}

TEST_F (ChordPadBankTest, GetPitchesForNoteOutOfRange)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);

  EXPECT_FALSE (bank_->get_pitches_for_note (59).has_value ());
  EXPECT_FALSE (bank_->get_pitches_for_note (72).has_value ());
  EXPECT_FALSE (bank_->get_pitches_for_note (0).has_value ());
  EXPECT_FALSE (bank_->get_pitches_for_note (127).has_value ());
}

TEST_F (ChordPadBankTest, GetPitchesForNoteBeyondChordCount)
{
  bank_->addChord (MusicalNote::C, ChordType::Major);

  EXPECT_TRUE (bank_->get_pitches_for_note (60).has_value ());
  EXPECT_FALSE (bank_->get_pitches_for_note (61).has_value ());
}

TEST_F (ChordPadBankTest, ApplyPresetFromScaleCMajor)
{
  bank_->applyPresetFromScale (MusicalScale::ScaleType::Ionian, MusicalNote::C);

  EXPECT_EQ (bank_->rowCount (), 7);
  expect_chord_matches (*bank_->chordAt (0), MusicalNote::C, ChordType::Major);
  expect_chord_matches (*bank_->chordAt (1), MusicalNote::D, ChordType::Minor);
  expect_chord_matches (*bank_->chordAt (2), MusicalNote::E, ChordType::Minor);
  expect_chord_matches (*bank_->chordAt (3), MusicalNote::F, ChordType::Major);
  expect_chord_matches (*bank_->chordAt (4), MusicalNote::G, ChordType::Major);
  expect_chord_matches (*bank_->chordAt (5), MusicalNote::A, ChordType::Minor);
  expect_chord_matches (
    *bank_->chordAt (6), MusicalNote::B, ChordType::Diminished);
}

TEST_F (ChordPadBankTest, ApplyPresetReplacesExistingChords)
{
  bank_->addChord (MusicalNote::C, ChordType::Augmented);
  bank_->addChord (MusicalNote::F, ChordType::Augmented);

  bank_->applyPresetFromScale (MusicalScale::ScaleType::Ionian, MusicalNote::C);

  EXPECT_EQ (bank_->rowCount (), 7);
  EXPECT_EQ (bank_->chordAt (0)->rootNote (), MusicalNote::C);
  EXPECT_EQ (bank_->chordAt (0)->chordType (), ChordType::Major);
}

TEST_F (ChordPadBankTest, ApplyPresetFromMinorScale)
{
  bank_->applyPresetFromScale (MusicalScale::ScaleType::Aeolian, MusicalNote::A);

  EXPECT_EQ (bank_->rowCount (), 7);
  expect_chord_matches (*bank_->chordAt (0), MusicalNote::A, ChordType::Minor);
  expect_chord_matches (
    *bank_->chordAt (1), MusicalNote::B, ChordType::Diminished);
  expect_chord_matches (*bank_->chordAt (2), MusicalNote::C, ChordType::Major);
  expect_chord_matches (*bank_->chordAt (3), MusicalNote::D, ChordType::Minor);
  expect_chord_matches (*bank_->chordAt (4), MusicalNote::E, ChordType::Minor);
  expect_chord_matches (*bank_->chordAt (5), MusicalNote::F, ChordType::Major);
  expect_chord_matches (*bank_->chordAt (6), MusicalNote::G, ChordType::Major);
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
    *restored->chordAt (0), MusicalNote::C, ChordType::Major);
  expect_chord_matches (
    *restored->chordAt (1), MusicalNote::D, ChordType::Minor,
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
        bank_->chordAt (i)->rootNote (), restored->chordAt (i)->rootNote ());
      EXPECT_EQ (
        bank_->chordAt (i)->chordType (), restored->chordAt (i)->chordType ());
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
