// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_playback_cache.h"

#include <gtest/gtest.h>

namespace zrythm::dsp
{

class MidiPlaybackCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    cache = std::make_unique<MidiPlaybackCache> ();

    // Create a simple MIDI sequence with note on/off events
    sequence.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
    sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 1.0);
    sequence.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 2.0);
    sequence.addEvent (juce::MidiMessage::noteOff (1, 64), 3.0);
    sequence.updateMatchedPairs ();
  }

  std::unique_ptr<MidiPlaybackCache> cache;
  juce::MidiMessageSequence          sequence;
};

TEST_F (MidiPlaybackCacheTest, InitialState)
{
  EXPECT_TRUE (cache->cached_events ().getNumEvents () == 0);
}

TEST_F (MidiPlaybackCacheTest, ClearMethod)
{
  // Add a sequence first
  cache->add_sequence ({ 0, 100 }, sequence);
  cache->finalize_changes ();

  // Verify sequence was added
  EXPECT_FALSE (cache->cached_events ().getNumEvents () == 0);

  // Clear the cache
  cache->clear ();

  // Verify cache is empty
  EXPECT_TRUE (cache->cached_events ().getNumEvents () == 0);
}

TEST_F (MidiPlaybackCacheTest, AddSequenceAndFinalize)
{
  // Add sequence and finalize
  cache->add_sequence ({ 0, 100 }, sequence);
  cache->finalize_changes ();

  // Should have 4 events (2 note-ons + 2 note-offs)
  EXPECT_EQ (cache->cached_events ().getNumEvents (), 4);
}

TEST_F (MidiPlaybackCacheTest, RemoveSequencesMatchingInterval_NoOverlap)
{
  // Create sequences that match their intervals
  juce::MidiMessageSequence seq1;
  seq1.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq1.addEvent (juce::MidiMessage::noteOff (1, 60), 25.0);

  juce::MidiMessageSequence seq2;
  seq2.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 160.0);
  seq2.addEvent (juce::MidiMessage::noteOff (1, 64), 175.0);

  // Add sequences that don't overlap with removal interval
  cache->add_sequence ({ 0, 50 }, seq1);    // Before removal interval
  cache->add_sequence ({ 150, 200 }, seq2); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->cached_events ().getNumEvents (), 4);

  // Remove sequences overlapping with [60, 140] - should remove nothing
  cache->remove_sequences_matching_interval ({ 60, 140 });
  cache->finalize_changes ();

  // Both sequences should remain
  EXPECT_EQ (cache->cached_events ().getNumEvents (), 4);
}

TEST_F (MidiPlaybackCacheTest, RemoveSequencesMatchingInterval_PartialOverlap)
{
  // Create sequences that match their intervals
  juce::MidiMessageSequence seq1;
  seq1.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq1.addEvent (juce::MidiMessage::noteOff (1, 60), 50.0);

  juce::MidiMessageSequence seq2;
  seq2.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 90.0);
  seq2.addEvent (juce::MidiMessage::noteOff (1, 64), 170.0);

  juce::MidiMessageSequence seq3;
  seq3.addEvent (juce::MidiMessage::noteOn (1, 67, 1.0f), 160.0);
  seq3.addEvent (juce::MidiMessage::noteOff (1, 67), 240.0);

  juce::MidiMessageSequence seq4;
  seq4.addEvent (juce::MidiMessage::noteOn (1, 69, 1.0f), 210.0);
  seq4.addEvent (juce::MidiMessage::noteOff (1, 69), 290.0);

  // Add sequences with various overlaps
  cache->add_sequence ({ 0, 100 }, seq1);  // Overlaps start of removal interval
  cache->add_sequence ({ 80, 180 }, seq2); // Overlaps middle of removal interval
  cache->add_sequence ({ 150, 250 }, seq3); // Overlaps end of removal interval
  cache->add_sequence ({ 200, 300 }, seq4); // No overlap
  cache->finalize_changes ();

  EXPECT_EQ (cache->cached_events ().getNumEvents (), 8);

  // Remove sequences overlapping with [90, 160]
  cache->remove_sequences_matching_interval ({ 90, 160 });
  cache->finalize_changes ();

  // Only the non-overlapping sequence should remain
  EXPECT_EQ (cache->cached_events ().getNumEvents (), 2);
}

TEST_F (MidiPlaybackCacheTest, RemoveSequencesMatchingInterval_FullyContained)
{
  // Create sequences that match their intervals
  juce::MidiMessageSequence seq1;
  seq1.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq1.addEvent (juce::MidiMessage::noteOff (1, 60), 25.0);

  juce::MidiMessageSequence seq2;
  seq2.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 70.0);
  seq2.addEvent (juce::MidiMessage::noteOff (1, 64), 85.0);

  juce::MidiMessageSequence seq3;
  seq3.addEvent (juce::MidiMessage::noteOn (1, 67, 1.0f), 110.0);
  seq3.addEvent (juce::MidiMessage::noteOff (1, 67), 140.0);

  // Add sequences where one is fully contained within removal interval
  cache->add_sequence ({ 0, 50 }, seq1);    // Before removal
  cache->add_sequence ({ 60, 90 }, seq2);   // Fully within removal
  cache->add_sequence ({ 100, 150 }, seq3); // After removal
  cache->finalize_changes ();

  EXPECT_EQ (cache->cached_events ().getNumEvents (), 6);

  // Remove sequences overlapping with [60, 99] - avoid boundary conflicts
  cache->remove_sequences_matching_interval ({ 60, 99 });
  cache->finalize_changes ();

  // Only sequences before and after should remain (sequence at {60,90} is
  // removed)
  EXPECT_EQ (cache->cached_events ().getNumEvents (), 4);
}

TEST_F (MidiPlaybackCacheTest, EventsOutsideIntervalValidation)
{
  // Create a sequence with events outside the specified interval
  juce::MidiMessageSequence out_of_bounds_sequence;
  out_of_bounds_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0); // Within interval [0, 100]
  out_of_bounds_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 64, 1.0f), 150.0); // Outside interval [0, 100]

  // Should throw exception when adding sequence with events outside interval
  EXPECT_THROW (
    cache->add_sequence ({ 0, 100 }, out_of_bounds_sequence),
    std::invalid_argument);
}

TEST_F (MidiPlaybackCacheTest, UnendedNotesValidation)
{
  // Create a sequence with unended notes
  juce::MidiMessageSequence unended_sequence;
  unended_sequence.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  unended_sequence.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 10.0);
  // Intentionally missing note-off events

  // Add sequence with interval [0, 100]
  cache->add_sequence ({ 0, 100 }, unended_sequence);
  cache->finalize_changes ();

  const auto &merged = cache->cached_events ();

  // Should have note-off events added automatically at interval end time (100)
  EXPECT_EQ (merged.getNumEvents (), 4); // 2 note-ons + 2 note-offs

  // Verify note-off events were added at the correct time
  int note_off_count = 0;
  for (int i = 0; i < merged.getNumEvents (); ++i)
    {
      const auto * event = merged.getEventPointer (i);
      if (event->message.isNoteOff ())
        {
          note_off_count++;
          EXPECT_EQ (event->message.getTimeStamp (), 100.0);
        }
    }
  EXPECT_EQ (note_off_count, 2);
}

TEST_F (MidiPlaybackCacheTest, MultipleAddRemoveOperations)
{
  // Create sequences that match their intervals
  juce::MidiMessageSequence seq1;
  seq1.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq1.addEvent (juce::MidiMessage::noteOff (1, 60), 50.0);

  juce::MidiMessageSequence seq2;
  seq2.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 70.0);
  seq2.addEvent (juce::MidiMessage::noteOff (1, 64), 140.0);

  juce::MidiMessageSequence seq3;
  seq3.addEvent (juce::MidiMessage::noteOn (1, 67, 1.0f), 110.0);
  seq3.addEvent (juce::MidiMessage::noteOff (1, 67), 190.0);

  // Add multiple overlapping sequences
  cache->add_sequence ({ 0, 100 }, seq1);
  cache->add_sequence ({ 50, 150 }, seq2);
  cache->add_sequence ({ 100, 200 }, seq3);
  cache->finalize_changes ();

  EXPECT_EQ (cache->cached_events ().getNumEvents (), 6);

  // Remove middle sequence by targeting its interval
  cache->remove_sequences_matching_interval ({ 50, 149 });
  cache->finalize_changes ();

  // All sequences should be removed (all touch boundaries)
  EXPECT_EQ (cache->cached_events ().getNumEvents (), 0);
}

TEST_F (MidiPlaybackCacheTest, EmptyCacheOperations)
{
  // Operations on empty cache should not crash
  cache->clear ();
  cache->remove_sequences_matching_interval ({ 0, 100 });
  cache->finalize_changes ();

  EXPECT_TRUE (cache->cached_events ().getNumEvents () == 0);
}

TEST_F (MidiPlaybackCacheTest, ConstCorrectness)
{
  cache->add_sequence ({ 0, 100 }, sequence);
  cache->finalize_changes ();

  // cached_events() should return const reference
  const auto &events = cache->cached_events ();
  // Should have 4 events: 2 note-ons + 2 note-offs
  EXPECT_EQ (events.getNumEvents (), 4);

  // Verify we can't modify the returned reference (compile-time check)
  static_assert (
    std::is_same_v<
      decltype (cache->cached_events ()), const juce::MidiMessageSequence &>);
}

} // namespace zrythm::dsp
