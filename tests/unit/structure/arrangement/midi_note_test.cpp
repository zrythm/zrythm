// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <memory>

#include "dsp/atomic_position_qml_adapter.h"
#include "dsp/tempo_map.h"
#include "structure/arrangement/midi_note.h"

#include "helpers/mock_qobject.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace zrythm::structure::arrangement
{
class MidiNoteTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    tempo_map = std::make_unique<dsp::TempoMap> (44100.0);
    parent = std::make_unique<MockQObject> ();
    note = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  }

  std::unique_ptr<dsp::TempoMap> tempo_map;
  std::unique_ptr<MockQObject>   parent;
  std::unique_ptr<MidiNote>      note;
};

// Test initial state
TEST_F (MidiNoteTest, InitialState)
{
  EXPECT_EQ (note->type (), ArrangerObject::Type::MidiNote);
  EXPECT_EQ (note->position ()->samples (), 0);
  EXPECT_EQ (note->pitch (), 60);    // Default pitch is C4
  EXPECT_EQ (note->velocity (), 90); // Default velocity
  EXPECT_FALSE (note->getSelected ());
  EXPECT_NE (note->bounds (), nullptr);
  EXPECT_NE (note->mute (), nullptr);
}

// Test pitch property
TEST_F (MidiNoteTest, PitchProperty)
{
  // Set pitch
  note->setPitch (72); // C5
  EXPECT_EQ (note->pitch (), 72);

  // Test clamping
  note->setPitch (200);
  EXPECT_EQ (note->pitch (), 127); // Max value

  note->setPitch (-10);
  EXPECT_EQ (note->pitch (), 0); // Min value

  // Test pitchChanged signal
  testing::MockFunction<void (int)> mockPitchChanged;
  QObject::connect (
    note.get (), &MidiNote::pitchChanged, parent.get (),
    mockPitchChanged.AsStdFunction ());

  EXPECT_CALL (mockPitchChanged, Call (64)).Times (1);
  note->setPitch (64);

  // No signal when same value
  EXPECT_CALL (mockPitchChanged, Call (64)).Times (0);
  note->setPitch (64);
}

// Test shift_pitch method
TEST_F (MidiNoteTest, ShiftPitch)
{
  // Initial pitch is 60
  note->shift_pitch (5);
  EXPECT_EQ (note->pitch (), 65);

  note->shift_pitch (-10);
  EXPECT_EQ (note->pitch (), 55);

  // Test clamping
  note->setPitch (120);
  note->shift_pitch (10);
  EXPECT_EQ (note->pitch (), 127);

  note->setPitch (5);
  note->shift_pitch (-10);
  EXPECT_EQ (note->pitch (), 0);
}

// Test velocity property
TEST_F (MidiNoteTest, VelocityProperty)
{
  // Set velocity
  note->setVelocity (100);
  EXPECT_EQ (note->velocity (), 100);

  // Test clamping
  note->setVelocity (200);
  EXPECT_EQ (note->velocity (), 127); // Max value

  note->setVelocity (-10);
  EXPECT_EQ (note->velocity (), 0); // Min value

  // Test velocityChanged signal
  testing::MockFunction<void (int)> mockVelocityChanged;
  QObject::connect (
    note.get (), &MidiNote::velocityChanged, parent.get (),
    mockVelocityChanged.AsStdFunction ());

  EXPECT_CALL (mockVelocityChanged, Call (80)).Times (1);
  note->setVelocity (80);

  // No signal when same value
  EXPECT_CALL (mockVelocityChanged, Call (80)).Times (0);
  note->setVelocity (80);
}

// Test pitchAsRichText method
TEST_F (MidiNoteTest, PitchAsRichText)
{
  // Test C4 (default)
  EXPECT_EQ (note->pitchAsRichText ().toStdString (), "C<sup>4</sup>");

  // Test other notes
  note->setPitch (61); // C#4
  EXPECT_EQ (note->pitchAsRichText ().toStdString (), "D♭<sup>4</sup>");

  note->setPitch (72); // C5
  EXPECT_EQ (note->pitchAsRichText ().toStdString (), "C<sup>5</sup>");

  // Test boundary cases
  note->setPitch (0); // C-1
  EXPECT_EQ (note->pitchAsRichText ().toStdString (), "C<sup>-1</sup>");

  note->setPitch (127); // G9
  EXPECT_EQ (note->pitchAsRichText ().toStdString (), "G<sup>9</sup>");
}

// Test pitch_to_string static method
TEST_F (MidiNoteTest, PitchToString)
{
  // Musical notation
  auto str = MidiNote::pitch_to_string (60, MidiNote::Notation::Musical, true);
  EXPECT_EQ (str.str (), "C<sup>4</sup>");

  str = MidiNote::pitch_to_string (60, MidiNote::Notation::Musical, false);
  EXPECT_EQ (str.str (), "C4");

  // Pitch notation
  EXPECT_THROW (
    MidiNote::pitch_to_string (60, MidiNote::Notation::Pitch, true),
    std::invalid_argument);

  str = MidiNote::pitch_to_string (60, MidiNote::Notation::Pitch, false);
  EXPECT_EQ (str.str (), "60");
}

// Test bounds functionality
TEST_F (MidiNoteTest, BoundsFunctionality)
{
  // Set length
  note->bounds ()->length ()->setSamples (500);
  EXPECT_EQ (note->bounds ()->length ()->samples (), 500);

  // Set position
  note->position ()->setSamples (1000);
  EXPECT_EQ (note->position ()->samples (), 1000);
}

// Test mute functionality
TEST_F (MidiNoteTest, MuteFunctionality)
{
  // Mute the note
  note->mute ()->setMuted (true);
  EXPECT_TRUE (note->mute ()->muted ());

  // Unmute the note
  note->mute ()->setMuted (false);
  EXPECT_FALSE (note->mute ()->muted ());
}

// Test friend functions for MidiNote collections
TEST_F (MidiNoteTest, FriendFunctions)
{
  // Create multiple notes with different properties
  auto note1 = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  note1->position ()->setSamples (1000);
  note1->bounds ()->length ()->setSamples (500); // End at 1500
  note1->setPitch (60);                          // C4

  auto note2 = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  note2->position ()->setSamples (2000);
  note2->bounds ()->length ()->setSamples (300); // End at 2300
  note2->setPitch (72);                          // C5

  auto note3 = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  note3->position ()->setSamples (500);
  note3->bounds ()->length ()->setSamples (1000); // End at 1500
  note3->setPitch (48);                           // C3

  // Create a vector of notes
  std::vector<MidiNote *> notes = { note1.get (), note2.get (), note3.get () };

  // Test get_first_midi_note
  MidiNote * first = get_first_midi_note (notes);
  EXPECT_EQ (first, note3.get ());
  EXPECT_EQ (first->position ()->samples (), 500);

  // Test get_last_midi_note
  MidiNote * last = get_last_midi_note (notes);
  EXPECT_EQ (last, note2.get ());
  EXPECT_EQ (last->bounds ()->get_end_position_samples (false), 2299);

  // Test get_pitch_range
  auto pitch_range = get_pitch_range (notes);
  ASSERT_TRUE (pitch_range.has_value ());
  EXPECT_EQ (pitch_range->first, 48);  // Min pitch (C3)
  EXPECT_EQ (pitch_range->second, 72); // Max pitch (C5)

  // Test with empty range
  std::vector<MidiNote *> empty;
  EXPECT_EQ (get_first_midi_note (empty), nullptr);
  EXPECT_EQ (get_last_midi_note (empty), nullptr);
  auto empty_range = get_pitch_range (empty);
  EXPECT_FALSE (empty_range.has_value ());

  // Test with single note
  std::vector<MidiNote *> single = { note1.get () };
  EXPECT_EQ (get_first_midi_note (single), note1.get ());
  EXPECT_EQ (get_last_midi_note (single), note1.get ());
  auto single_range = get_pitch_range (single);
  ASSERT_TRUE (single_range.has_value ());
  EXPECT_EQ (single_range->first, 60);
  EXPECT_EQ (single_range->second, 60);
}

// Test edge cases for friend functions
TEST_F (MidiNoteTest, FriendFunctionsEdgeCases)
{
  // Create notes with same positions
  auto note1 = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  note1->position ()->setSamples (1000);
  note1->bounds ()->length ()->setSamples (500);
  note1->setPitch (60);

  auto note2 = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  note2->position ()->setSamples (1000);
  note2->bounds ()->length ()->setSamples (500);
  note2->setPitch (72);

  std::vector<MidiNote *> notes = { note1.get (), note2.get () };

  // When positions are equal, first should be the first in the container
  EXPECT_EQ (get_first_midi_note (notes), note1.get ());

  // When end positions are equal, last should be the last in the container
  EXPECT_EQ (get_last_midi_note (notes), note2.get ());

  // Test pitch range with different pitches
  auto pitch_range = get_pitch_range (notes);
  ASSERT_TRUE (pitch_range.has_value ());
  EXPECT_EQ (pitch_range->first, 60);
  EXPECT_EQ (pitch_range->second, 72);

  // When pitches are equal, range should be [pitch, pitch]
  note2->setPitch (60);
  auto equal_pitch_range = get_pitch_range (notes);
  ASSERT_TRUE (equal_pitch_range.has_value ());
  EXPECT_EQ (equal_pitch_range->first, 60);
  EXPECT_EQ (equal_pitch_range->second, 60);

  // Test with notes at minimum/maximum pitches
  auto minNote = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  minNote->setPitch (0);

  auto maxNote = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  maxNote->setPitch (127);

  std::vector<MidiNote *> extremeNotes = { minNote.get (), maxNote.get () };
  auto                    extreme_range = get_pitch_range (extremeNotes);
  ASSERT_TRUE (extreme_range.has_value ());
  EXPECT_EQ (extreme_range->first, 0);
  EXPECT_EQ (extreme_range->second, 127);
}

// Test serialization/deserialization
TEST_F (MidiNoteTest, Serialization)
{
  // Set initial state
  note->position ()->setSamples (2000);
  note->setPitch (72);
  note->setVelocity (110);
  note->bounds ()->length ()->setSamples (1000);

  // Serialize
  nlohmann::json j;
  to_json (j, *note);

  // Create new note
  auto new_note = std::make_unique<MidiNote> (*tempo_map, parent.get ());
  from_json (j, *new_note);

  // Verify state
  EXPECT_EQ (new_note->position ()->samples (), 2000);
  EXPECT_EQ (new_note->pitch (), 72);
  EXPECT_EQ (new_note->velocity (), 110);
  EXPECT_EQ (new_note->bounds ()->length ()->samples (), 1000);
}

// Test copying
TEST_F (MidiNoteTest, Copying)
{
  // Set initial state
  note->position ()->setSamples (3000);
  note->setPitch (65);
  note->setVelocity (95);
  note->bounds ()->length ()->setSamples (1500);

  // Create target
  auto target = std::make_unique<MidiNote> (*tempo_map, parent.get ());

  // Copy
  init_from (*target, *note, utils::ObjectCloneType::Snapshot);

  // Verify state
  EXPECT_EQ (target->position ()->samples (), 3000);
  EXPECT_EQ (target->pitch (), 65);
  EXPECT_EQ (target->velocity (), 95);
  EXPECT_EQ (target->bounds ()->length ()->samples (), 1500);
}

} // namespace zrythm::structure::arrangement
