// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/timeline_data_cache.h"

#include <gtest/gtest.h>
#include <juce_audio_formats/juce_audio_formats.h>

namespace zrythm::dsp
{

class MidiTimelineDataCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    cache = std::make_unique<MidiTimelineDataCache> ();

    // Create a simple MIDI sequence with note on/off events
    sequence.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
    sequence.addEvent (juce::MidiMessage::noteOff (1, 60), 1.0);
    sequence.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 2.0);
    sequence.addEvent (juce::MidiMessage::noteOff (1, 64), 3.0);
    sequence.updateMatchedPairs ();
  }

  std::unique_ptr<MidiTimelineDataCache> cache;
  juce::MidiMessageSequence              sequence;
};

TEST_F (MidiTimelineDataCacheTest, InitialState)
{
  EXPECT_TRUE (cache->get_midi_events ().getNumEvents () == 0);
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, ClearMethod)
{
  // Add a MIDI sequence first
  cache->add_midi_sequence (
    { units::samples (0), units::samples (100) }, sequence);
  cache->finalize_changes ();

  // Verify sequences were added
  EXPECT_FALSE (cache->get_midi_events ().getNumEvents () == 0);
  EXPECT_TRUE (cache->has_content ());

  // Clear the cache
  cache->clear ();

  // Verify cache is empty
  EXPECT_TRUE (cache->get_midi_events ().getNumEvents () == 0);
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, AddSequenceAndFinalize)
{
  // Add MIDI sequence and finalize
  cache->add_midi_sequence (
    { units::samples (0), units::samples (100) }, sequence);
  cache->finalize_changes ();

  // Should have 4 events (2 note-ons + 2 note-offs)
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
  EXPECT_TRUE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, RemoveSequencesMatchingInterval_NoOverlap)
{
  // Create sequences that match their intervals
  juce::MidiMessageSequence seq1;
  seq1.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  seq1.addEvent (juce::MidiMessage::noteOff (1, 60), 25.0);

  juce::MidiMessageSequence seq2;
  seq2.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 160.0);
  seq2.addEvent (juce::MidiMessage::noteOff (1, 64), 175.0);

  // Add sequences that don't overlap with removal interval
  cache->add_midi_sequence (
    { units::samples (0), units::samples (50) },
    seq1); // Before removal interval
  cache->add_midi_sequence (
    { units::samples (150), units::samples (200) },
    seq2); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);

  // Remove sequences overlapping with [60, 140] - should remove nothing
  cache->remove_sequences_matching_interval (
    { units::samples (60), units::samples (140) });
  cache->finalize_changes ();

  // Both sequences should remain
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
}

TEST_F (MidiTimelineDataCacheTest, RemoveSequencesMatchingInterval_PartialOverlap)
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
    { units::samples (0), units::samples (100) },
    seq1); // Overlaps start of removal interval
  cache->add_midi_sequence (
    { units::samples (80), units::samples (180) },
    seq2); // Overlaps middle of removal interval
  cache->add_midi_sequence (
    { units::samples (150), units::samples (250) },
    seq3); // Overlaps end of removal interval
  cache->add_midi_sequence (
    { units::samples (200), units::samples (300) }, seq4); // No overlap
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 8);

  // Remove sequences overlapping with [90, 160]
  cache->remove_sequences_matching_interval (
    { units::samples (90), units::samples (160) });
  cache->finalize_changes ();

  // Only the non-overlapping sequence should remain
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 2);
}
TEST_F (MidiTimelineDataCacheTest, RemoveSequencesMatchingInterval_FullyContained)
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
  cache->add_midi_sequence (
    { units::samples (0), units::samples (50) }, seq1); // Before removal
  cache->add_midi_sequence (
    { units::samples (60), units::samples (90) }, seq2); // Fully within removal
  cache->add_midi_sequence (
    { units::samples (100), units::samples (150) }, seq3); // After removal
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 6);

  // Remove sequences overlapping with [60, 99] - avoid boundary conflicts
  cache->remove_sequences_matching_interval (
    { units::samples (60), units::samples (99) });
  cache->finalize_changes ();

  // Only sequences before and after should remain (sequence at {60,90} is
  // removed)
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 4);
}

TEST_F (MidiTimelineDataCacheTest, MultipleAddRemoveOperations)
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
  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq1);
  cache->add_midi_sequence ({ units::samples (50), units::samples (150) }, seq2);
  cache->add_midi_sequence (
    { units::samples (100), units::samples (200) }, seq3);
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 6);

  // Remove middle sequence by targeting its interval
  cache->remove_sequences_matching_interval (
    { units::samples (50), units::samples (149) });
  cache->finalize_changes ();

  // All sequences should be removed (all touch boundaries)
  EXPECT_EQ (cache->get_midi_events ().getNumEvents (), 0);
}

TEST_F (MidiTimelineDataCacheTest, EventsOutsideIntervalValidation)
{
  // Create a sequence with events outside the specified interval
  juce::MidiMessageSequence out_of_bounds_sequence;
  out_of_bounds_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0); // Within interval [0, 100]
  out_of_bounds_sequence.addEvent (
    juce::MidiMessage::noteOn (1, 64, 1.0f), 150.0); // Outside interval [0, 100]

  // Should throw exception when adding sequence with events outside interval
  EXPECT_THROW (
    cache->add_midi_sequence (
      { units::samples (0), units::samples (100) }, out_of_bounds_sequence),
    std::invalid_argument);
}

TEST_F (MidiTimelineDataCacheTest, UnendedNotesValidation)
{
  // Create a sequence with unended notes
  juce::MidiMessageSequence unended_sequence;
  unended_sequence.addEvent (juce::MidiMessage::noteOn (1, 60, 1.0f), 0.0);
  unended_sequence.addEvent (juce::MidiMessage::noteOn (1, 64, 1.0f), 10.0);
  // Intentionally missing note-off events

  // Add sequence with interval [0, 100]
  cache->add_midi_sequence (
    { units::samples (0), units::samples (100) }, unended_sequence);
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

TEST_F (MidiTimelineDataCacheTest, EmptyCacheOperations)
{
  // Operations on empty cache should not crash
  cache->clear ();
  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  EXPECT_TRUE (cache->get_midi_events ().getNumEvents () == 0);
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, ConstCorrectness)
{
  cache->add_midi_sequence (
    { units::samples (0), units::samples (100) }, sequence);
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

class AudioTimelineDataCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    cache = std::make_unique<AudioTimelineDataCache> ();

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

  std::unique_ptr<AudioTimelineDataCache> cache;
  juce::AudioSampleBuffer                 audio_buffer;
};

TEST_F (AudioTimelineDataCacheTest, InitialState)
{
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (AudioTimelineDataCacheTest, ClearMethod)
{
  // Add an audio region first
  cache->add_audio_region (
    { units::samples (0), units::samples (256) }, audio_buffer);
  cache->finalize_changes ();

  // Verify regions were added
  EXPECT_TRUE (cache->has_content ());

  // Clear the cache
  cache->clear ();

  // Verify cache is empty
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (AudioTimelineDataCacheTest, AddAudioRegionAndFinalize)
{
  // Add audio region and finalize
  cache->add_audio_region (
    { units::samples (0), units::samples (256) }, audio_buffer);
  cache->finalize_changes ();

  // Should have one audio region
  EXPECT_TRUE (cache->has_content ());
  EXPECT_EQ (cache->get_audio_regions ().size (), 1);

  // Verify the audio region properties
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions[0].start_sample, units::samples (0));
  EXPECT_EQ (regions[0].end_sample, units::samples (256));
  EXPECT_EQ (regions[0].audio_buffer.getNumChannels (), 2);
  EXPECT_EQ (regions[0].audio_buffer.getNumSamples (), 256);
}

TEST_F (AudioTimelineDataCacheTest, AddMultipleAudioRegions)
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
  cache->add_audio_region (
    { units::samples (0), units::samples (128) }, buffer1);
  cache->add_audio_region (
    { units::samples (256), units::samples (512) }, buffer2);
  cache->finalize_changes ();

  // Should have two regions sorted by start position
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions.size (), 2);
  EXPECT_EQ (regions[0].start_sample, units::samples (0));
  EXPECT_EQ (regions[1].start_sample, units::samples (256));

  // Verify the buffers are independent copies
  EXPECT_EQ (regions[0].audio_buffer.getReadPointer (0)[0], 1.0f);
  EXPECT_EQ (regions[1].audio_buffer.getReadPointer (0)[0], 2.0f);
}

TEST_F (AudioTimelineDataCacheTest, RemoveAudioRegionsMatchingInterval)
{
  // Create audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 128);
  juce::AudioSampleBuffer buffer3 (2, 128);

  buffer1.clear ();
  buffer2.clear ();
  buffer3.clear ();

  // Add regions at different positions
  cache->add_audio_region (
    { units::samples (0), units::samples (128) },
    buffer1); // Before removal interval
  cache->add_audio_region (
    { units::samples (200), units::samples (328) },
    buffer2); // Inside removal interval
  cache->add_audio_region (
    { units::samples (400), units::samples (528) },
    buffer3); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_audio_regions ().size (), 3);

  // Remove regions overlapping with [150, 350]
  cache->remove_sequences_matching_interval (
    { units::samples (150), units::samples (350) });
  cache->finalize_changes ();

  // Should have 2 regions remaining (before and after)
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions.size (), 2);
  EXPECT_EQ (regions[0].start_sample, units::samples (0));
  EXPECT_EQ (regions[1].start_sample, units::samples (400));
}

TEST_F (AudioTimelineDataCacheTest, AudioBufferIndependence)
{
  // Add an audio buffer
  cache->add_audio_region (
    { units::samples (0), units::samples (256) }, audio_buffer);
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

TEST_F (AudioTimelineDataCacheTest, AudioRegionSorting)
{
  // Create audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 128);
  juce::AudioSampleBuffer buffer3 (2, 128);

  buffer1.clear ();
  buffer2.clear ();
  buffer3.clear ();

  // Add regions in non-sequential order
  cache->add_audio_region (
    { units::samples (300), units::samples (428) }, buffer1); // Last
  cache->add_audio_region (
    { units::samples (100), units::samples (228) }, buffer2); // First
  cache->add_audio_region (
    { units::samples (200), units::samples (328) }, buffer3); // Middle
  cache->finalize_changes ();

  // Verify regions are sorted by start position
  const auto &regions = cache->get_audio_regions ();
  EXPECT_EQ (regions.size (), 3);
  EXPECT_EQ (regions[0].start_sample, units::samples (100));
  EXPECT_EQ (regions[1].start_sample, units::samples (200));
  EXPECT_EQ (regions[2].start_sample, units::samples (300));
}

TEST_F (AudioTimelineDataCacheTest, EmptyAudioOperations)
{
  // Operations on empty audio cache should not crash
  EXPECT_FALSE (cache->has_content ());
  EXPECT_EQ (cache->get_audio_regions ().size (), 0);

  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  EXPECT_FALSE (cache->has_content ());
}

// ========== Automation-Specific Tests ==========

class AutomationTimelineDataCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    cache = std::make_unique<AutomationTimelineDataCache> ();

    // Create a simple automation values vector for testing
    automation_values = {
      0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f
    };
  }

  std::unique_ptr<AutomationTimelineDataCache> cache;
  std::vector<float>                           automation_values;
};

TEST_F (AutomationTimelineDataCacheTest, InitialState)
{
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (AutomationTimelineDataCacheTest, ClearMethod)
{
  // Add an automation sequence first
  cache->add_automation_sequence (
    { units::samples (0), units::samples (256) }, automation_values);
  cache->finalize_changes ();

  // Verify sequences were added
  EXPECT_TRUE (cache->has_content ());

  // Clear the cache
  cache->clear ();

  // Verify cache is empty
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (AutomationTimelineDataCacheTest, AddAutomationSequenceAndFinalize)
{
  // Add automation sequence and finalize
  cache->add_automation_sequence (
    { units::samples (0), units::samples (256) }, automation_values);
  cache->finalize_changes ();

  // Should have one automation sequence
  EXPECT_TRUE (cache->has_content ());
  EXPECT_EQ (cache->get_automation_sequences ().size (), 1);

  // Verify the automation sequence properties
  const auto &sequences = cache->get_automation_sequences ();
  EXPECT_EQ (sequences[0].start_sample, units::samples (0));
  EXPECT_EQ (sequences[0].end_sample, units::samples (256));
  EXPECT_EQ (sequences[0].automation_values.size (), automation_values.size ());
}

TEST_F (AutomationTimelineDataCacheTest, AddMultipleAutomationSequences)
{
  // Create different automation values
  std::vector<float> automation_values2 = {
    1.0f, 0.9f, 0.8f, 0.7f, 0.6f, 0.5f, 0.4f, 0.3f, 0.2f, 0.1f, 0.0f
  };

  // Add sequences at different positions
  cache->add_automation_sequence (
    { units::samples (0), units::samples (128) }, automation_values);
  cache->add_automation_sequence (
    { units::samples (256), units::samples (384) }, automation_values2);
  cache->finalize_changes ();

  // Should have two sequences sorted by start position
  const auto &sequences = cache->get_automation_sequences ();
  EXPECT_EQ (sequences.size (), 2);
  EXPECT_EQ (sequences[0].start_sample, units::samples (0));
  EXPECT_EQ (sequences[1].start_sample, units::samples (256));
}

TEST_F (
  AutomationTimelineDataCacheTest,
  RemoveAutomationSequencesMatchingInterval)
{
  // Add sequences at different positions
  cache->add_automation_sequence (
    { units::samples (0), units::samples (128) },
    automation_values); // Before removal interval
  cache->add_automation_sequence (
    { units::samples (200), units::samples (328) },
    automation_values); // Inside removal interval
  cache->add_automation_sequence (
    { units::samples (400), units::samples (528) },
    automation_values); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->get_automation_sequences ().size (), 3);

  // Remove sequences overlapping with [150, 350]
  cache->remove_sequences_matching_interval (
    { units::samples (150), units::samples (350) });
  cache->finalize_changes ();

  // Should have 2 sequences remaining (before and after)
  const auto &sequences = cache->get_automation_sequences ();
  EXPECT_EQ (sequences.size (), 2);
  EXPECT_EQ (sequences[0].start_sample, units::samples (0));
  EXPECT_EQ (sequences[1].start_sample, units::samples (400));
}

TEST_F (AutomationTimelineDataCacheTest, AutomationSequenceSorting)
{
  // Add sequences in non-sequential order
  cache->add_automation_sequence (
    { units::samples (300), units::samples (428) },
    automation_values); // Last
  cache->add_automation_sequence (
    { units::samples (100), units::samples (228) },
    automation_values); // First
  cache->add_automation_sequence (
    { units::samples (200), units::samples (328) },
    automation_values); // Middle
  cache->finalize_changes ();

  // Verify sequences are sorted by start position
  const auto &sequences = cache->get_automation_sequences ();
  EXPECT_EQ (sequences.size (), 3);
  EXPECT_EQ (sequences[0].start_sample, units::samples (100));
  EXPECT_EQ (sequences[1].start_sample, units::samples (200));
  EXPECT_EQ (sequences[2].start_sample, units::samples (300));
}

TEST_F (AutomationTimelineDataCacheTest, EmptyAutomationOperations)
{
  // Operations on empty automation cache should not crash
  EXPECT_FALSE (cache->has_content ());
  EXPECT_EQ (cache->get_automation_sequences ().size (), 0);

  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  EXPECT_FALSE (cache->has_content ());
}

} // namespace zrythm::dsp
