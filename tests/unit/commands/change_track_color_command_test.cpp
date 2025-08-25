// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "commands/change_track_color_command.h"

#include "unit/structure/tracks/mock_track.h"
#include <gtest/gtest.h>

namespace zrythm::commands
{
class ChangeTrackColorCommandTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track = mock_track_factory.createMockTrack (
      structure::tracks::Track::Type::Audio);
    track->setColor (QColor ("#FF0000")); // Red
  }

  structure::tracks::MockTrackFactory           mock_track_factory;
  std::unique_ptr<structure::tracks::MockTrack> track;
};

// Test initial state after construction
TEST_F (ChangeTrackColorCommandTest, InitialState)
{
  ChangeTrackColorCommand command (*track, QColor ("#00FF00"));

  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test redo operation
TEST_F (ChangeTrackColorCommandTest, RedoOperation)
{
  ChangeTrackColorCommand command (*track, QColor ("#00FF00"));

  command.redo ();

  EXPECT_EQ (track->color (), QColor ("#00FF00"));
}

// Test undo operation
TEST_F (ChangeTrackColorCommandTest, UndoOperation)
{
  ChangeTrackColorCommand command (*track, QColor ("#00FF00"));

  command.redo ();
  command.undo ();

  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test same color (no-op)
TEST_F (ChangeTrackColorCommandTest, SameColor)
{
  ChangeTrackColorCommand command (*track, QColor ("#FF0000"));

  command.redo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test multiple undo/redo cycles
TEST_F (ChangeTrackColorCommandTest, MultipleUndoRedoCycles)
{
  ChangeTrackColorCommand command (*track, QColor ("#0000FF"));

  // First cycle
  command.redo ();
  EXPECT_EQ (track->color (), QColor ("#0000FF"));

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));

  // Second cycle
  command.redo ();
  EXPECT_EQ (track->color (), QColor ("#0000FF"));

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test command text
TEST_F (ChangeTrackColorCommandTest, CommandText)
{
  ChangeTrackColorCommand command (*track, QColor ("#FFFFFF"));

  // The command should have the text "Change Track Color" for display in undo
  // stack
  EXPECT_EQ (command.text (), QString ("Change Track Color"));
}

// Test with black color
TEST_F (ChangeTrackColorCommandTest, BlackColor)
{
  ChangeTrackColorCommand command (*track, QColor ("#000000"));

  command.redo ();
  EXPECT_EQ (track->color (), QColor ("#000000"));

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test with white color
TEST_F (ChangeTrackColorCommandTest, WhiteColor)
{
  ChangeTrackColorCommand command (*track, QColor ("#FFFFFF"));

  command.redo ();
  EXPECT_EQ (track->color (), QColor ("#FFFFFF"));

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test with transparent color
TEST_F (ChangeTrackColorCommandTest, TransparentColor)
{
  QColor                  transparent_red (255, 0, 0, 128);
  ChangeTrackColorCommand command (*track, transparent_red);

  command.redo ();
  EXPECT_EQ (track->color (), transparent_red);

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test consecutive color changes
TEST_F (ChangeTrackColorCommandTest, ConsecutiveColorChanges)
{
  ChangeTrackColorCommand command1 (*track, QColor ("#00FF00"));
  ChangeTrackColorCommand command2 (*track, QColor ("#0000FF"));

  // First color change
  command1.redo ();
  EXPECT_EQ (track->color (), QColor ("#00FF00"));

  // Second color change
  command2.redo ();
  EXPECT_EQ (track->color (), QColor ("#0000FF"));

  // Undo second color change
  command2.undo ();
  EXPECT_EQ (track->color (), QColor ("#00FF00"));

  // Undo first color change
  command1.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test with custom RGB values
TEST_F (ChangeTrackColorCommandTest, CustomRgbValues)
{
  QColor                  custom_color (123, 45, 67);
  ChangeTrackColorCommand command (*track, custom_color);

  command.redo ();
  EXPECT_EQ (track->color (), custom_color);

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

// Test with named colors
TEST_F (ChangeTrackColorCommandTest, NamedColors)
{
  ChangeTrackColorCommand command (*track, QColor ("magenta"));

  command.redo ();
  EXPECT_EQ (track->color (), QColor ("magenta"));

  command.undo ();
  EXPECT_EQ (track->color (), QColor ("#FF0000"));
}

} // namespace zrythm::commands
