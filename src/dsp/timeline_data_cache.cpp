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
  IntervalType                          interval,
  std::span<const SampleBasedMidiEvent> events)
{
  const auto [start_time, end_time] = interval;

  validate_interval (interval);

  for (const auto &event : events)
    {
      if (event.time_ < start_time || event.time_ > end_time)
        {
          throw std::invalid_argument (
            "MIDI sequence contains events outside the specified interval");
        }
    }

  auto validated =
    events | std::ranges::to<std::vector<SampleBasedMidiEvent>> ();

  // Single reverse pass: accumulate note-offs in a multiset, consume one per
  // matching note-on. Unmatched note-ons get an auto-generated note-off at
  // interval end. Collects missing note-offs first to avoid
  // mutation-during-iteration.
  using NoteKey = std::pair<int, int>;
  std::multiset<NoteKey>                           unmatched_note_offs;
  std::vector<std::pair<NoteKey, units::sample_t>> missing_note_offs;

  for (const auto &ev : validated | std::views::reverse)
    {
      const auto d = ev.data ();
      if (
        !utils::midi::midi_is_note_on (d) && !utils::midi::midi_is_note_off (d))
        continue;

      const NoteKey key{ d[0] & 0x0F, d[1] };
      if (utils::midi::midi_is_note_off (d))
        {
          unmatched_note_offs.insert (key);
        }
      else if (
        auto it = unmatched_note_offs.find (key);
        it != unmatched_note_offs.end ())
        {
          unmatched_note_offs.erase (it);
        }
      else
        {
          missing_note_offs.emplace_back (key, end_time);
        }
    }

  for (const auto &[key, time] : missing_note_offs)
    {
      validated.push_back (
        midi_event::make_note_off (key.first, key.second, time));
    }

  z_trace (
    "{} events, interval=[{}, {}]", validated.size (), start_time, end_time);
  midi_sequences_[interval] = std::move (validated);
}

void
MidiTimelineDataCache::finalize_changes_impl ()
{
  merged_midi_events_.clear ();

  // Concatenate all events from all intervals into one vector
  for (const auto &[interval, seq] : midi_sequences_)
    {
      merged_midi_events_.insert (
        merged_midi_events_.end (), seq.begin (), seq.end ());
    }

  // Sort once: by timestamp, then noteOff before noteOn at same timestamp
  std::ranges::stable_sort (
    merged_midi_events_,
    [] (const SampleBasedMidiEvent &a, const SampleBasedMidiEvent &b) {
      if (a.time_ != b.time_)
        return a.time_ < b.time_;
      const auto ad = a.data ();
      const auto bd = b.data ();
      return !utils::midi::midi_is_note_on (ad)
             && utils::midi::midi_is_note_on (bd);
    });
}

bool
MidiTimelineDataCache::has_content () const
{
  return !merged_midi_events_.empty ();
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
  audio_clips_.clear ();
}

void
AudioTimelineDataCache::remove_sequences_matching_interval (
  IntervalType interval)
{
  validate_interval (interval);

  // Strict overlap: adjacent intervals are not considered overlapping.
  std::erase_if (audio_clips_, [&] (const AudioClipEntry &entry) {
    return intervals_overlap (
      { entry.start_sample, entry.end_sample }, interval);
  });
}

void
AudioTimelineDataCache::add_audio_clip (
  IntervalType                   interval,
  const juce::AudioSampleBuffer &audio_buffer)
{
  const auto [start_sample, end_sample] = interval;

  validate_interval (interval);

  AudioClipEntry entry;
  // Create a copy of the audio buffer
  entry.audio_buffer = audio_buffer;
  entry.start_sample = start_sample;
  entry.end_sample = end_sample;

  audio_clips_.push_back (entry);
}

void
AudioTimelineDataCache::finalize_changes_impl ()
{
  // Sort audio clips by start position for efficient processing
  std::ranges::sort (audio_clips_, {}, [] (const AudioClipEntry &entry) {
    return entry.start_sample;
  });
}

bool
AudioTimelineDataCache::has_content () const
{
  return !audio_clips_.empty ();
}

std::vector<AudioTimelineDataCache::IntervalType>
AudioTimelineDataCache::compute_cached_sample_ranges () const
{
  return audio_clips_ | std::views::transform ([] (const AudioClipEntry &e) {
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
