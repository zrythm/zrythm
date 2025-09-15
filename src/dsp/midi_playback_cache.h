// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "juce_wrapper.h"

namespace zrythm::dsp
{
class MidiPlaybackCache
{
public:
  using IntervalType = std::pair<int64_t, int64_t>;

  void clear ()
  {
    event_sequences_.clear ();
    merged_events_.clear ();
  }

  void remove_sequences_matching_interval (IntervalType interval)
  {
    const auto [target_start, target_end] = interval;

    std::erase_if (event_sequences_, [&] (const auto &entry) {
      const auto &[existing_interval, seq] = entry;
      const auto [existing_start, existing_end] = existing_interval;
      // Remove sequences that overlap with any part of the target interval
      return !(existing_end < target_start || existing_start > target_end);
    });
  }

  void
  add_sequence (IntervalType interval, const juce::MidiMessageSequence &sequence)
  {
    const auto [start_time, end_time] = interval;

    // Validate that all events are within the interval
    for (const auto * event : sequence)
      {
        const double event_time = event->message.getTimeStamp ();
        if (
          event_time < static_cast<double> (start_time)
          || event_time > static_cast<double> (end_time))
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
              static_cast<double> (end_time));
          }
      }

    validated_sequence.updateMatchedPairs ();
    event_sequences_[interval] = validated_sequence;
  }

  void finalize_changes ()
  {
    merged_events_.clear ();
    for (const auto &[interval, seq] : event_sequences_)
      {
        merged_events_.addSequence (seq, 0);
        merged_events_.updateMatchedPairs ();
      }
  }

  const auto &cached_events () const { return merged_events_; }

private:
  /**
   * @brief A list of sequences (eg, corresponding to each MIDI region).
   */
  std::map<IntervalType, juce::MidiMessageSequence> event_sequences_;
  juce::MidiMessageSequence                         merged_events_;
};
}
