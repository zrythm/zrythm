// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>

#include "dsp/timeline_data_cache.h"

namespace zrythm::dsp
{

// ========== MidiTimelineDataCache Implementation ==========

void
MidiTimelineDataCache::clear ()
{
  midi_sequences_.clear ();
  merged_midi_events_.clear ();
}

void
MidiTimelineDataCache::remove_sequences_matching_interval (IntervalType interval)
{
  const auto [target_start, target_end] = interval;

  // Remove MIDI sequences that overlap with the interval
  std::erase_if (midi_sequences_, [&] (const auto &entry) {
    const auto &[existing_interval, seq] = entry;
    const auto [existing_start, existing_end] = existing_interval;
    // Remove sequences that overlap with any part of the target interval
    return !(existing_end < target_start || existing_start > target_end);
  });
}

void
MidiTimelineDataCache::add_midi_sequence (
  IntervalType                     interval,
  const juce::MidiMessageSequence &sequence)
{
  const auto [start_time, end_time] = interval;

  // Validate that all events are within the interval
  for (const auto * event : sequence)
    {
      const double event_time = event->message.getTimeStamp ();
      if (
        event_time < start_time.in<double> (units::samples)
        || event_time > end_time.in<double> (units::samples))
        {
          throw std::invalid_argument (
            "MIDI sequence contains events outside the specified interval");
        }
    }

  // Ensure the sequence has no unended notes
  juce::MidiMessageSequence validated_sequence (sequence);
  validated_sequence.updateMatchedPairs ();

  // Check for unended notes and add note-off events at the interval end time
  for (const auto * event : validated_sequence)
    {
      if (event->message.isNoteOn () && event->noteOffObject == nullptr)
        {
          // Add a note-off event at the end of the interval
          validated_sequence.addEvent (
            juce::MidiMessage::noteOff (
              event->message.getChannel (), event->message.getNoteNumber ()),
            end_time.in<double> (units::samples));
        }
    }

  validated_sequence.updateMatchedPairs ();
  midi_sequences_[interval] = validated_sequence;
}

void
MidiTimelineDataCache::finalize_changes ()
{
  // Finalize MIDI events
  merged_midi_events_.clear ();
  for (const auto &[interval, seq] : midi_sequences_)
    {
      merged_midi_events_.addSequence (seq, 0);
      merged_midi_events_.updateMatchedPairs ();
    }
}

bool
MidiTimelineDataCache::has_content () const
{
  return merged_midi_events_.getNumEvents () > 0;
}

// ========== AudioTimelineDataCache Implementation ==========

void
AudioTimelineDataCache::clear ()
{
  audio_regions_.clear ();
}

void
AudioTimelineDataCache::remove_sequences_matching_interval (
  IntervalType interval)
{
  const auto [target_start, target_end] = interval;

  // Remove audio regions that overlap with the interval
  std::erase_if (audio_regions_, [&] (const AudioRegionEntry &entry) {
    // Remove regions that overlap with any part of the target interval
    return entry.end_sample >= target_start && entry.start_sample <= target_end;
  });
}

void
AudioTimelineDataCache::add_audio_region (
  IntervalType                   interval,
  const juce::AudioSampleBuffer &audio_buffer)
{
  const auto [start_sample, end_sample] = interval;

  AudioRegionEntry entry;
  // Create a copy of the audio buffer
  entry.audio_buffer = audio_buffer;
  entry.start_sample = start_sample;
  entry.end_sample = end_sample;

  audio_regions_.push_back (entry);
}

void
AudioTimelineDataCache::finalize_changes ()
{
  // Sort audio regions by start position for efficient processing
  std::ranges::sort (audio_regions_, {}, [] (const AudioRegionEntry &entry) {
    return entry.start_sample;
  });
}

bool
AudioTimelineDataCache::has_content () const
{
  return !audio_regions_.empty ();
}

// ========== AutomationTimelineDataCache Implementation ==========

void
AutomationTimelineDataCache::clear ()
{
  automation_sequences_.clear ();
}

void
AutomationTimelineDataCache::remove_sequences_matching_interval (
  IntervalType interval)
{
  const auto [target_start, target_end] = interval;

  // Remove automation sequences that overlap with the interval
  std::erase_if (automation_sequences_, [&] (const AutomationCacheEntry &entry) {
    // Remove sequences that overlap with any part of the target interval
    return entry.end_sample >= target_start && entry.start_sample <= target_end;
  });
}

void
AutomationTimelineDataCache::add_automation_sequence (
  IntervalType              interval,
  const std::vector<float> &automation_values)
{
  const auto [start_sample, end_sample] = interval;

  AutomationCacheEntry entry;
  entry.automation_values = automation_values;
  entry.start_sample = start_sample;
  entry.end_sample = end_sample;

  automation_sequences_.push_back (entry);
}

void
AutomationTimelineDataCache::finalize_changes ()
{
  // Sort automation sequences by start position for efficient processing
  std::ranges::sort (
    automation_sequences_, {},
    [] (const AutomationCacheEntry &entry) { return entry.start_sample; });
}

bool
AutomationTimelineDataCache::has_content () const
{
  return !automation_sequences_.empty ();
}

} // namespace zrythm::dsp
