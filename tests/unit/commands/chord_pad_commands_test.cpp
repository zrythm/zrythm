// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/chord_pad_commands.h"
#include "structure/project/chord_pad_bank.h"
#include "utils/qt.h"

#include <gtest/gtest.h>

namespace zrythm::commands
{

class ChordPadCommandsTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    bank = std::make_unique<structure::project::ChordPadBank> ();
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

  std::unique_ptr<structure::project::ChordPadBank> bank;
};

// --- ChordPadState ---

TEST_F (ChordPadCommandsTest, PadStateFromDescriptor)
{
  dsp::ChordDescriptor desc (
    dsp::MusicalNote::A, dsp::ChordType::Minor, dsp::ChordAccent::Seventh, 2,
    dsp::MusicalNote::D);
  auto state = ChordPadState::from_descriptor (desc);
  EXPECT_EQ (state.rootNote, dsp::MusicalNote::A);
  EXPECT_EQ (state.type, dsp::ChordType::Minor);
  EXPECT_EQ (state.accent, dsp::ChordAccent::Seventh);
  EXPECT_EQ (state.inversion, 2);
  ASSERT_TRUE (state.bass.has_value ());
  EXPECT_EQ (*state.bass, dsp::MusicalNote::D);
}

TEST_F (ChordPadCommandsTest, PadStateFromDescriptorNoBass)
{
  dsp::ChordDescriptor desc (dsp::MusicalNote::C, dsp::ChordType::Major);
  auto                 state = ChordPadState::from_descriptor (desc);
  EXPECT_FALSE (state.bass.has_value ());
}

TEST_F (ChordPadCommandsTest, PadStateApplyTo)
{
  ChordPadState state{
    dsp::MusicalNote::E, dsp::ChordType::Diminished, dsp::ChordAccent::Ninth,
    -1, dsp::MusicalNote::G
  };
  dsp::ChordDescriptor desc;
  state.apply_to (desc);
  expect_descriptor_equals (
    desc, dsp::MusicalNote::E, dsp::ChordType::Diminished,
    dsp::ChordAccent::Ninth, -1, true);
  EXPECT_EQ (desc.bassNote (), dsp::MusicalNote::G);
}

TEST_F (ChordPadCommandsTest, PadStateApplyToClearsBass)
{
  ChordPadState state{
    dsp::MusicalNote::C, dsp::ChordType::Major, dsp::ChordAccent::None, 0,
    std::nullopt
  };
  dsp::ChordDescriptor desc;
  desc.setBassNote (dsp::MusicalNote::G);
  ASSERT_TRUE (desc.hasBass ());
  state.apply_to (desc);
  EXPECT_FALSE (desc.hasBass ());
}

TEST_F (ChordPadCommandsTest, PadStateEquality)
{
  ChordPadState a{
    dsp::MusicalNote::C, dsp::ChordType::Major, dsp::ChordAccent::None, 0,
    std::nullopt
  };
  ChordPadState b{
    dsp::MusicalNote::C, dsp::ChordType::Major, dsp::ChordAccent::None, 0,
    std::nullopt
  };
  ChordPadState c{
    dsp::MusicalNote::D, dsp::ChordType::Major, dsp::ChordAccent::None, 0,
    std::nullopt
  };
  EXPECT_EQ (a, b);
  EXPECT_NE (a, c);
}

// --- EditChordPadCommand ---

TEST_F (ChordPadCommandsTest, EditPadRedoUndo)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  ASSERT_EQ (bank->rowCount (), 1);

  ChordPadState after{
    dsp::MusicalNote::A, dsp::ChordType::Minor, dsp::ChordAccent::Seventh, 1,
    dsp::MusicalNote::D
  };
  EditChordPadCommand cmd (*bank, 0, after);

  cmd.redo ();
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::A, dsp::ChordType::Minor,
    dsp::ChordAccent::Seventh, 1, true);
  EXPECT_EQ (bank->chordAt (0)->bassNote (), dsp::MusicalNote::D);

  cmd.undo ();
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

TEST_F (ChordPadCommandsTest, EditPadMultipleCycles)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);

  ChordPadState       after{ dsp::MusicalNote::D, dsp::ChordType::Minor };
  EditChordPadCommand cmd (*bank, 0, after);

  for (int i = 0; i < 3; ++i)
    {
      cmd.redo ();
      expect_descriptor_equals (
        *bank->chordAt (0), dsp::MusicalNote::D, dsp::ChordType::Minor);
      cmd.undo ();
      expect_descriptor_equals (
        *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
    }
}

// --- AddChordPadCommand ---

TEST_F (ChordPadCommandsTest, AddPadRedoUndo)
{
  ChordPadState      state{ dsp::MusicalNote::E, dsp::ChordType::Minor };
  AddChordPadCommand cmd (*bank, state);

  cmd.redo ();
  ASSERT_EQ (bank->rowCount (), 1);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::E, dsp::ChordType::Minor);

  cmd.undo ();
  EXPECT_EQ (bank->rowCount (), 0);
}

TEST_F (ChordPadCommandsTest, AddPadMultiple)
{
  ChordPadState      s1{ dsp::MusicalNote::C, dsp::ChordType::Major };
  ChordPadState      s2{ dsp::MusicalNote::D, dsp::ChordType::Minor };
  AddChordPadCommand cmd1 (*bank, s1);
  AddChordPadCommand cmd2 (*bank, s2);

  cmd1.redo ();
  cmd2.redo ();
  ASSERT_EQ (bank->rowCount (), 2);

  cmd2.undo ();
  ASSERT_EQ (bank->rowCount (), 1);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);

  cmd1.undo ();
  EXPECT_EQ (bank->rowCount (), 0);
}

// --- RemoveChordPadCommand ---

TEST_F (ChordPadCommandsTest, RemovePadRedoUndo)
{
  bank->addChord (
    dsp::MusicalNote::G, dsp::ChordType::Augmented, dsp::ChordAccent::None, 2,
    dsp::MusicalNote::B);
  ASSERT_EQ (bank->rowCount (), 1);

  RemoveChordPadCommand cmd (*bank, 0);
  cmd.redo ();
  EXPECT_EQ (bank->rowCount (), 0);

  cmd.undo ();
  ASSERT_EQ (bank->rowCount (), 1);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::G, dsp::ChordType::Augmented,
    dsp::ChordAccent::None, 2, true);
  EXPECT_EQ (bank->chordAt (0)->bassNote (), dsp::MusicalNote::B);
}

TEST_F (ChordPadCommandsTest, RemoveMiddlePadRestoresCorrectly)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank->addChord (dsp::MusicalNote::D, dsp::ChordType::Minor);
  bank->addChord (dsp::MusicalNote::E, dsp::ChordType::Diminished);
  ASSERT_EQ (bank->rowCount (), 3);

  RemoveChordPadCommand cmd (*bank, 1);
  cmd.redo ();
  ASSERT_EQ (bank->rowCount (), 2);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank->chordAt (1), dsp::MusicalNote::E, dsp::ChordType::Diminished);

  cmd.undo ();
  ASSERT_EQ (bank->rowCount (), 3);
  expect_descriptor_equals (
    *bank->chordAt (1), dsp::MusicalNote::D, dsp::ChordType::Minor);
}

// --- MoveChordPadCommand ---

TEST_F (ChordPadCommandsTest, MovePadForwardRedoUndo)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank->addChord (dsp::MusicalNote::D, dsp::ChordType::Minor);
  bank->addChord (dsp::MusicalNote::E, dsp::ChordType::Diminished);

  MoveChordPadCommand cmd (*bank, 0, 2);
  cmd.redo ();
  ASSERT_EQ (bank->rowCount (), 3);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::D, dsp::ChordType::Minor);
  expect_descriptor_equals (
    *bank->chordAt (1), dsp::MusicalNote::E, dsp::ChordType::Diminished);
  expect_descriptor_equals (
    *bank->chordAt (2), dsp::MusicalNote::C, dsp::ChordType::Major);

  cmd.undo ();
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank->chordAt (1), dsp::MusicalNote::D, dsp::ChordType::Minor);
  expect_descriptor_equals (
    *bank->chordAt (2), dsp::MusicalNote::E, dsp::ChordType::Diminished);
}

TEST_F (ChordPadCommandsTest, MovePadBackwardRedoUndo)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank->addChord (dsp::MusicalNote::D, dsp::ChordType::Minor);
  bank->addChord (dsp::MusicalNote::E, dsp::ChordType::Diminished);

  MoveChordPadCommand cmd (*bank, 2, 0);
  cmd.redo ();
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::E, dsp::ChordType::Diminished);
  expect_descriptor_equals (
    *bank->chordAt (2), dsp::MusicalNote::D, dsp::ChordType::Minor);

  cmd.undo ();
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank->chordAt (2), dsp::MusicalNote::E, dsp::ChordType::Diminished);
}

// --- ReplaceAllChordPadsCommand ---

TEST_F (ChordPadCommandsTest, ReplaceAllRedoUndo)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  bank->addChord (dsp::MusicalNote::D, dsp::ChordType::Minor);

  std::vector<ChordPadState> after = {
    { dsp::MusicalNote::A, dsp::ChordType::Minor, dsp::ChordAccent::Seventh, 0,
     dsp::MusicalNote::E },
    { dsp::MusicalNote::G, dsp::ChordType::Major },
    { dsp::MusicalNote::F, dsp::ChordType::Diminished },
  };

  ReplaceAllChordPadsCommand cmd (*bank, std::move (after));
  cmd.redo ();
  ASSERT_EQ (bank->rowCount (), 3);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::A, dsp::ChordType::Minor,
    dsp::ChordAccent::Seventh, 0, true);
  expect_descriptor_equals (
    *bank->chordAt (1), dsp::MusicalNote::G, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank->chordAt (2), dsp::MusicalNote::F, dsp::ChordType::Diminished);

  cmd.undo ();
  ASSERT_EQ (bank->rowCount (), 2);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
  expect_descriptor_equals (
    *bank->chordAt (1), dsp::MusicalNote::D, dsp::ChordType::Minor);
}

TEST_F (ChordPadCommandsTest, ReplaceAllFromEmpty)
{
  std::vector<ChordPadState> after = {
    { dsp::MusicalNote::C, dsp::ChordType::Major },
  };

  ReplaceAllChordPadsCommand cmd (*bank, std::move (after));
  cmd.redo ();
  ASSERT_EQ (bank->rowCount (), 1);

  cmd.undo ();
  EXPECT_EQ (bank->rowCount (), 0);
}

TEST_F (ChordPadCommandsTest, ReplaceAllToEmpty)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);

  std::vector<ChordPadState> after;
  ReplaceAllChordPadsCommand cmd (*bank, std::move (after));
  cmd.redo ();
  EXPECT_EQ (bank->rowCount (), 0);

  cmd.undo ();
  ASSERT_EQ (bank->rowCount (), 1);
  expect_descriptor_equals (
    *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
}

TEST_F (ChordPadCommandsTest, ReplaceAllMultipleCycles)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);

  std::vector<ChordPadState> after = {
    { dsp::MusicalNote::A, dsp::ChordType::Minor },
  };

  ReplaceAllChordPadsCommand cmd (*bank, std::move (after));
  for (int i = 0; i < 3; ++i)
    {
      cmd.redo ();
      ASSERT_EQ (bank->rowCount (), 1);
      expect_descriptor_equals (
        *bank->chordAt (0), dsp::MusicalNote::A, dsp::ChordType::Minor);
      cmd.undo ();
      ASSERT_EQ (bank->rowCount (), 1);
      expect_descriptor_equals (
        *bank->chordAt (0), dsp::MusicalNote::C, dsp::ChordType::Major);
    }
}

TEST_F (ChordPadCommandsTest, CommandText)
{
  bank->addChord (dsp::MusicalNote::C, dsp::ChordType::Major);
  EditChordPadCommand edit_cmd (
    *bank, 0, ChordPadState{ dsp::MusicalNote::D, dsp::ChordType::Minor });
  EXPECT_EQ (edit_cmd.text (), QString ("Edit Chord Pad"));

  AddChordPadCommand add_cmd (
    *bank, ChordPadState{ dsp::MusicalNote::C, dsp::ChordType::Major });
  EXPECT_EQ (add_cmd.text (), QString ("Add Chord Pad"));

  RemoveChordPadCommand rm_cmd (*bank, 0);
  EXPECT_EQ (rm_cmd.text (), QString ("Remove Chord Pad"));

  MoveChordPadCommand mv_cmd (*bank, 0, 1);
  EXPECT_EQ (mv_cmd.text (), QString ("Move Chord Pad"));

  ReplaceAllChordPadsCommand repl_cmd (*bank, {});
  EXPECT_EQ (repl_cmd.text (), QString ("Replace Chord Pads"));
}

} // namespace zrythm::commands
