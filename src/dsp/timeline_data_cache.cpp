// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <set>

#include "dsp/timeline_data_cache.h"
#include "utils/logger.h"

namespace zrythm::dsp
{

// ========== MidiTimelineDataCache Implementation ==========

void
MidiTimelineDataCache::clear_impl ()
{
  midi_sequences_.clear ();
  merged_midi_events_.clear ();
}

void
MidiTimelineDataCache::remove_sequences_matching_interval (IntervalType interval)
{
  validate_interval (interval);

  // Strict overlap: adjacent intervals are not considered overlapping.
  std::erase_if (midi_sequences_, [&] (const auto &entry) {
    return intervals_overlap (entry.first, interval);
  });
}

void
MidiTimelineDataCache::add_midi_sequence (
  IntervalType                     interval,
  const juce::MidiMessageSequence &sequence)
{
  const auto [start_time, end_time] = interval;

  validate_interval (interval);

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

  // Single reverse pass over original events: track unmatched note-offs in a
  // multiset. When we see a noteOn, consume a matching noteOff if available;
  // otherwise record it for auto-generation. This correctly handles overlapping
  // same-pitch note-ons (each gets its own note-off) and avoids
  // mutation-during-iteration by collecting missing note-offs first.
  using NoteKey = std::pair<int, int>;
  std::multiset<NoteKey>                  unmatched_note_offs;
  std::vector<std::pair<NoteKey, double>> missing_note_offs;

  const int original_count = validated_sequence.getNumEvents ();
  for (int i = original_count - 1; i >= 0; --i)
    {
      const auto &msg = validated_sequence.getEventPointer (i)->message;
      if (!msg.isNoteOn () && !msg.isNoteOff ())
        continue;

      const auto key = NoteKey{ msg.getChannel (), msg.getNoteNumber () };
      if (msg.isNoteOff ())
        {
          unmatched_note_offs.insert (key);
        }
      else
        {
          auto it = unmatched_note_offs.find (key);
          if (it != unmatched_note_offs.end ())
            {
              unmatched_note_offs.erase (it);
            }
          else
            {
              missing_note_offs.emplace_back (
                key, end_time.in<double> (units::samples));
            }
        }
    }

  for (const auto &[key, time] : missing_note_offs)
    {
      validated_sequence.addEvent (
        juce::MidiMessage::noteOff (key.first, key.second), time);
    }

  z_trace (
    "{} events, interval=[{}, {}]", validated_sequence.getNumEvents (),
    start_time, end_time);
  midi_sequences_[interval] = validated_sequence;
}

void
MidiTimelineDataCache::finalize_changes_impl ()
{
  merged_midi_events_.clear ();

  // Collect all messages from all intervals into one vector
  std::vector<juce::MidiMessage> all;
  all.reserve (
    std::transform_reduce (
      midi_sequences_.begin (), midi_sequences_.end (), 0, std::plus{},
      [] (const auto &p) { return p.second.getNumEvents (); }));
  for (const auto &[interval, seq] : midi_sequences_)
    {
      for (const auto * ev : seq)
        all.push_back (ev->message);
    }

  // Sort once: by timestamp, then noteOff before noteOn at same timestamp
  std::ranges::stable_sort (
    all, [] (const juce::MidiMessage &a, const juce::MidiMessage &b) {
      if (a.getTimeStamp () != b.getTimeStamp ())
        return a.getTimeStamp () < b.getTimeStamp ();
      return !a.isNoteOn () && b.isNoteOn ();
    });

  for (const auto &msg : all)
    merged_midi_events_.addEvent (msg);
}

bool
MidiTimelineDataCache::has_content () const
{
  return merged_midi_events_.getNumEvents () > 0;
}

std::vector<MidiTimelineDataCache::IntervalType>
MidiTimelineDataCache::compute_cached_sample_ranges () const
{
  return midi_sequences_ | std::views::keys
         | std::ranges::to<std::vector<IntervalType>> ();
}

// ========== AudioTimelineDataCache Implementation ==========

void
AudioTimelineDataCache::clear_impl ()
{
  audio_regions_.clear ();
}

void
AudioTimelineDataCache::remove_sequences_matching_interval (
  IntervalType interval)
{
  validate_interval (interval);

  // Strict overlap: adjacent intervals are not considered overlapping.
  std::erase_if (audio_regions_, [&] (const AudioRegionEntry &entry) {
    return intervals_overlap (
      { entry.start_sample, entry.end_sample }, interval);
  });
}

void
AudioTimelineDataCache::add_audio_region (
  IntervalType                   interval,
  const juce::AudioSampleBuffer &audio_buffer)
{
  const auto [start_sample, end_sample] = interval;

  validate_interval (interval);

  AudioRegionEntry entry;
  // Create a copy of the audio buffer
  entry.audio_buffer = audio_buffer;
  entry.start_sample = start_sample;
  entry.end_sample = end_sample;

  audio_regions_.push_back (entry);
}

void
AudioTimelineDataCache::finalize_changes_impl ()
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

std::vector<AudioTimelineDataCache::IntervalType>
AudioTimelineDataCache::compute_cached_sample_ranges () const
{
  return audio_regions_ | std::views::transform ([] (const AudioRegionEntry &e) {
           return std::make_pair (e.start_sample, e.end_sample);
         })
         | std::ranges::to<std::vector<IntervalType>> ();
}

// ========== AutomationTimelineDataCache Implementation ==========

void
AutomationTimelineDataCache::clear_impl ()
{
  automation_sequences_.clear ();
}

void
AutomationTimelineDataCache::remove_sequences_matching_interval (
  IntervalType interval)
{
  validate_interval (interval);

  // Strict overlap: adjacent intervals are not considered overlapping.
  std::erase_if (automation_sequences_, [&] (const AutomationCacheEntry &entry) {
    return intervals_overlap (
      { entry.start_sample, entry.end_sample }, interval);
  });
}

void
AutomationTimelineDataCache::add_automation_sequence (
  IntervalType              interval,
  const std::vector<float> &automation_values)
{
  const auto [start_sample, end_sample] = interval;

  validate_interval (interval);

  AutomationCacheEntry entry;
  entry.automation_values = automation_values;
  entry.start_sample = start_sample;
  entry.end_sample = end_sample;

  automation_sequences_.push_back (entry);
}

void
AutomationTimelineDataCache::finalize_changes_impl ()
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

std::vector<AutomationTimelineDataCache::IntervalType>
AutomationTimelineDataCache::compute_cached_sample_ranges () const
{
  return automation_sequences_
         | std::views::transform ([] (const AutomationCacheEntry &e) {
             return std::make_pair (e.start_sample, e.end_sample);
           })
         | std::ranges::to<std::vector<IntervalType>> ();
}

TimelineDataCache::~TimelineDataCache () = default;

} // namespace zrythm::dsp
