// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/midi_event.h"
#include "dsp/timeline_data_cache.h"
#include "utils/midi.h"
#include "utils/qt.h"

#include <QSignalSpy>

#include "helpers/scoped_qcoreapplication.h"

#include <gtest/gtest.h>

using namespace zrythm::test_helpers;

namespace zrythm::dsp
{

class MidiTimelineDataCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<ScopedQCoreApplication> ();
    cache = utils::make_qobject_unique<dsp::MidiTimelineDataCache> ();
  }

  utils::QObjectUniquePtr<dsp::MidiTimelineDataCache> cache;

private:
  std::unique_ptr<ScopedQCoreApplication> app_;
};

TEST_F (MidiTimelineDataCacheTest, InitialState)
{
  EXPECT_TRUE (cache->midi_events ().empty ());
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, ClearMethod)
{
  // Add a MIDI sequence first
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (1)),
    midi_event::make_note_on (0, 64, 127, units::samples (2)),
    midi_event::make_note_off (0, 64, units::samples (3)),
  };
  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq);
  cache->finalize_changes ();

  // Verify sequences were added
  EXPECT_FALSE (cache->midi_events ().empty ());
  EXPECT_TRUE (cache->has_content ());

  // Clear the cache
  cache->clear ();

  // Verify cache is empty
  EXPECT_TRUE (cache->midi_events ().empty ());
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, AddSequenceAndFinalize)
{
  // Add MIDI sequence and finalize
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (1)),
    midi_event::make_note_on (0, 64, 127, units::samples (2)),
    midi_event::make_note_off (0, 64, units::samples (3)),
  };
  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq);
  cache->finalize_changes ();

  // Should have 4 events (2 note-ons + 2 note-offs)
  EXPECT_EQ (cache->midi_events ().size (), 4);
  EXPECT_TRUE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, RemoveSequencesMatchingInterval_NoOverlap)
{
  // Create sequences that match their intervals
  std::vector<SampleBasedMidiEvent> seq1{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (25)),
  };

  std::vector<SampleBasedMidiEvent> seq2{
    midi_event::make_note_on (0, 64, 127, units::samples (160)),
    midi_event::make_note_off (0, 64, units::samples (175)),
  };

  // Add sequences that don't overlap with removal interval
  cache->add_midi_sequence (
    { units::samples (0), units::samples (50) },
    seq1); // Before removal interval
  cache->add_midi_sequence (
    { units::samples (150), units::samples (200) },
    seq2); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->midi_events ().size (), 4);

  // Remove sequences overlapping with [60, 140] - should remove nothing
  cache->remove_sequences_matching_interval (
    { units::samples (60), units::samples (140) });
  cache->finalize_changes ();

  // Both sequences should remain
  EXPECT_EQ (cache->midi_events ().size (), 4);
}

TEST_F (MidiTimelineDataCacheTest, RemoveSequencesMatchingInterval_PartialOverlap)
{
  // Create sequences that match their intervals
  std::vector<SampleBasedMidiEvent> seq1{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (50)),
  };

  std::vector<SampleBasedMidiEvent> seq2{
    midi_event::make_note_on (0, 64, 127, units::samples (90)),
    midi_event::make_note_off (0, 64, units::samples (170)),
  };

  std::vector<SampleBasedMidiEvent> seq3{
    midi_event::make_note_on (0, 67, 127, units::samples (160)),
    midi_event::make_note_off (0, 67, units::samples (240)),
  };

  std::vector<SampleBasedMidiEvent> seq4{
    midi_event::make_note_on (0, 69, 127, units::samples (210)),
    midi_event::make_note_off (0, 69, units::samples (290)),
  };

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

  EXPECT_EQ (cache->midi_events ().size (), 8);

  // Remove sequences overlapping with [90, 160]
  cache->remove_sequences_matching_interval (
    { units::samples (90), units::samples (160) });
  cache->finalize_changes ();

  // Only the non-overlapping sequence should remain
  EXPECT_EQ (cache->midi_events ().size (), 2);
}
TEST_F (MidiTimelineDataCacheTest, RemoveSequencesMatchingInterval_FullyContained)
{
  // Create sequences that match their intervals
  std::vector<SampleBasedMidiEvent> seq1{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (25)),
  };

  std::vector<SampleBasedMidiEvent> seq2{
    midi_event::make_note_on (0, 64, 127, units::samples (70)),
    midi_event::make_note_off (0, 64, units::samples (85)),
  };

  std::vector<SampleBasedMidiEvent> seq3{
    midi_event::make_note_on (0, 67, 127, units::samples (110)),
    midi_event::make_note_off (0, 67, units::samples (140)),
  };

  // Add sequences where one is fully contained within removal interval
  cache->add_midi_sequence (
    { units::samples (0), units::samples (50) }, seq1); // Before removal
  cache->add_midi_sequence (
    { units::samples (60), units::samples (90) }, seq2); // Fully within removal
  cache->add_midi_sequence (
    { units::samples (100), units::samples (150) }, seq3); // After removal
  cache->finalize_changes ();

  EXPECT_EQ (cache->midi_events ().size (), 6);

  // Remove sequences overlapping with [60, 99] - avoid boundary conflicts
  cache->remove_sequences_matching_interval (
    { units::samples (60), units::samples (99) });
  cache->finalize_changes ();

  // Only sequences before and after should remain (sequence at {60,90} is
  // removed)
  EXPECT_EQ (cache->midi_events ().size (), 4);
}

// Regression test for gitlab #5240: adjacent intervals (end == start) must not
// be considered overlapping during removal.
TEST_F (
  MidiTimelineDataCacheTest,
  RemoveSequencesMatchingInterval_AdjacentNotOverlapping)
{
  // Two adjacent clips: R1 at [0, 100) and R2 at [100, 200)
  std::vector<SampleBasedMidiEvent> seq1{
    midi_event::make_note_on (0, 60, 127, units::samples (10)),
    midi_event::make_note_off (0, 60, units::samples (90)),
  };

  std::vector<SampleBasedMidiEvent> seq2{
    midi_event::make_note_on (0, 64, 127, units::samples (110)),
    midi_event::make_note_off (0, 64, units::samples (190)),
  };

  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq1);
  cache->add_midi_sequence (
    { units::samples (100), units::samples (200) }, seq2);
  cache->finalize_changes ();

  EXPECT_EQ (cache->midi_events ().size (), 4);

  // Remove overlapping with [100, 200) — R1 at [0, 100) is adjacent, not
  // overlapping
  cache->remove_sequences_matching_interval (
    { units::samples (100), units::samples (200) });
  cache->finalize_changes ();

  // R1 at [0, 100) should remain
  EXPECT_EQ (cache->midi_events ().size (), 2);
}

// Reverse direction: [0, 100) removal must not evict [100, 200)
TEST_F (
  MidiTimelineDataCacheTest,
  RemoveSequencesMatchingInterval_AdjacentReverse)
{
  std::vector<SampleBasedMidiEvent> seq1{
    midi_event::make_note_on (0, 60, 127, units::samples (10)),
    midi_event::make_note_off (0, 60, units::samples (90)),
  };

  std::vector<SampleBasedMidiEvent> seq2{
    midi_event::make_note_on (0, 64, 127, units::samples (110)),
    midi_event::make_note_off (0, 64, units::samples (190)),
  };

  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq1);
  cache->add_midi_sequence (
    { units::samples (100), units::samples (200) }, seq2);
  cache->finalize_changes ();

  EXPECT_EQ (cache->midi_events ().size (), 4);

  // Remove overlapping with [0, 100) - should only remove seq1
  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  // seq2 at [100, 200) should remain
  EXPECT_EQ (cache->midi_events ().size (), 2);
}

TEST_F (MidiTimelineDataCacheTest, MultipleAddRemoveOperations)
{
  // Create sequences that match their intervals
  std::vector<SampleBasedMidiEvent> seq1{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (50)),
  };

  std::vector<SampleBasedMidiEvent> seq2{
    midi_event::make_note_on (0, 64, 127, units::samples (70)),
    midi_event::make_note_off (0, 64, units::samples (140)),
  };

  std::vector<SampleBasedMidiEvent> seq3{
    midi_event::make_note_on (0, 67, 127, units::samples (110)),
    midi_event::make_note_off (0, 67, units::samples (190)),
  };

  // Add multiple overlapping sequences
  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq1);
  cache->add_midi_sequence ({ units::samples (50), units::samples (150) }, seq2);
  cache->add_midi_sequence (
    { units::samples (100), units::samples (200) }, seq3);
  cache->finalize_changes ();

  EXPECT_EQ (cache->midi_events ().size (), 6);

  // Remove middle sequence by targeting its interval
  cache->remove_sequences_matching_interval (
    { units::samples (50), units::samples (149) });
  cache->finalize_changes ();

  // All sequences should be removed (all truly overlap with [50, 149])
  EXPECT_EQ (cache->midi_events ().size (), 0);
}

TEST_F (MidiTimelineDataCacheTest, EventsOutsideIntervalValidation)
{
  // Create a sequence with events outside the specified interval
  std::vector<SampleBasedMidiEvent> out_of_bounds_sequence{
    midi_event::make_note_on (0, 60, 127, units::samples (0)), // Within interval
                                                               // [0, 100]
    midi_event::make_note_on (
      0, 64, 127, units::samples (150)), // Outside interval [0, 100]
  };

  // Should throw exception when adding sequence with events outside interval
  EXPECT_THROW (
    cache->add_midi_sequence (
      { units::samples (0), units::samples (100) }, out_of_bounds_sequence),
    std::invalid_argument);
}

TEST_F (MidiTimelineDataCacheTest, ReversedIntervalThrows)
{
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (10)),
  };

  EXPECT_THROW (
    cache->add_midi_sequence ({ units::samples (100), units::samples (0) }, seq),
    std::invalid_argument);
}

TEST_F (MidiTimelineDataCacheTest, UnendedNotesValidation)
{
  // Create a sequence with unended notes
  std::vector<SampleBasedMidiEvent> unended_sequence{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_on (0, 64, 127, units::samples (10)),
    // Intentionally missing note-off events
  };

  // Add sequence with interval [0, 100]
  cache->add_midi_sequence (
    { units::samples (0), units::samples (100) }, unended_sequence);
  cache->finalize_changes ();

  const auto &merged = cache->midi_events ();

  // Should have note-off events added automatically at interval end time (100)
  EXPECT_EQ (merged.size (), 4); // 2 note-ons + 2 note-offs

  // Verify note-off events were added at the correct time
  int note_off_count = 0;
  for (const auto &ev : merged)
    {
      if (utils::midi::midi_is_note_off (ev.data ()))
        {
          note_off_count++;
          EXPECT_EQ (ev.time_, units::samples (100));
        }
    }
  EXPECT_EQ (note_off_count, 2);
}

// Two same-pitch note-ons without any note-offs each generate their own note-off.
TEST_F (MidiTimelineDataCacheTest, SamePitchOverlappingNoteOnsGetSeparateNoteOffs)
{
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_on (0, 60, 127, units::samples (50)),
  };

  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq);
  cache->finalize_changes ();

  const auto &merged = cache->midi_events ();
  ASSERT_EQ (merged.size (), 4)
    << "Expected 2 note-ons + 2 auto-generated note-offs";

  int note_off_count = 0;
  for (const auto &ev : merged)
    {
      if (utils::midi::midi_is_note_off (ev.data ()))
        ++note_off_count;
    }
  EXPECT_EQ (note_off_count, 2);
}

// Regression test for GitLab #5279: when two adjacent same-pitch notes are
// rendered, JUCE's addEvent sorts noteOn before noteOff. Then
// updateMatchedPairs auto-generates a spurious noteOff(vel=0) which kills
// the second note.
TEST_F (MidiTimelineDataCacheTest, AdjacentSamePitchNotesNoSpuriousNoteOff)
{
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (
      0, 60, static_cast<midi_byte_t> (90), units::samples (0)),
    midi_event::make_note_on (
      0, 60, static_cast<midi_byte_t> (90), units::samples (100)),
    midi_event::make_note_off (0, 60, units::samples (100)),
    midi_event::make_note_off (0, 60, units::samples (200)),
  };

  // Confirm the problematic ordering: noteOn@100 before noteOff@100
  ASSERT_EQ (seq.size (), 4);
  ASSERT_TRUE (utils::midi::midi_is_note_on (seq[1].data ()));
  ASSERT_TRUE (utils::midi::midi_is_note_off (seq[2].data ()));

  cache->add_midi_sequence ({ units::samples (0), units::samples (200) }, seq);
  cache->finalize_changes ();

  const auto &merged = cache->midi_events ();
  ASSERT_EQ (merged.size (), 4)
    << "updateMatchedPairs must not auto-generate spurious noteOff events";
}

// At the boundary between two adjacent same-pitch notes, the merged output
// must send noteOff (release voice 1) before noteOn (start voice 2). Some
// synths use last-voice matching: noteOff for pitch P ends the most recently
// started voice for P.
TEST_F (
  MidiTimelineDataCacheTest,
  AdjacentSamePitchNotesNoteOffBeforeNoteOnAtBoundary)
{
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (
      0, 60, static_cast<midi_byte_t> (90), units::samples (0)),
    midi_event::make_note_on (
      0, 60, static_cast<midi_byte_t> (90), units::samples (100)),
    midi_event::make_note_off (0, 60, units::samples (100)),
    midi_event::make_note_off (0, 60, units::samples (200)),
  };

  cache->add_midi_sequence ({ units::samples (0), units::samples (200) }, seq);
  cache->finalize_changes ();

  const auto &merged = cache->midi_events ();
  ASSERT_EQ (merged.size (), 4);

  for (size_t i = 0; i + 1 < merged.size (); ++i)
    {
      if (merged[i].time_ == merged[i + 1].time_)
        {
          ASSERT_FALSE (
            utils::midi::midi_is_note_on (merged[i].data ())
            && utils::midi::midi_is_note_off (merged[i + 1].data ()))
            << "At timestamp " << merged[i].time_.in (units::samples)
            << ": noteOff must precede noteOn for correct voice allocation, "
            << "but got noteOn then noteOff";
        }
    }
}

TEST_F (MidiTimelineDataCacheTest, CachedRangesSignalOnFinalize)
{
  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);

  std::vector<SampleBasedMidiEvent> seq1{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (25)),
  };

  std::vector<SampleBasedMidiEvent> seq2{
    midi_event::make_note_on (0, 64, 127, units::samples (110)),
    midi_event::make_note_off (0, 64, units::samples (150)),
  };

  cache->add_midi_sequence ({ units::samples (0), units::samples (50) }, seq1);
  cache->add_midi_sequence (
    { units::samples (100), units::samples (200) }, seq2);
  cache->finalize_changes ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  ASSERT_EQ (ranges.size (), 2);
  EXPECT_EQ (ranges[0].first, units::samples (0));
  EXPECT_EQ (ranges[0].second, units::samples (50));
  EXPECT_EQ (ranges[1].first, units::samples (100));
  EXPECT_EQ (ranges[1].second, units::samples (200));
}

TEST_F (MidiTimelineDataCacheTest, CachedRangesEmptyWhenNoContent)
{
  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);

  cache->finalize_changes ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  EXPECT_TRUE (ranges.empty ());
}

TEST_F (MidiTimelineDataCacheTest, CachedRangesClearedOnClear)
{
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (50)),
    midi_event::make_note_on (0, 64, 127, units::samples (60)),
    midi_event::make_note_off (0, 64, units::samples (80)),
  };
  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq);
  cache->finalize_changes ();

  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);
  cache->clear ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  EXPECT_TRUE (ranges.empty ());
}

TEST_F (MidiTimelineDataCacheTest, EmptyCacheOperations)
{
  // Operations on empty cache should not crash
  cache->clear ();
  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  EXPECT_TRUE (cache->midi_events ().size () == 0);
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (MidiTimelineDataCacheTest, ConstCorrectness)
{
  std::vector<SampleBasedMidiEvent> seq{
    midi_event::make_note_on (0, 60, 127, units::samples (0)),
    midi_event::make_note_off (0, 60, units::samples (50)),
    midi_event::make_note_on (0, 64, 127, units::samples (60)),
    midi_event::make_note_off (0, 64, units::samples (80)),
  };
  cache->add_midi_sequence ({ units::samples (0), units::samples (100) }, seq);
  cache->finalize_changes ();

  // cached_events() should return const reference
  const auto &events = cache->midi_events ();
  // Should have 4 events: 2 note-ons + 2 note-offs
  EXPECT_EQ (events.size (), 4);

  // Verify we can't modify the returned reference (compile-time check)
  static_assert (
    std::is_same_v<
      decltype (cache->midi_events ()), std::span<const SampleBasedMidiEvent>>);
}

// ========== Audio-Specific Tests ==========

class AudioTimelineDataCacheTest : public ::testing::Test
{
protected:
  void SetUp () override
  {
    app_ = std::make_unique<ScopedQCoreApplication> ();
    cache = utils::make_qobject_unique<dsp::AudioTimelineDataCache> ();

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

  utils::QObjectUniquePtr<dsp::AudioTimelineDataCache> cache;
  juce::AudioSampleBuffer                              audio_buffer;

private:
  std::unique_ptr<ScopedQCoreApplication> app_;
};

TEST_F (AudioTimelineDataCacheTest, InitialState)
{
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (AudioTimelineDataCacheTest, ClearMethod)
{
  // Add an audio clip first
  cache->add_audio_clip (
    { units::samples (0), units::samples (256) }, audio_buffer);
  cache->finalize_changes ();

  // Verify clips were added
  EXPECT_TRUE (cache->has_content ());

  // Clear the cache
  cache->clear ();

  // Verify cache is empty
  EXPECT_FALSE (cache->has_content ());
}

TEST_F (AudioTimelineDataCacheTest, AddAudioClipAndFinalize)
{
  // Add audio clip and finalize
  cache->add_audio_clip (
    { units::samples (0), units::samples (256) }, audio_buffer);
  cache->finalize_changes ();

  // Should have one audio clip
  EXPECT_TRUE (cache->has_content ());
  EXPECT_EQ (cache->audio_clips ().size (), 1);

  // Verify the audio clip properties
  const auto &clips = cache->audio_clips ();
  EXPECT_EQ (clips[0].start_sample, units::samples (0));
  EXPECT_EQ (clips[0].end_sample, units::samples (256));
  EXPECT_EQ (clips[0].audio_buffer.getNumChannels (), 2);
  EXPECT_EQ (clips[0].audio_buffer.getNumSamples (), 256);
}

TEST_F (AudioTimelineDataCacheTest, ReversedIntervalThrows)
{
  EXPECT_THROW (
    cache->add_audio_clip (
      { units::samples (200), units::samples (100) }, audio_buffer),
    std::invalid_argument);
}

TEST_F (AudioTimelineDataCacheTest, AddMultipleAudioClips)
{
  // Create multiple audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 256);

  buffer1.clear ();
  buffer2.clear ();

  // Fill with different patterns
  buffer1.getWritePointer (0)[0] = 1.0f; // Mark buffer1
  buffer2.getWritePointer (0)[0] = 2.0f; // Mark buffer2

  // Add clips at different positions
  cache->add_audio_clip ({ units::samples (0), units::samples (128) }, buffer1);
  cache->add_audio_clip (
    { units::samples (256), units::samples (512) }, buffer2);
  cache->finalize_changes ();

  // Should have two clips sorted by start position
  const auto &clips = cache->audio_clips ();
  EXPECT_EQ (clips.size (), 2);
  EXPECT_EQ (clips[0].start_sample, units::samples (0));
  EXPECT_EQ (clips[1].start_sample, units::samples (256));

  // Verify the buffers are independent copies
  EXPECT_EQ (clips[0].audio_buffer.getReadPointer (0)[0], 1.0f);
  EXPECT_EQ (clips[1].audio_buffer.getReadPointer (0)[0], 2.0f);
}

TEST_F (AudioTimelineDataCacheTest, RemoveAudioClipsMatchingInterval)
{
  // Create audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 128);
  juce::AudioSampleBuffer buffer3 (2, 128);

  buffer1.clear ();
  buffer2.clear ();
  buffer3.clear ();

  // Add clips at different positions
  cache->add_audio_clip (
    { units::samples (0), units::samples (128) },
    buffer1); // Before removal interval
  cache->add_audio_clip (
    { units::samples (200), units::samples (328) },
    buffer2); // Inside removal interval
  cache->add_audio_clip (
    { units::samples (400), units::samples (528) },
    buffer3); // After removal interval
  cache->finalize_changes ();

  EXPECT_EQ (cache->audio_clips ().size (), 3);

  // Remove clips overlapping with [150, 350]
  cache->remove_sequences_matching_interval (
    { units::samples (150), units::samples (350) });
  cache->finalize_changes ();

  // Should have 2 clips remaining (before and after)
  const auto &clips = cache->audio_clips ();
  EXPECT_EQ (clips.size (), 2);
  EXPECT_EQ (clips[0].start_sample, units::samples (0));
  EXPECT_EQ (clips[1].start_sample, units::samples (400));
}

// Regression test for gitlab #5240 (audio variant)
TEST_F (AudioTimelineDataCacheTest, RemoveAudioClipsMatchingInterval_Adjacent)
{
  juce::AudioSampleBuffer buffer1 (2, 100);
  juce::AudioSampleBuffer buffer2 (2, 100);
  buffer1.clear ();
  buffer2.clear ();

  // Adjacent clips: [0, 100) and [100, 200)
  cache->add_audio_clip ({ units::samples (0), units::samples (100) }, buffer1);
  cache->add_audio_clip (
    { units::samples (100), units::samples (200) }, buffer2);
  cache->finalize_changes ();

  EXPECT_EQ (cache->audio_clips ().size (), 2);

  // Remove overlapping with [100, 200) — buffer1 at [0, 100) is adjacent, not
  // overlapping
  cache->remove_sequences_matching_interval (
    { units::samples (100), units::samples (200) });
  cache->finalize_changes ();

  // buffer1 at [0, 100) should remain
  const auto &clips = cache->audio_clips ();
  EXPECT_EQ (clips.size (), 1);
  EXPECT_EQ (clips[0].start_sample, units::samples (0));
}

// Reverse direction: [0, 100) removal must not evict [100, 200)
TEST_F (
  AudioTimelineDataCacheTest,
  RemoveAudioClipsMatchingInterval_AdjacentReverse)
{
  juce::AudioSampleBuffer buffer1 (2, 100);
  juce::AudioSampleBuffer buffer2 (2, 100);
  buffer1.clear ();
  buffer2.clear ();

  cache->add_audio_clip ({ units::samples (0), units::samples (100) }, buffer1);
  cache->add_audio_clip (
    { units::samples (100), units::samples (200) }, buffer2);
  cache->finalize_changes ();

  EXPECT_EQ (cache->audio_clips ().size (), 2);

  // Remove overlapping with [0, 100) — buffer2 at [100, 200) is adjacent, not
  // overlapping
  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  // buffer2 at [100, 200) should remain
  const auto &clips = cache->audio_clips ();
  EXPECT_EQ (clips.size (), 1);
  EXPECT_EQ (clips[0].start_sample, units::samples (100));
}

TEST_F (AudioTimelineDataCacheTest, AudioBufferIndependence)
{
  // Add an audio buffer
  cache->add_audio_clip (
    { units::samples (0), units::samples (256) }, audio_buffer);
  cache->finalize_changes ();

  // Modify the original buffer
  audio_buffer.setSample (0, 0, 0.2f);

  // Verify the cached buffer is unaffected (independent copy)
  const auto &clips = cache->audio_clips ();
  EXPECT_FLOAT_EQ (clips[0].audio_buffer.getReadPointer (0)[0], 0.5f);

  // Modify the cached buffer
  const_cast<juce::AudioSampleBuffer &> (clips[0].audio_buffer).clear ();

  // Verify the original buffer is still unaffected
  EXPECT_FLOAT_EQ (audio_buffer.getReadPointer (0)[0], 0.2f);
}

TEST_F (AudioTimelineDataCacheTest, AudioClipSorting)
{
  // Create audio buffers
  juce::AudioSampleBuffer buffer1 (2, 128);
  juce::AudioSampleBuffer buffer2 (2, 128);
  juce::AudioSampleBuffer buffer3 (2, 128);

  buffer1.clear ();
  buffer2.clear ();
  buffer3.clear ();

  // Add clips in non-sequential order
  cache->add_audio_clip (
    { units::samples (300), units::samples (428) }, buffer1); // Last
  cache->add_audio_clip (
    { units::samples (100), units::samples (228) }, buffer2); // First
  cache->add_audio_clip (
    { units::samples (200), units::samples (328) }, buffer3); // Middle
  cache->finalize_changes ();

  // Verify clips are sorted by start position
  const auto &clips = cache->audio_clips ();
  EXPECT_EQ (clips.size (), 3);
  EXPECT_EQ (clips[0].start_sample, units::samples (100));
  EXPECT_EQ (clips[1].start_sample, units::samples (200));
  EXPECT_EQ (clips[2].start_sample, units::samples (300));
}

TEST_F (AudioTimelineDataCacheTest, CachedRangesSignalOnFinalize)
{
  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);

  juce::AudioSampleBuffer buffer1 (1, 64);
  juce::AudioSampleBuffer buffer2 (1, 64);
  buffer1.clear ();
  buffer2.clear ();

  cache->add_audio_clip (
    { units::samples (100), units::samples (200) }, buffer1);
  cache->add_audio_clip (
    { units::samples (300), units::samples (400) }, buffer2);
  cache->finalize_changes ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  ASSERT_EQ (ranges.size (), 2);
  EXPECT_EQ (ranges[0].first, units::samples (100));
  EXPECT_EQ (ranges[0].second, units::samples (200));
  EXPECT_EQ (ranges[1].first, units::samples (300));
  EXPECT_EQ (ranges[1].second, units::samples (400));
}

TEST_F (AudioTimelineDataCacheTest, CachedRangesEmptyWhenNoContent)
{
  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);

  cache->finalize_changes ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  EXPECT_TRUE (ranges.empty ());
}

TEST_F (AudioTimelineDataCacheTest, CachedRangesClearedOnClear)
{
  juce::AudioSampleBuffer buffer (1, 64);
  buffer.clear ();

  cache->add_audio_clip ({ units::samples (0), units::samples (64) }, buffer);
  cache->finalize_changes ();

  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);
  cache->clear ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  EXPECT_TRUE (ranges.empty ());
}

TEST_F (AudioTimelineDataCacheTest, EmptyAudioOperations)
{
  // Operations on empty audio cache should not crash
  EXPECT_FALSE (cache->has_content ());
  EXPECT_EQ (cache->audio_clips ().size (), 0);

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
    app_ = std::make_unique<ScopedQCoreApplication> ();
    cache = utils::make_qobject_unique<dsp::AutomationTimelineDataCache> ();

    // Create a simple automation values vector for testing
    automation_values = {
      0.0f, 0.1f, 0.2f, 0.3f, 0.4f, 0.5f, 0.6f, 0.7f, 0.8f, 0.9f, 1.0f
    };
  }

  utils::QObjectUniquePtr<dsp::AutomationTimelineDataCache> cache;
  std::vector<float>                                        automation_values;

private:
  std::unique_ptr<ScopedQCoreApplication> app_;
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
  EXPECT_EQ (cache->automation_sequences ().size (), 1);

  // Verify the automation sequence properties
  const auto &sequences = cache->automation_sequences ();
  EXPECT_EQ (sequences[0].start_sample, units::samples (0));
  EXPECT_EQ (sequences[0].end_sample, units::samples (256));
  EXPECT_EQ (sequences[0].automation_values.size (), automation_values.size ());
}

TEST_F (AutomationTimelineDataCacheTest, ReversedIntervalThrows)
{
  EXPECT_THROW (
    cache->add_automation_sequence (
      { units::samples (200), units::samples (100) }, automation_values),
    std::invalid_argument);
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
  const auto &sequences = cache->automation_sequences ();
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

  EXPECT_EQ (cache->automation_sequences ().size (), 3);

  // Remove sequences overlapping with [150, 350]
  cache->remove_sequences_matching_interval (
    { units::samples (150), units::samples (350) });
  cache->finalize_changes ();

  // Should have 2 sequences remaining (before and after)
  const auto &sequences = cache->automation_sequences ();
  EXPECT_EQ (sequences.size (), 2);
  EXPECT_EQ (sequences[0].start_sample, units::samples (0));
  EXPECT_EQ (sequences[1].start_sample, units::samples (400));
}

// Regression test for gitlab #5240 (automation variant)
TEST_F (
  AutomationTimelineDataCacheTest,
  RemoveAutomationSequencesMatchingInterval_Adjacent)
{
  // Adjacent sequences: [0, 100) and [100, 200)
  cache->add_automation_sequence (
    { units::samples (0), units::samples (100) }, automation_values);
  cache->add_automation_sequence (
    { units::samples (100), units::samples (200) }, automation_values);
  cache->finalize_changes ();

  EXPECT_EQ (cache->automation_sequences ().size (), 2);

  // Remove overlapping with [100, 200) — first entry at [0, 100) is adjacent,
  // not overlapping
  cache->remove_sequences_matching_interval (
    { units::samples (100), units::samples (200) });
  cache->finalize_changes ();

  // First sequence at [0, 100) should remain
  const auto &sequences = cache->automation_sequences ();
  EXPECT_EQ (sequences.size (), 1);
  EXPECT_EQ (sequences[0].start_sample, units::samples (0));
}

// Reverse direction: [0, 100) removal must not evict [100, 200)
TEST_F (
  AutomationTimelineDataCacheTest,
  RemoveAutomationSequencesMatchingInterval_AdjacentReverse)
{
  cache->add_automation_sequence (
    { units::samples (0), units::samples (100) }, automation_values);
  cache->add_automation_sequence (
    { units::samples (100), units::samples (200) }, automation_values);
  cache->finalize_changes ();

  EXPECT_EQ (cache->automation_sequences ().size (), 2);

  // Remove overlapping with [0, 100) — second entry at [100, 200) is adjacent,
  // not overlapping
  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  // Second sequence at [100, 200) should remain
  const auto &sequences = cache->automation_sequences ();
  EXPECT_EQ (sequences.size (), 1);
  EXPECT_EQ (sequences[0].start_sample, units::samples (100));
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
  const auto &sequences = cache->automation_sequences ();
  EXPECT_EQ (sequences.size (), 3);
  EXPECT_EQ (sequences[0].start_sample, units::samples (100));
  EXPECT_EQ (sequences[1].start_sample, units::samples (200));
  EXPECT_EQ (sequences[2].start_sample, units::samples (300));
}

TEST_F (AutomationTimelineDataCacheTest, CachedRangesSignalOnFinalize)
{
  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);

  cache->add_automation_sequence (
    { units::samples (100), units::samples (200) }, automation_values);
  cache->add_automation_sequence (
    { units::samples (300), units::samples (400) }, automation_values);
  cache->finalize_changes ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  ASSERT_EQ (ranges.size (), 2);
  EXPECT_EQ (ranges[0].first, units::samples (100));
  EXPECT_EQ (ranges[0].second, units::samples (200));
  EXPECT_EQ (ranges[1].first, units::samples (300));
  EXPECT_EQ (ranges[1].second, units::samples (400));
}

TEST_F (AutomationTimelineDataCacheTest, CachedRangesEmptyWhenNoContent)
{
  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);

  cache->finalize_changes ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  EXPECT_TRUE (ranges.empty ());
}

TEST_F (AutomationTimelineDataCacheTest, CachedRangesClearedOnClear)
{
  cache->add_automation_sequence (
    { units::samples (0), units::samples (256) }, automation_values);
  cache->finalize_changes ();

  QSignalSpy spy (cache.get (), &TimelineDataCache::cachedRangesChanged);
  cache->clear ();

  ASSERT_EQ (spy.count (), 1);
  auto ranges =
    spy.at (0).at (0).value<std::vector<TimelineDataCache::IntervalType>> ();
  EXPECT_TRUE (ranges.empty ());
}

TEST_F (AutomationTimelineDataCacheTest, EmptyAutomationOperations)
{
  // Operations on empty automation cache should not crash
  EXPECT_FALSE (cache->has_content ());
  EXPECT_EQ (cache->automation_sequences ().size (), 0);

  cache->remove_sequences_matching_interval (
    { units::samples (0), units::samples (100) });
  cache->finalize_changes ();

  EXPECT_FALSE (cache->has_content ());
}

} // namespace zrythm::dsp
