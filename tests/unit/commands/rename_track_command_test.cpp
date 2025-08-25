// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/rename_track_command.h"

#include "unit/structure/tracks/mock_track.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{
class RenameTrackCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track = mock_track_factory.createMockTrack (
      structure::tracks::Track::Type::Audio);
    track->setName ("Original Track Name");
  }

  structure::tracks::MockTrackFactory           mock_track_factory;
  std::unique_ptr<structure::tracks::MockTrack> track;
};

// Test initial state after construction
TEST_F (RenameTrackCommandTest, InitialState)
{
  RenameTrackCommand command (*track, "New Track Name");

  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test redo operation
TEST_F (RenameTrackCommandTest, RedoOperation)
{
  RenameTrackCommand command (*track, "New Track Name");

  command.redo ();

  EXPECT_EQ (track->name (), QString ("New Track Name"));
}

// Test undo operation
TEST_F (RenameTrackCommandTest, UndoOperation)
{
  RenameTrackCommand command (*track, "New Track Name");

  command.redo ();
  command.undo ();

  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test empty name handling
TEST_F (RenameTrackCommandTest, EmptyName)
{
  RenameTrackCommand command (*track, "");

  command.redo ();
  EXPECT_EQ (track->name (), QString (""));

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test same name (no-op)
TEST_F (RenameTrackCommandTest, SameName)
{
  RenameTrackCommand command (*track, "Original Track Name");

  command.redo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test special characters in name
TEST_F (RenameTrackCommandTest, SpecialCharacters)
{
  const QString special_name =
    "Track with special chars: @#$%^&*()_+-=[]{}|;':\",./<>?";
  RenameTrackCommand command (*track, special_name);

  command.redo ();
  EXPECT_EQ (track->name (), special_name);

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test Unicode characters
TEST_F (RenameTrackCommandTest, UnicodeCharacters)
{
  const QString      unicode_name = "トラック名 🎵 音乐 🎹";
  RenameTrackCommand command (*track, unicode_name);

  command.redo ();
  EXPECT_EQ (track->name (), unicode_name);

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test very long name
TEST_F (RenameTrackCommandTest, VeryLongName)
{
  const QString      long_name = QString (1000, 'A'); // 1000 'A' characters
  RenameTrackCommand command (*track, long_name);

  command.redo ();
  EXPECT_EQ (track->name (), long_name);

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test multiple undo/redo cycles
TEST_F (RenameTrackCommandTest, MultipleUndoRedoCycles)
{
  RenameTrackCommand command (*track, "First New Name");

  // First cycle
  command.redo ();
  EXPECT_EQ (track->name (), QString ("First New Name"));

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));

  // Second cycle
  command.redo ();
  EXPECT_EQ (track->name (), QString ("First New Name"));

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test command text
TEST_F (RenameTrackCommandTest, CommandText)
{
  RenameTrackCommand command (*track, "Test Name");

  // The command should have the text "Rename Track" for display in undo stack
  EXPECT_EQ (command.text (), QString ("Rename Track"));
}

// Test with whitespace-only name
TEST_F (RenameTrackCommandTest, WhitespaceOnlyName)
{
  const QString      whitespace_name = "   \t\n  ";
  RenameTrackCommand command (*track, whitespace_name);

  command.redo ();
  EXPECT_EQ (track->name (), whitespace_name);

  command.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test consecutive renames
TEST_F (RenameTrackCommandTest, ConsecutiveRenames)
{
  RenameTrackCommand command1 (*track, "First Rename");
  RenameTrackCommand command2 (*track, "Second Rename");

  // First rename
  command1.redo ();
  EXPECT_EQ (track->name (), QString ("First Rename"));

  // Second rename
  command2.redo ();
  EXPECT_EQ (track->name (), QString ("Second Rename"));

  // Undo second rename
  command2.undo ();
  EXPECT_EQ (track->name (), QString ("First Rename"));

  // Undo first rename
  command1.undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

} // namespace zrythm::commands
