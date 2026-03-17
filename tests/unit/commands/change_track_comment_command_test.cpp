// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/change_track_comment_command.h"

#include "unit/structure/tracks/mock_track.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{

class ChangeTrackCommentCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track = mock_track_factory.createMockTrack (
      structure::tracks::Track::Type::Audio);
    track->setComment ("Initial comment");
  }

  structure::tracks::MockTrackFactory           mock_track_factory;
  std::unique_ptr<structure::tracks::MockTrack> track;
};

// Test initial state after construction
TEST_F (ChangeTrackCommentCommandTest, InitialState)
{
  ChangeTrackCommentCommand command (*track, "New comment");

  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test redo operation
TEST_F (ChangeTrackCommentCommandTest, RedoOperation)
{
  ChangeTrackCommentCommand command (*track, "New comment");

  command.redo ();

  EXPECT_EQ (track->comment (), QString ("New comment"));
}

// Test undo operation
TEST_F (ChangeTrackCommentCommandTest, UndoOperation)
{
  ChangeTrackCommentCommand command (*track, "New comment");

  command.redo ();
  command.undo ();

  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test same comment (no-op)
TEST_F (ChangeTrackCommentCommandTest, SameComment)
{
  ChangeTrackCommentCommand command (*track, "Initial comment");

  command.redo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test multiple undo/redo cycles
TEST_F (ChangeTrackCommentCommandTest, MultipleUndoRedoCycles)
{
  ChangeTrackCommentCommand command (*track, "Another comment");

  // First cycle
  command.redo ();
  EXPECT_EQ (track->comment (), QString ("Another comment"));

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));

  // Second cycle
  command.redo ();
  EXPECT_EQ (track->comment (), QString ("Another comment"));

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test command text
TEST_F (ChangeTrackCommentCommandTest, CommandText)
{
  ChangeTrackCommentCommand command (*track, "Test comment");

  EXPECT_EQ (command.text (), QString ("Change Track Comment"));
}

// Test with empty comment
TEST_F (ChangeTrackCommentCommandTest, EmptyComment)
{
  ChangeTrackCommentCommand command (*track, "");

  command.redo ();
  EXPECT_EQ (track->comment (), QString (""));

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test with multiline comment
TEST_F (ChangeTrackCommentCommandTest, MultilineComment)
{
  QString                   multiline = "Line 1\nLine 2\nLine 3";
  ChangeTrackCommentCommand command (*track, multiline);

  command.redo ();
  EXPECT_EQ (track->comment (), multiline);

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test with special characters
TEST_F (ChangeTrackCommentCommandTest, SpecialCharacters)
{
  QString                   special = "Special: <>&\"'\\n\\t";
  ChangeTrackCommentCommand command (*track, special);

  command.redo ();
  EXPECT_EQ (track->comment (), special);

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test with unicode characters
TEST_F (ChangeTrackCommentCommandTest, UnicodeCharacters)
{
  QString                   unicode = "Unicode: 日本語 עברית emoji 🎵";
  ChangeTrackCommentCommand command (*track, unicode);

  command.redo ();
  EXPECT_EQ (track->comment (), unicode);

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test consecutive comment changes
TEST_F (ChangeTrackCommentCommandTest, ConsecutiveCommentChanges)
{
  ChangeTrackCommentCommand command1 (*track, "First comment");
  ChangeTrackCommentCommand command2 (*track, "Second comment");

  // First comment change
  command1.redo ();
  EXPECT_EQ (track->comment (), QString ("First comment"));

  // Second comment change
  command2.redo ();
  EXPECT_EQ (track->comment (), QString ("Second comment"));

  // Undo second comment change
  command2.undo ();
  EXPECT_EQ (track->comment (), QString ("First comment"));

  // Undo first comment change
  command1.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test with long comment
TEST_F (ChangeTrackCommentCommandTest, LongComment)
{
  QString long_comment;
  for (int i = 0; i < 1000; ++i)
    {
      long_comment += "This is a long comment. ";
    }

  ChangeTrackCommentCommand command (*track, long_comment);

  command.redo ();
  EXPECT_EQ (track->comment (), long_comment);

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Initial comment"));
}

// Test clearing comment to empty
TEST_F (ChangeTrackCommentCommandTest, ClearToEmpty)
{
  track->setComment ("Non-empty comment");

  ChangeTrackCommentCommand command (*track, "");

  command.redo ();
  EXPECT_TRUE (track->comment ().isEmpty ());

  command.undo ();
  EXPECT_EQ (track->comment (), QString ("Non-empty comment"));
}

} // namespace zrythm::commands
