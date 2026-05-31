// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "structure/project/midi_editor.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::project
{

class MidiEditorTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a QCoreApplication for signal testing
    app = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    // Create MidiEditor object
    midi_editor = std::make_unique<MidiEditor> ();

    // Initialize the piano roll to set up descriptors
    midi_editor->init ();
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app;
  std::unique_ptr<MidiEditor>                           midi_editor;
};

// Test initial state
TEST_F (MidiEditorTest, InitialState)
{
  // MidiEditor centers Y on construction
  EXPECT_DOUBLE_EQ (midi_editor->getY (), 16.0 * 64.0);

  EXPECT_EQ (midi_editor->keyHeight (), 16);
}

// Test keyHeight property and signal
TEST_F (MidiEditorTest, KeyHeightProperty)
{
  // Note: keyHeight is currently read-only in QML interface
  // We can only test the getter and signal emission if it changes internally

  QSignalSpy keyHeightSpy (midi_editor.get (), &MidiEditor::keyHeightChanged);

  // Initially no signal should be emitted
  EXPECT_EQ (keyHeightSpy.count (), 0);

  // Test getter returns expected value
  EXPECT_EQ (midi_editor->keyHeight (), 16);
}

// Test getKeyAtY QML invokable method
TEST_F (MidiEditorTest, GetKeyAtY)
{
  // Test key calculation based on Y position
  // Formula: key = 127 - (y / note_height_)
  const int note_height = midi_editor->keyHeight ();

  // Test middle position
  int middle_y = note_height * 64; // Should give key around 63
  int key = midi_editor->getKeyAtY (middle_y);
  EXPECT_EQ (key, 127 - 64); // 63

  // Test top position (y = 0)
  key = midi_editor->getKeyAtY (0);
  EXPECT_EQ (key, 127);

  // Test bottom position
  int bottom_y = note_height * 127;
  key = midi_editor->getKeyAtY (bottom_y);
  EXPECT_EQ (key, 0);

  // Test fractional Y positions (should be truncated)
  key = midi_editor->getKeyAtY (note_height * 32.5);
  EXPECT_EQ (key, 127 - 32); // 95, fractional part truncated

  // Test negative Y position
  key = midi_editor->getKeyAtY (-10);
  EXPECT_EQ (key, 127); // Should clamp to maximum key (127)

  // Test very large Y position
  key = midi_editor->getKeyAtY (note_height * 200);
  EXPECT_EQ (key, 0); // Should clamp to minimum key (0)
}

// Test isBlackKey QML invokable method
TEST_F (MidiEditorTest, IsBlackKey)
{
  // Test known black keys (C# = 1, D# = 3, F# = 6, G# = 8, A# = 10)
  EXPECT_TRUE (MidiEditor::isBlackKey (1));  // C#
  EXPECT_TRUE (MidiEditor::isBlackKey (3));  // D#
  EXPECT_TRUE (MidiEditor::isBlackKey (6));  // F#
  EXPECT_TRUE (MidiEditor::isBlackKey (8));  // G#
  EXPECT_TRUE (MidiEditor::isBlackKey (10)); // A#

  // Test known white keys (C = 0, D = 2, E = 4, F = 5, G = 7, A = 9, B = 11)
  EXPECT_FALSE (MidiEditor::isBlackKey (0));  // C
  EXPECT_FALSE (MidiEditor::isBlackKey (2));  // D
  EXPECT_FALSE (MidiEditor::isBlackKey (4));  // E
  EXPECT_FALSE (MidiEditor::isBlackKey (5));  // F
  EXPECT_FALSE (MidiEditor::isBlackKey (7));  // G
  EXPECT_FALSE (MidiEditor::isBlackKey (9));  // A
  EXPECT_FALSE (MidiEditor::isBlackKey (11)); // B

  // Test with octave wrapping (mod 12)
  EXPECT_TRUE (MidiEditor::isBlackKey (13));  // C# in next octave (1 + 12)
  EXPECT_TRUE (MidiEditor::isBlackKey (15));  // D# in next octave (3 + 12)
  EXPECT_FALSE (MidiEditor::isBlackKey (12)); // C in next octave (0 + 12)
  EXPECT_FALSE (MidiEditor::isBlackKey (14)); // D in next octave (2 + 12)

  // Test edge cases
  EXPECT_FALSE (MidiEditor::isBlackKey (-11)); // Should wrap to 0 (C)
  EXPECT_FALSE (MidiEditor::isBlackKey (-12)); // Should wrap to 0 (C)
}

// Test isWhiteKey QML invokable method
TEST_F (MidiEditorTest, IsWhiteKey)
{
  // isWhiteKey should be the opposite of isBlackKey
  for (int i = 0; i < 24; ++i) // Test two octaves
    {
      EXPECT_EQ (MidiEditor::isWhiteKey (i), !MidiEditor::isBlackKey (i))
        << "Failed at note " << i;
    }
}

// Test isNextKeyBlack QML invokable method
TEST_F (MidiEditorTest, IsNextKeyBlack)
{
  // Test next key predictions
  EXPECT_TRUE (MidiEditor::isNextKeyBlack (0));  // Next to C is C# (black)
  EXPECT_FALSE (MidiEditor::isNextKeyBlack (1)); // Next to C# is D (white)
  EXPECT_TRUE (MidiEditor::isNextKeyBlack (2));  // Next to D is D# (black)
  EXPECT_FALSE (MidiEditor::isNextKeyBlack (3)); // Next to D# is E (white)
  EXPECT_FALSE (MidiEditor::isNextKeyBlack (4)); // Next to E is F (white)
  EXPECT_TRUE (MidiEditor::isNextKeyBlack (5));  // Next to F is F# (black)

  // Test octave wrapping
  EXPECT_FALSE (MidiEditor::isNextKeyBlack (11)); // Next to B is C (white)
  EXPECT_FALSE (MidiEditor::isNextKeyBlack (-1)); // Next to B-1 is C (white)
}

// Test isNextKeyWhite QML invokable method
TEST_F (MidiEditorTest, IsNextKeyWhite)
{
  // isNextKeyWhite should be the opposite of isNextKeyBlack
  for (int i = 0; i < 24; ++i) // Test two octaves
    {
      EXPECT_EQ (MidiEditor::isNextKeyWhite (i), !MidiEditor::isNextKeyBlack (i))
        << "Failed at note " << i;
    }
}

// Test isPrevKeyBlack QML invokable method
TEST_F (MidiEditorTest, IsPrevKeyBlack)
{
  // Test previous key predictions
  EXPECT_FALSE (
    MidiEditor::isPrevKeyBlack (0)); // Prev to C is B (white) - but we are
                                     // clamping so we are at C (white)
  EXPECT_FALSE (MidiEditor::isPrevKeyBlack (1)); // Prev to C# is C (white)
  EXPECT_TRUE (MidiEditor::isPrevKeyBlack (2));  // Prev to D is C# (black)
  EXPECT_FALSE (MidiEditor::isPrevKeyBlack (3)); // Prev to D# is D (white)

  // Test octave wrapping
  EXPECT_FALSE (
    MidiEditor::isPrevKeyBlack (12)); // Prev to C in next octave is B (white)
}

// Test isPrevKeyWhite QML invokable method
TEST_F (MidiEditorTest, IsPrevKeyWhite)
{
  // isPrevKeyWhite should be the opposite of isPrevKeyBlack
  for (int i = 0; i < 24; ++i) // Test two octaves
    {
      EXPECT_EQ (MidiEditor::isPrevKeyWhite (i), !MidiEditor::isPrevKeyBlack (i))
        << "Failed at note " << i;
    }
}

// Test object parenting
TEST_F (MidiEditorTest, ObjectParenting)
{
  // Create MidiEditor with a parent
  QObject parent;
  auto    child_midi_editor = std::make_unique<MidiEditor> (&parent);

  // Verify parent relationship
  EXPECT_EQ (child_midi_editor->parent (), &parent);

  // Test that QML methods still work
  EXPECT_TRUE (MidiEditor::isBlackKey (1));
  EXPECT_FALSE (MidiEditor::isWhiteKey (1));
  EXPECT_EQ (child_midi_editor->getKeyAtY (0), 127);
}

// Test edge cases for QML methods
TEST_F (MidiEditorTest, QmlMethodEdgeCases)
{
  // Test getKeyAtY with extreme values
  // EXPECT_EQ (midi_editor->getKeyAtY (std::numeric_limits<double>::max ()),
  // 0); EXPECT_EQ (
  //   midi_editor->getKeyAtY (std::numeric_limits<double>::lowest ()), 127);
  // EXPECT_EQ (
  // midi_editor->getKeyAtY (std::numeric_limits<double>::quiet_NaN ()), 127);

  // Test key methods with extreme values
  EXPECT_FALSE (MidiEditor::isBlackKey (std::numeric_limits<int>::max ()));
  EXPECT_FALSE (MidiEditor::isBlackKey (std::numeric_limits<int>::min ()));
  // EXPECT_TRUE (MidiEditor::isNextKeyBlack (std::numeric_limits<int>::max
  // ())); EXPECT_TRUE (MidiEditor::isPrevKeyBlack
  // (std::numeric_limits<int>::min ()));
}

// Test QML method performance (basic sanity check)
TEST_F (MidiEditorTest, QmlMethodPerformance)
{
  // Test that static methods can be called many times without issues
  for (int i = 0; i < 1000; ++i)
    {
      int  note = i % 128;
      bool is_black = MidiEditor::isBlackKey (note);
      bool is_white = MidiEditor::isWhiteKey (note);
      bool next_black = MidiEditor::isNextKeyBlack (note);
      bool next_white = MidiEditor::isNextKeyWhite (note);
      bool prev_black = MidiEditor::isPrevKeyBlack (note);
      bool prev_white = MidiEditor::isPrevKeyWhite (note);

      // Basic sanity checks
      EXPECT_EQ (is_black, !is_white);
      EXPECT_EQ (next_black, !next_white);
      EXPECT_EQ (prev_black, !prev_white);
    }
}

} // namespace zrythm::structure::project
