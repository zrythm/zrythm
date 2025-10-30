// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "structure/arrangement/piano_roll.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{

class PianoRollTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    // Create a QCoreApplication for signal testing
    app = std::make_unique<test_helpers::ScopedQCoreApplication> ();

    // Create PianoRoll object
    piano_roll = std::make_unique<PianoRoll> ();

    // Initialize the piano roll to set up descriptors
    piano_roll->init ();
  }

  std::unique_ptr<test_helpers::ScopedQCoreApplication> app;
  std::unique_ptr<PianoRoll>                            piano_roll;
};

// Test initial state and QML properties
TEST_F (PianoRollTest, InitialState)
{
  // Test editor settings property
  EXPECT_NE (piano_roll->getEditorSettings (), nullptr);
  EXPECT_DOUBLE_EQ (piano_roll->getEditorSettings ()->getX (), 0.0);
  EXPECT_DOUBLE_EQ (
    piano_roll->getEditorSettings ()->getY (), 16.0 * 64.0); // Centered
  EXPECT_DOUBLE_EQ (
    piano_roll->getEditorSettings ()->getHorizontalZoomLevel (), 1.0);

  // Test key height property
  EXPECT_EQ (piano_roll->getKeyHeight (), 16);
}

// Test keyHeight property and signal
TEST_F (PianoRollTest, KeyHeightProperty)
{
  // Note: keyHeight is currently read-only in QML interface
  // We can only test the getter and signal emission if it changes internally

  QSignalSpy keyHeightSpy (piano_roll.get (), &PianoRoll::keyHeightChanged);

  // Initially no signal should be emitted
  EXPECT_EQ (keyHeightSpy.count (), 0);

  // Test getter returns expected value
  EXPECT_EQ (piano_roll->getKeyHeight (), 16);
}

// Test getKeyAtY QML invokable method
TEST_F (PianoRollTest, GetKeyAtY)
{
  // Test key calculation based on Y position
  // Formula: key = 127 - (y / note_height_)
  const int note_height = piano_roll->getKeyHeight ();

  // Test middle position
  int middle_y = note_height * 64; // Should give key around 63
  int key = piano_roll->getKeyAtY (middle_y);
  EXPECT_EQ (key, 127 - 64); // 63

  // Test top position (y = 0)
  key = piano_roll->getKeyAtY (0);
  EXPECT_EQ (key, 127);

  // Test bottom position
  int bottom_y = note_height * 127;
  key = piano_roll->getKeyAtY (bottom_y);
  EXPECT_EQ (key, 0);

  // Test fractional Y positions (should be truncated)
  key = piano_roll->getKeyAtY (note_height * 32.5);
  EXPECT_EQ (key, 127 - 32); // 95, fractional part truncated

  // Test negative Y position
  key = piano_roll->getKeyAtY (-10);
  EXPECT_EQ (key, 127); // Should clamp to maximum key (127)

  // Test very large Y position
  key = piano_roll->getKeyAtY (note_height * 200);
  EXPECT_EQ (key, 0); // Should clamp to minimum key (0)
}

// Test isBlackKey QML invokable method
TEST_F (PianoRollTest, IsBlackKey)
{
  // Test known black keys (C# = 1, D# = 3, F# = 6, G# = 8, A# = 10)
  EXPECT_TRUE (PianoRoll::isBlackKey (1));  // C#
  EXPECT_TRUE (PianoRoll::isBlackKey (3));  // D#
  EXPECT_TRUE (PianoRoll::isBlackKey (6));  // F#
  EXPECT_TRUE (PianoRoll::isBlackKey (8));  // G#
  EXPECT_TRUE (PianoRoll::isBlackKey (10)); // A#

  // Test known white keys (C = 0, D = 2, E = 4, F = 5, G = 7, A = 9, B = 11)
  EXPECT_FALSE (PianoRoll::isBlackKey (0));  // C
  EXPECT_FALSE (PianoRoll::isBlackKey (2));  // D
  EXPECT_FALSE (PianoRoll::isBlackKey (4));  // E
  EXPECT_FALSE (PianoRoll::isBlackKey (5));  // F
  EXPECT_FALSE (PianoRoll::isBlackKey (7));  // G
  EXPECT_FALSE (PianoRoll::isBlackKey (9));  // A
  EXPECT_FALSE (PianoRoll::isBlackKey (11)); // B

  // Test with octave wrapping (mod 12)
  EXPECT_TRUE (PianoRoll::isBlackKey (13));  // C# in next octave (1 + 12)
  EXPECT_TRUE (PianoRoll::isBlackKey (15));  // D# in next octave (3 + 12)
  EXPECT_FALSE (PianoRoll::isBlackKey (12)); // C in next octave (0 + 12)
  EXPECT_FALSE (PianoRoll::isBlackKey (14)); // D in next octave (2 + 12)

  // Test edge cases
  EXPECT_FALSE (PianoRoll::isBlackKey (-11)); // Should wrap to 0 (C)
  EXPECT_FALSE (PianoRoll::isBlackKey (-12)); // Should wrap to 0 (C)
}

// Test isWhiteKey QML invokable method
TEST_F (PianoRollTest, IsWhiteKey)
{
  // isWhiteKey should be the opposite of isBlackKey
  for (int i = 0; i < 24; ++i) // Test two octaves
    {
      EXPECT_EQ (PianoRoll::isWhiteKey (i), !PianoRoll::isBlackKey (i))
        << "Failed at note " << i;
    }
}

// Test isNextKeyBlack QML invokable method
TEST_F (PianoRollTest, IsNextKeyBlack)
{
  // Test next key predictions
  EXPECT_TRUE (PianoRoll::isNextKeyBlack (0));  // Next to C is C# (black)
  EXPECT_FALSE (PianoRoll::isNextKeyBlack (1)); // Next to C# is D (white)
  EXPECT_TRUE (PianoRoll::isNextKeyBlack (2));  // Next to D is D# (black)
  EXPECT_FALSE (PianoRoll::isNextKeyBlack (3)); // Next to D# is E (white)
  EXPECT_FALSE (PianoRoll::isNextKeyBlack (4)); // Next to E is F (white)
  EXPECT_TRUE (PianoRoll::isNextKeyBlack (5));  // Next to F is F# (black)

  // Test octave wrapping
  EXPECT_FALSE (PianoRoll::isNextKeyBlack (11)); // Next to B is C (white)
  EXPECT_FALSE (PianoRoll::isNextKeyBlack (-1)); // Next to B-1 is C (white)
}

// Test isNextKeyWhite QML invokable method
TEST_F (PianoRollTest, IsNextKeyWhite)
{
  // isNextKeyWhite should be the opposite of isNextKeyBlack
  for (int i = 0; i < 24; ++i) // Test two octaves
    {
      EXPECT_EQ (PianoRoll::isNextKeyWhite (i), !PianoRoll::isNextKeyBlack (i))
        << "Failed at note " << i;
    }
}

// Test isPrevKeyBlack QML invokable method
TEST_F (PianoRollTest, IsPrevKeyBlack)
{
  // Test previous key predictions
  EXPECT_FALSE (
    PianoRoll::isPrevKeyBlack (0)); // Prev to C is B (white) - but we are
                                    // clamping so we are at C (white)
  EXPECT_FALSE (PianoRoll::isPrevKeyBlack (1)); // Prev to C# is C (white)
  EXPECT_TRUE (PianoRoll::isPrevKeyBlack (2));  // Prev to D is C# (black)
  EXPECT_FALSE (PianoRoll::isPrevKeyBlack (3)); // Prev to D# is D (white)

  // Test octave wrapping
  EXPECT_FALSE (
    PianoRoll::isPrevKeyBlack (12)); // Prev to C in next octave is B (white)
}

// Test isPrevKeyWhite QML invokable method
TEST_F (PianoRollTest, IsPrevKeyWhite)
{
  // isPrevKeyWhite should be the opposite of isPrevKeyBlack
  for (int i = 0; i < 24; ++i) // Test two octaves
    {
      EXPECT_EQ (PianoRoll::isPrevKeyWhite (i), !PianoRoll::isPrevKeyBlack (i))
        << "Failed at note " << i;
    }
}

// Test editor settings property interaction
TEST_F (PianoRollTest, EditorSettingsInteraction)
{
  auto * editor_settings = piano_roll->getEditorSettings ();
  ASSERT_NE (editor_settings, nullptr);

  // Test that we can modify editor settings through the property
  QSignalSpy xChangedSpy (editor_settings, &EditorSettings::xChanged);
  QSignalSpy yChangedSpy (editor_settings, &EditorSettings::yChanged);
  QSignalSpy hzoomChangedSpy (
    editor_settings, &EditorSettings::horizontalZoomLevelChanged);

  // Modify editor settings
  editor_settings->setX (100.0);
  editor_settings->setY (200.0);
  editor_settings->setHorizontalZoomLevel (2.5);

  // Verify changes
  EXPECT_DOUBLE_EQ (editor_settings->getX (), 100.0);
  EXPECT_DOUBLE_EQ (editor_settings->getY (), 200.0);
  EXPECT_DOUBLE_EQ (editor_settings->getHorizontalZoomLevel (), 2.5);

  // Verify signals were emitted
  EXPECT_EQ (xChangedSpy.count (), 1);
  EXPECT_EQ (yChangedSpy.count (), 1);
  EXPECT_EQ (hzoomChangedSpy.count (), 1);
}

// Test QML property constants
TEST_F (PianoRollTest, QmlPropertyConstants)
{
  // Test that editorSettings is CONSTANT (should always return same object)
  auto * editor_settings1 = piano_roll->getEditorSettings ();
  auto * editor_settings2 = piano_roll->getEditorSettings ();
  EXPECT_EQ (editor_settings1, editor_settings2);
}

// Test object parenting
TEST_F (PianoRollTest, ObjectParenting)
{
  // Create PianoRoll with a parent
  QObject parent;
  auto    child_piano_roll = std::make_unique<PianoRoll> (&parent);

  // Verify parent relationship
  EXPECT_EQ (child_piano_roll->parent (), &parent);

  // Test that editor settings are still accessible
  EXPECT_NE (child_piano_roll->getEditorSettings (), nullptr);

  // Test that QML methods still work
  EXPECT_TRUE (PianoRoll::isBlackKey (1));
  EXPECT_FALSE (PianoRoll::isWhiteKey (1));
  EXPECT_EQ (child_piano_roll->getKeyAtY (0), 127);
}

// Test edge cases for QML methods
TEST_F (PianoRollTest, QmlMethodEdgeCases)
{
  // Test getKeyAtY with extreme values
  // EXPECT_EQ (piano_roll->getKeyAtY (std::numeric_limits<double>::max ()), 0);
  // EXPECT_EQ (
  //   piano_roll->getKeyAtY (std::numeric_limits<double>::lowest ()), 127);
  // EXPECT_EQ (
  // piano_roll->getKeyAtY (std::numeric_limits<double>::quiet_NaN ()), 127);

  // Test key methods with extreme values
  EXPECT_FALSE (PianoRoll::isBlackKey (std::numeric_limits<int>::max ()));
  EXPECT_FALSE (PianoRoll::isBlackKey (std::numeric_limits<int>::min ()));
  // EXPECT_TRUE (PianoRoll::isNextKeyBlack (std::numeric_limits<int>::max ()));
  // EXPECT_TRUE (PianoRoll::isPrevKeyBlack (std::numeric_limits<int>::min ()));
}

// Test QML method performance (basic sanity check)
TEST_F (PianoRollTest, QmlMethodPerformance)
{
  // Test that static methods can be called many times without issues
  for (int i = 0; i < 1000; ++i)
    {
      int  note = i % 128;
      bool is_black = PianoRoll::isBlackKey (note);
      bool is_white = PianoRoll::isWhiteKey (note);
      bool next_black = PianoRoll::isNextKeyBlack (note);
      bool next_white = PianoRoll::isNextKeyWhite (note);
      bool prev_black = PianoRoll::isPrevKeyBlack (note);
      bool prev_white = PianoRoll::isPrevKeyWhite (note);

      // Basic sanity checks
      EXPECT_EQ (is_black, !is_white);
      EXPECT_EQ (next_black, !next_white);
      EXPECT_EQ (prev_black, !prev_white);
    }
}

} // namespace zrythm::structure::arrangement
