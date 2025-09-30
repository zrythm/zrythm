// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "actions/track_operator.h"
#include "undo/undo_stack.h"

#include "unit/actions/mock_undo_stack.h"
#include "unit/structure/tracks/mock_track.h"
#include <gtest/gtest.h>

namespace zrythm::actions
{
class TrackOperatorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    track = mock_track_factory.createMockTrack (
      structure::tracks::Track::Type::Audio);
    track->setName ("Original Track Name");
    track->setColor (QColor ("red"));
    undo_stack = create_mock_undo_stack ();
    track_operator = std::make_unique<TrackOperator> ();
    track_operator->setTrack (track.get ());
    track_operator->setUndoStack (undo_stack.get ());
  }

  structure::tracks::MockTrackFactory           mock_track_factory;
  std::unique_ptr<structure::tracks::MockTrack> track;
  std::unique_ptr<undo::UndoStack>              undo_stack;
  std::unique_ptr<TrackOperator>                track_operator;
};

// Test initial state
TEST_F (TrackOperatorTest, InitialState)
{
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
  EXPECT_EQ (undo_stack->count (), 0);
  EXPECT_EQ (undo_stack->index (), 0);
}

// Test basic rename operation
TEST_F (TrackOperatorTest, BasicRename)
{
  track_operator->rename ("New Track Name");

  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test undo after rename
TEST_F (TrackOperatorTest, UndoAfterRename)
{
  track_operator->rename ("New Track Name");
  undo_stack->undo ();

  EXPECT_EQ (track->name (), QString ("Original Track Name"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 0);
  EXPECT_FALSE (undo_stack->canUndo ());
  EXPECT_TRUE (undo_stack->canRedo ());
}

// Test redo after undo
TEST_F (TrackOperatorTest, RedoAfterUndo)
{
  track_operator->rename ("New Track Name");
  undo_stack->undo ();
  undo_stack->redo ();

  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test multiple consecutive renames
TEST_F (TrackOperatorTest, MultipleConsecutiveRenames)
{
  track_operator->rename ("First New Name");
  EXPECT_EQ (track->name (), QString ("First New Name"));
  EXPECT_EQ (undo_stack->count (), 1);

  track_operator->rename ("Second New Name");
  EXPECT_EQ (track->name (), QString ("Second New Name"));
  EXPECT_EQ (undo_stack->count (), 2);

  track_operator->rename ("Third New Name");
  EXPECT_EQ (track->name (), QString ("Third New Name"));
  EXPECT_EQ (undo_stack->count (), 3);

  // Undo all changes
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Second New Name"));

  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("First New Name"));

  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
}

// Test undo/redo cycles
TEST_F (TrackOperatorTest, UndoRedoCycles)
{
  track_operator->rename ("New Name");

  // First cycle
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));

  undo_stack->redo ();
  EXPECT_EQ (track->name (), QString ("New Name"));

  // Second cycle
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));

  undo_stack->redo ();
  EXPECT_EQ (track->name (), QString ("New Name"));
}

#if 0
// Test track operator with null track (should throw exception)
TEST_F (TrackOperatorTest, NullTrackThrowsException)
{
  EXPECT_THROW (track_operator->setTrack (nullptr), std::invalid_argument);
}

// Test track operator with null undo stack (should throw exception)
TEST_F (TrackOperatorTest, NullUndoStackThrowsException)
{
  EXPECT_THROW (track_operator->setUndoStack (nullptr), std::invalid_argument);
}
#endif

// Test command text in undo stack
TEST_F (TrackOperatorTest, CommandTextInUndoStack)
{
  track_operator->rename ("Test Name");

  EXPECT_EQ (undo_stack->text (0), QString ("Rename Track"));
}

// Test basic setColor operation
TEST_F (TrackOperatorTest, BasicSetColor)
{
  track->setColor (QColor ("#FF0000"));          // Red
  track_operator->setColor (QColor ("#00FF00")); // Green

  EXPECT_EQ (track->color (), QColor ("#00FF00"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test undo after setColor
TEST_F (TrackOperatorTest, UndoAfterSetColor)
{
  track->setColor (QColor ("#FF0000"));
  track_operator->setColor (QColor ("#00FF00"));
  undo_stack->undo ();

  EXPECT_EQ (track->color (), QColor ("#FF0000"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 0);
  EXPECT_FALSE (undo_stack->canUndo ());
  EXPECT_TRUE (undo_stack->canRedo ());
}

// Test redo after undo for setColor
TEST_F (TrackOperatorTest, RedoAfterUndoSetColor)
{
  track->setColor (QColor ("#FF0000"));
  track_operator->setColor (QColor ("#00FF00"));
  undo_stack->undo ();
  undo_stack->redo ();

  EXPECT_EQ (track->color (), QColor ("#00FF00"));
  EXPECT_EQ (undo_stack->count (), 1);
  EXPECT_EQ (undo_stack->index (), 1);
  EXPECT_TRUE (undo_stack->canUndo ());
  EXPECT_FALSE (undo_stack->canRedo ());
}

// Test command text in undo stack for setColor
TEST_F (TrackOperatorTest, CommandTextInUndoStackSetColor)
{
  track_operator->setColor (QColor ("#123456"));

  EXPECT_EQ (undo_stack->text (0), QString ("Change Track Color"));
}

// Test mixed operations (rename and setColor)
TEST_F (TrackOperatorTest, MixedOperations)
{
  track_operator->rename ("New Track Name");
  track_operator->setColor (QColor ("#00FF00"));

  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (track->color (), QColor ("#00FF00"));
  EXPECT_EQ (undo_stack->count (), 2);

  // Undo color change
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("New Track Name"));
  EXPECT_EQ (track->color (), QColor ("red"));

  // Undo rename
  undo_stack->undo ();
  EXPECT_EQ (track->name (), QString ("Original Track Name"));
  EXPECT_EQ (track->color (), QColor ("red"));
}

} // namespace zrythm::actions
