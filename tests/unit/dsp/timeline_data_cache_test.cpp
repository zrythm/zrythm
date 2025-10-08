// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/timeline_data_cache.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::dsp
{

class TimelineDataCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    cache = std::make_unique<TimelineDataCache> ();

    // Create a simple MIDI sequence with note on/off events
    sequence.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
    sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 1.0);
    sequence.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 2.0);
    sequence.addEvent (juce::MidiMessage::noteOff (1, 64), 3.0);
    sequence.updateMatchedPairs ();

    // Create a simple audio buffer for testing
    audio_buffer.setSize (2, 256); // stereo, 256 samples
    audio_buffer.clear ();

    // Fill with a simple cosine wave pattern
    for (int channel = 0; channel < 2; ++channel)
      {
        auto * channel_data = audio_buffer.getWritePointer (channel);
        for (int sample = 0; sample < 256; ++sample)
          {
            channel_data[sample] =
              0.5f
              * std::cos (
                2.0f * std::numbers::pi_v<float>
                * static_cast<float> (sample) / 256.0f);
          }
      }
  }

  std::unique_ptr<TimelineDataCache> cache;
  juce::MidiMessageSequence          sequence;
  juce::AudioSampleBuffer            audio_buffer;
};

TEST_F (TimelineDataCacheTest, InitialState)
{
  EXPECT_TRUE (cache->get_midi_events ().getNumEvents () == 0);
  EXPECT_FALSE (cache->has_audio_regions ());
}

TEST_F (TimelineDataCacheTest, ClearMethod)
{
  // Add a MIDI sequence first
  cache->add_midi_sequence ({ 0, 100 }, sequence);
  cache->add_audio_region ({ 0, 256 }, audio_buffer);
  cache->finalize_changes ();

  // Verify sequences were added
  EXPECT_FALSE (cache->get_midi_events ().getNumEvents () == 0);
  EXPECT_TRUE (cache->has_audio_regions ());

  // Clear the cache
  cache->clear ();

  // Verify cache is empty
  EXPECT_TRUE (cache->get_midi_events ().getNumEvents () == 0);
  EXPECT_FALSE (cache->has_audio_regions ());
}

TEST_F (TimelineDataCacheTest, AddSequenceAndFinalize)
{
  // Add MIDI sequence and finalize
  cache->add_midi_sequence ({ 0, 100 }, sequence);
  cache->finalize_changes ();

  // Should have 4 events (2 note-ons + 2 note-offs)
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
}

TEST_F (TimelineDataCacheTest, AddAudioRegionAndFinalize)
{
  // Add audio region and finalize
  cache->add_audio_region ({ 0, 256 }, audio_buffer);
  cache->finalize_changes ();

  // Should have one audio region
  EXPECT_TRUE (cache->has_audio_regions ());
  EXPECT_EQ (cache->get_audio_regions ().size (), 1);

  // Verify the audio region properties
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions[0].start_sample, 0);
  EXPECT_EQ (regions[0].end_sample, 256);
  EXPECT_EQ (regions[0].audio_buffer.getNumChannels (), 2);
  EXPECT_EQ (regions[0].audio_buffer.getNumSamples (), 256);
}

TEST_F (TimelineDataCacheTest, RemoveSequencesMatchingInterval_NoOverlap)
{
  // Create sequences that match their intervals
  juce::MidiMessageSequence seq1;
  seq1.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq1.addEvent (juce::MidiMessage::noteOff (1, 60), 25.0);

  juce::MidiMessageSequence seq2;
  seq2.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 160.0);
  seq2.addEvent (juce::MidiMessage::noteOff (1, 64), 175.0);

  // Add sequences that don't overlap with removal interval
  cache->add_midi_sequence ({ 0, 50 }, seq1);    // Before removal interval
  cache->add_midi_sequence ({ 150, 200 }, seq2); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);

  // Remove sequences overlapping with [60, 140] - should remove nothing
  cache->remove_sequences_matching_interval ({ 60, 140 });
  cache->finalize_changes ();

  // Both sequences should remain
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
}

TEST_F (TimelineDataCacheTest, RemoveSequencesMatchingInterval_PartialOverlap)
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
  cache->add_midi_sequence (
    { 0, 100 }, seq1); // Overlaps start of removal interval
  cache->add_midi_sequence (
    { 80, 180 }, seq2); // Overlaps middle of removal interval
  cache->add_midi_sequence (
    { 150, 250 }, seq3); // Overlaps end of removal interval
  cache->add_midi_sequence ({ 200, 300 }, seq4); // No overlap
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 8);

  // Remove sequences overlapping with [90, 160]
  cache->remove_sequences_matching_interval ({ 90, 160 });
  cache->finalize_changes ();

  // Only the non-overlapping sequence should remain
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 2);
}

TEST_F (TimelineDataCacheTest, RemoveSequencesMatchingInterval_FullyContained)
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
  cache->add_midi_sequence ({ 0, 50 }, seq1);    // Before removal
  cache->add_midi_sequence ({ 60, 90 }, seq2);   // Fully within removal
  cache->add_midi_sequence ({ 100, 150 }, seq3); // After removal
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 6);

  // Remove sequences overlapping with [60, 99] - avoid boundary conflicts
  cache->remove_sequences_matching_interval ({ 60, 99 });
  cache->finalize_changes ();

  // Only sequences before and after should remain (sequence at {60,90} is
  // removed)
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
}

TEST_F (TimelineDataCacheTest, EventsOutsideIntervalValidation)
{
  // Create a sequence with events outside the specified interval
  juce::MidiMessageSequence out_of_bounds_sequence;
  out_of_bounds_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0); // Within interval [0, 100]
  out_of_bounds_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 64, 1.0f), 150.0); // Outside interval [0, 100]

  // Should throw exception when adding sequence with events outside interval
  EXPECT_THROW (
    cache->add_midi_sequence ({ 0, 100 }, out_of_bounds_sequence),
    std::invalid_argument);
}

TEST_F (TimelineDataCacheTest, UnendedNotesValidation)
{
  // Create a sequence with unended notes
  juce::MidiMessageSequence unended_sequence;
  unended_sequence.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  unended_sequence.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 10.0);
  // Intentionally missing note-off events

  // Add sequence with interval [0, 100]
  cache->add_midi_sequence ({ 0, 100 }, unended_sequence);
  cache->finalize_changes ();

  const auto &merged = cache->get_midi_events ();

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

TEST_F (TimelineDataCacheTest, MultipleAddRemoveOperations)
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
  cache->add_midi_sequence ({ 0, 100 }, seq1);
  cache->add_midi_sequence ({ 50, 150 }, seq2);
  cache->add_midi_sequence ({ 100, 200 }, seq3);
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 6);

  // Remove middle sequence by targeting its interval
  cache->remove_sequences_matching_interval ({ 50, 149 });
  cache->finalize_changes ();

  // All sequences should be removed (all touch boundaries)
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 0);
}

TEST_F (TimelineDataCacheTest, EmptyCacheOperations)
{
  // Operations on empty cache should not crash
  cache->clear ();
  cache->remove_sequences_matching_interval ({ 0, 100 });
  cache->finalize_changes ();

  EXPECT_TRUE (cache->get_midi_events ().getNumEvents () == 0);
}

TEST_F (TimelineDataCacheTest, ConstCorrectness)
{
  cache->add_midi_sequence ({ 0, 100 }, sequence);
  cache->finalize_changes ();

  // cached_events() should return const reference
  const auto &events = cache->get_midi_events ();
  // Should have 4 events: 2 note-ons + 2 note-offs
  EXPECT_EQ (events.getNumEvents (), 4);

  // Verify we can't modify the returned reference (compile-time check)
  static_assert (
    std::is_same_v<
      decltype (cache->get_midi_events ()), const juce::MidiMessageSequence &>);
}

// ========== Audio-Specific Tests ==========

TEST_F (TimelineDataCacheTest, AddMultipleAudioRegions)
{
  // Create multiple audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 256);

  buffer1.clear ();
  buffer2.clear ();

  // Fill with different patterns
  buffer1.getWritePointer (0)[0] = 1.0f; // Mark buffer1
  buffer2.getWritePointer (0)[0] = 2.0f; // Mark buffer2

  // Add regions at different positions
  cache->add_audio_region ({ 0, 128 }, buffer1);
  cache->add_audio_region ({ 256, 512 }, buffer2);
  cache->finalize_changes ();

  // Should have two regions sorted by start position
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions.size (), 2);
  EXPECT_EQ (regions[0].start_sample, 0);
  EXPECT_EQ (regions[1].start_sample, 256);

  // Verify the buffers are independent copies
  EXPECT_EQ (regions[0].audio_buffer.getReadPointer (0)[0], 1.0f);
  EXPECT_EQ (regions[1].audio_buffer.getReadPointer (0)[0], 2.0f);
}

TEST_F (TimelineDataCacheTest, RemoveAudioRegionsMatchingInterval)
{
  // Create audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 128);
  juce::AudioSampleBuffer buffer3 (2, 128);

  buffer1.clear ();
  buffer2.clear ();
  buffer3.clear ();

  // Add regions at different positions
  cache->add_audio_region ({ 0, 128 }, buffer1);   // Before removal interval
  cache->add_audio_region ({ 200, 328 }, buffer2); // Inside removal interval
  cache->add_audio_region ({ 400, 528 }, buffer3); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_audio_regions ().size (), 3);

  // Remove regions overlapping with [150, 350]
  cache->remove_sequences_matching_interval ({ 150, 350 });
  cache->finalize_changes ();

  // Should have 2 regions remaining (before and after)
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions.size (), 2);
  EXPECT_EQ (regions[0].start_sample, 0);
  EXPECT_EQ (regions[1].start_sample, 400);
}

TEST_F (TimelineDataCacheTest, MixedMidiAndAudioOperations)
{
  // Add both MIDI and audio data at non-overlapping intervals
  cache->add_midi_sequence ({ 0, 100 }, sequence);
  cache->add_audio_region ({ 200, 456 }, audio_buffer);
  cache->finalize_changes ();

  // Verify both types are present
  EXPECT_TRUE (cache->has_midi_events ());
  EXPECT_TRUE (cache->has_audio_regions ());
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
  EXPECT_EQ (cache->get_audio_regions ().size (), 1);

  // Remove only the audio region (interval doesn't overlap MIDI)
  cache->remove_sequences_matching_interval ({ 200, 456 });
  cache->finalize_changes ();

  // MIDI should remain, audio should be gone
  EXPECT_TRUE (cache->has_midi_events ());
  EXPECT_FALSE (cache->has_audio_regions ());

  // Remove only the MIDI sequence (interval doesn't overlap remaining audio)
  cache->remove_sequences_matching_interval ({ 0, 100 });
  cache->finalize_changes ();

  // Both should be gone
  EXPECT_FALSE (cache->has_midi_events ());
  EXPECT_FALSE (cache->has_audio_regions ());
}

TEST_F (TimelineDataCacheTest, MixedMidiAndAudioOperations_OverlappingRemoval)
{
  // Add both MIDI and audio data
  cache->add_midi_sequence ({ 0, 100 }, sequence);
  cache->add_audio_region ({ 50, 306 }, audio_buffer);
  cache->finalize_changes ();

  // Verify both types are present
  EXPECT_TRUE (cache->has_midi_events ());
  EXPECT_TRUE (cache->has_audio_regions ());
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
  EXPECT_EQ (cache->get_audio_regions ().size (), 1);

  // Remove both MIDI and audio with overlapping interval
  cache->remove_sequences_matching_interval ({ 25, 200 });
  cache->finalize_changes ();

  // Both should be gone because the interval overlaps both
  EXPECT_FALSE (cache->has_midi_events ());
  EXPECT_FALSE (cache->has_audio_regions ());
}

TEST_F (TimelineDataCacheTest, AudioBufferIndependence)
{
  // Add an audio buffer
  cache->add_audio_region ({ 0, 256 }, audio_buffer);
  cache->finalize_changes ();

  // Modify the original buffer
  audio_buffer.setSample (0, 0, 0.2f);

  // Verify the cached buffer is unaffected (independent copy)
  const auto &regions = cache->get_audio_regions ();
  EXPECT_FLOAT_EQ (regions[0].audio_buffer.getReadPointer (0)[0], 0.5f);

  // Modify the cached buffer
  const_cast<juce::AudioSampleBuffer &> (regions[0].audio_buffer).clear ();

  // Verify the original buffer is still unaffected
  EXPECT_FLOAT_EQ (audio_buffer.getReadPointer (0)[0], 0.2f);
}

TEST_F (TimelineDataCacheTest, AudioRegionSorting)
{
  // Create audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 128);
  juce::AudioSampleBuffer buffer3 (2, 128);

  buffer1.clear ();
  buffer2.clear ();
  buffer3.clear ();

  // Add regions in non-sequential order
  cache->add_audio_region ({ 300, 428 }, buffer1); // Last
  cache->add_audio_region ({ 100, 228 }, buffer2); // First
  cache->add_audio_region ({ 200, 328 }, buffer3); // Middle
  cache->finalize_changes ();

  // Verify regions are sorted by start position
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions.size (), 3);
  EXPECT_EQ (regions[0].start_sample, 100);
  EXPECT_EQ (regions[1].start_sample, 200);
  EXPECT_EQ (regions[2].start_sample, 300);
}

TEST_F (TimelineDataCacheTest, EmptyAudioOperations)
{
  // Operations on empty audio cache should not crash
  EXPECT_FALSE (cache->has_audio_regions ());
  EXPECT_EQ (cache->get_audio_regions ().size (), 0);

  cache->remove_sequences_matching_interval ({ 0, 100 });
  cache->finalize_changes ();

  EXPECT_FALSE (cache->has_audio_regions ());
}

} // namespace zrythm::dsp
