// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/chord_pad_bank_operator.h"
#include "dsp/chord_preset.h"
#include "structure/project/chord_pad_bank.h"
#include "undo/undo_stack.h"
#include "utils/qt.h"

#include "unit/actions/mock_undo_stack.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{

class ChordPadBankOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    bank_ = std::make_unique<structure::project::ChordPadBank> ();
    undo_stack_ = create_mock_undo_stack ();
    operator_ = std::make_unique<ChordPadBankOperator> ();
    operator_->setChordPadBank (bank_.get ());
    operator_->setUndoStack (undo_stack_.get ());
  }

  static void expect_descriptor_equals (
    const dsp::ChordDescriptor &desc,
    dsp::MusicalNote            root,
    dsp::ChordType              type,
    dsp::ChordAccent            accent = dsp::ChordAccent::None,
    int                         inversion = 0,
    bool                        has_bass = false)
  {
    EXPECT_EQ (desc.rootNote (), root);
    EXPECT_EQ (desc.chordType (), type);
    EXPECT_EQ (desc.chordAccent (), accent);
    EXPECT_EQ (desc.inversion (), inversion);
    EXPECT_EQ (desc.hasBass (), has_bass);
  }

  std::unique_ptr<structure::project::ChordPadBank> bank_;
  std::unique_ptr<undo::UndoStack>                  undo_stack_;
  std::unique_ptr<ChordPadBankOperator>             operator_;
};

// --- editPad ---

TEST_F (ChordPadBankOperatorTest, EditPadChangesDescriptor)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  ASSERT_EQ (bank_->rowCount (), 1);

  operator_->editPad (
    0, dsp::MusicalNote::A, dsp::ChordType::Minor, dsp::ChordAccent::Seventh, 1,
    true, dsp::MusicalNote::D);

  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::A, dsp::ChordType::Minor,
    dsp::ChordAccent::Seventh, 1, true);
  EXPECT_EQ (bank_->chordAt (0)->bassNote (), dsp::MusicalNote::D);
}

TEST_F (ChordPadBankOperatorTest, EditPadIsUndoable)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  operator_->editPad (
    0, dsp::MusicalNote::D, dsp::ChordType::Minor, dsp::ChordAccent::None, 0,
    false, dsp::MusicalNote::C);

  ASSERT_TRUE (undo_stack_->canUndo ());
  undo_stack_->undo ();
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

// --- addPad ---

TEST_F (ChordPadBankOperatorTest, AddPadAppendsDescriptor)
{
  operator_->addPad (
    dsp::MusicalNote::E, dsp::ChordType::Minor, dsp::ChordAccent::Ninth, -1,
    true, dsp::MusicalNote::G);

  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::E, dsp::ChordType::Minor,
    dsp::ChordAccent::Ninth, -1, true);
  EXPECT_EQ (bank_->chordAt (0)->bassNote (), dsp::MusicalNote::G);
}

TEST_F (ChordPadBankOperatorTest, AddPadDefaultsHaveNoAccentNoBass)
{
  operator_->addPad (dsp::MusicalNote::C, dsp::ChordType::Major);

  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

TEST_F (ChordPadBankOperatorTest, AddPadIsUndoable)
{
  operator_->addPad (dsp::MusicalNote::C, dsp::ChordType::Major);
  ASSERT_EQ (bank_->rowCount (), 1);

  undo_stack_->undo ();
  EXPECT_EQ (bank_->rowCount (), 0);
}

// --- removePad ---

TEST_F (ChordPadBankOperatorTest, RemovePadDeletesAtindex)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank_->addChord (dsp::MusicalNote::D, dsp::ChordType::Minor);
  ASSERT_EQ (bank_->rowCount (), 2);

  operator_->removePad (0);

  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::D, dsp::ChordType::Minor);
}

TEST_F (ChordPadBankOperatorTest, RemovePadIsUndoable)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  operator_->removePad (0);
  EXPECT_EQ (bank_->rowCount (), 0);

  undo_stack_->undo ();
  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

// --- movePad ---

TEST_F (ChordPadBankOperatorTest, MovePadReorders)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank_->addChord (dsp::MusicalNote::D, dsp::ChordType::Minor);
  bank_->addChord (dsp::MusicalNote::E, dsp::ChordType::Diminished);

  operator_->movePad (0, 2);

  ASSERT_EQ (bank_->rowCount (), 3);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::D, dsp::ChordType::Minor);
  expect_descriptor_equals (
    *bank_->chordAt (1), dsp::MusicalNote::E, dsp::ChordType::Diminished);
  expect_descriptor_equals (
    *bank_->chordAt (2), dsp::MusicalNote::C, dsp::ChordType::Major);
}

TEST_F (ChordPadBankOperatorTest, MovePadIsUndoable)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank_->addChord (dsp::MusicalNote::D, dsp::ChordType::Minor);

  operator_->movePad (0, 1);
  undo_stack_->undo ();

  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank_->chordAt (1), dsp::MusicalNote::D, dsp::ChordType::Minor);
}

// --- transposePads ---

TEST_F (ChordPadBankOperatorTest, TransposeUpShiftsRootNotes)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank_->addChord (dsp::MusicalNote::E, dsp::ChordType::Minor);

  operator_->transposePads (true);

  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::CSharp, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank_->chordAt (1), dsp::MusicalNote::F, dsp::ChordType::Minor);
}

TEST_F (ChordPadBankOperatorTest, TransposeUpWrapsAround)
{
  bank_->addChord (dsp::MusicalNote::B, dsp::ChordType::Major);

  operator_->transposePads (true);

  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

TEST_F (ChordPadBankOperatorTest, TransposeDownShiftsRootNotes)
{
  bank_->addChord (dsp::MusicalNote::D, dsp::ChordType::Major);

  operator_->transposePads (false);

  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::CSharp, dsp::ChordType::Major);
}

TEST_F (ChordPadBankOperatorTest, TransposeDownWrapsAround)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);

  // Down from C wraps to B (11 semitones down mod 12).
  operator_->transposePads (false);

  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::B, dsp::ChordType::Major);
}

TEST_F (ChordPadBankOperatorTest, TransposeShiftsBassNote)
{
  bank_->addChord (
    dsp::MusicalNote::C, dsp::ChordType::Major, dsp::ChordAccent::None, 0,
    dsp::MusicalNote::G);

  operator_->transposePads (true);

  ASSERT_TRUE (bank_->chordAt (0)->hasBass ());
  EXPECT_EQ (bank_->chordAt (0)->bassNote (), dsp::MusicalNote::GSharp);
}

TEST_F (ChordPadBankOperatorTest, TransposeEmptyBankIsNoOp)
{
  operator_->transposePads (true);

  EXPECT_EQ (bank_->rowCount (), 0);
  EXPECT_FALSE (undo_stack_->canUndo ());
}

TEST_F (ChordPadBankOperatorTest, TransposeIsUndoable)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  operator_->transposePads (true);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::CSharp, dsp::ChordType::Major);

  undo_stack_->undo ();
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

// --- applyPresetFromScale ---

TEST_F (ChordPadBankOperatorTest, ApplyPresetFromScalePopulatesDiatonicTriads)
{
  // C Major scale: notes C D E F G A B (7 notes).
  operator_->applyPresetFromScale (
    dsp::MusicalScale::ScaleType::Major, dsp::MusicalNote::C);

  ASSERT_EQ (bank_->rowCount (), 7);

  // Diatonic triads for a major scale:
  // I=Major, ii=Minor, iii=Minor, IV=Major, V=Major, vi=Minor, vii°=Diminished
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank_->chordAt (1), dsp::MusicalNote::D, dsp::ChordType::Minor);
  expect_descriptor_equals (
    *bank_->chordAt (2), dsp::MusicalNote::E, dsp::ChordType::Minor);
  expect_descriptor_equals (
    *bank_->chordAt (3), dsp::MusicalNote::F, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank_->chordAt (4), dsp::MusicalNote::G, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank_->chordAt (5), dsp::MusicalNote::A, dsp::ChordType::Minor);
  expect_descriptor_equals (
    *bank_->chordAt (6), dsp::MusicalNote::B, dsp::ChordType::Diminished);
}

TEST_F (ChordPadBankOperatorTest, ApplyPresetFromScaleOffsetsByRoot)
{
  // D Major scale starts on D.
  operator_->applyPresetFromScale (
    dsp::MusicalScale::ScaleType::Major, dsp::MusicalNote::D);

  ASSERT_EQ (bank_->rowCount (), 7);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::D, dsp::ChordType::Major);
}

TEST_F (ChordPadBankOperatorTest, ApplyPresetFromScaleIsUndoable)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  operator_->applyPresetFromScale (
    dsp::MusicalScale::ScaleType::Major, dsp::MusicalNote::C);
  ASSERT_EQ (bank_->rowCount (), 7);

  undo_stack_->undo ();
  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

// --- applyPreset ---

TEST_F (ChordPadBankOperatorTest, ApplyPresetReplacesWithPresetDescriptors)
{
  ChordPreset preset;
  preset.addDescriptor (
    utils::make_qobject_unique<dsp::ChordDescriptor> (
      dsp::MusicalNote::A, dsp::ChordType::Minor, dsp::ChordAccent::Seventh));
  preset.addDescriptor (
    utils::make_qobject_unique<dsp::ChordDescriptor> (
      dsp::MusicalNote::F, dsp::ChordType::Major));

  operator_->applyPreset (&preset);

  ASSERT_EQ (bank_->rowCount (), 2);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::A, dsp::ChordType::Minor,
    dsp::ChordAccent::Seventh);
  expect_descriptor_equals (
    *bank_->chordAt (1), dsp::MusicalNote::F, dsp::ChordType::Major);
}

TEST_F (ChordPadBankOperatorTest, ApplyPresetNullIsNoOp)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);

  operator_->applyPreset (nullptr);

  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
  EXPECT_FALSE (undo_stack_->canUndo ());
}

TEST_F (ChordPadBankOperatorTest, ApplyPresetIsUndoable)
{
  bank_->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);

  ChordPreset preset;
  preset.addDescriptor (
    utils::make_qobject_unique<dsp::ChordDescriptor> (
      dsp::MusicalNote::G, dsp::ChordType::Major));

  operator_->applyPreset (&preset);
  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::G, dsp::ChordType::Major);

  undo_stack_->undo ();
  ASSERT_EQ (bank_->rowCount (), 1);
  expect_descriptor_equals (
    *bank_->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

} // namespace zrythm::actions
