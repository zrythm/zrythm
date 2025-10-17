// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/types.h"
#include "utils/units.h"

#include "juce_wrapper.h"

namespace zrythm::dsp
{

/**
 * @brief Unified playback cache that handles both MIDI and audio events.
 *
 * This cache provides a single interface for managing timeline events of
 * different types (MIDI and audio) with consistent behavior for thread-safe
 * access and range-based updates.
 */
class TimelineDataCache
{
public:
  using IntervalType = std::pair<units::sample_t, units::sample_t>;

  /**
   * @brief Audio region entry for caching.
   *
   * Contains all the information needed to process an audio region during
   * playback without accessing the region object directly in the real-time
   * thread.
   */
  struct AudioRegionEntry
  {
    /** Copy of the audio sample buffer. */
    juce::AudioSampleBuffer audio_buffer;

    /** Start position in samples. */
    units::sample_t start_sample;

    /** End position in samples. */
    units::sample_t end_sample;
  };

public:
  /**
   * @brief Clears all cached events (both MIDI and audio).
   */
  void clear ();

  /**
   * @brief Removes both MIDI sequences and audio regions matching the given
   * interval.
   *
   * This method removes ALL cached data (both MIDI and audio) that overlaps
   * with the specified time interval. Any MIDI sequence or audio region whose
   * interval overlaps with the given interval will be removed.
   *
   * @param interval The time interval to remove (in samples).
   */
  void remove_sequences_matching_interval (IntervalType interval);

  /**
   * @brief Adds a MIDI sequence for the given interval.
   *
   * @param interval The time interval (in samples).
   * @param sequence The MIDI message sequence.
   */
  void add_midi_sequence (
    IntervalType                     interval,
    const juce::MidiMessageSequence &sequence);

  /**
   * @brief Adds an audio region for the given interval.
   *
   * @param interval The time interval (in samples).
   * @param audio_buffer The audio sample buffer to copy.
   */
  void add_audio_region (
    IntervalType                   interval,
    const juce::AudioSampleBuffer &audio_buffer);

  /**
   * @brief Finalizes changes and prepares cached data for access.
   *
   * This should be called after all modifications are complete to prepare the
   * cached data for real-time access.
   */
  void finalize_changes ();

  /**
   * @brief Gets the cached MIDI events.
   *
   * @return Reference to the merged MIDI message sequence.
   */
  const juce::MidiMessageSequence &get_midi_events () const
  {
    return merged_midi_events_;
  }

  /**
   * @brief Gets the cached audio regions.
   *
   * @return Reference to the vector of audio region entries.
   */
  const std::vector<AudioRegionEntry> &get_audio_regions () const
  {
    return audio_regions_;
  }

  /**
   * @brief Checks if the cache has any MIDI events.
   */
  bool has_midi_events () const
  {
    return merged_midi_events_.getNumEvents () > 0;
  }

  /**
   * @brief Checks if the cache has any audio regions.
   */
  bool has_audio_regions () const { return !audio_regions_.empty (); }

private:
  /**
   * @brief MIDI sequences organized by time interval.
   *
   * The key is the time interval (start/end samples) and the value is the
   * MIDI message sequence for that interval.
   */
  std::map<IntervalType, juce::MidiMessageSequence> midi_sequences_;

  /**
   * @brief Audio region entries for playback.
   *
   * These are stored in a vector and sorted by start position during
   * finalization for efficient processing.
   */
  std::vector<AudioRegionEntry> audio_regions_;

  /**
   * @brief Merged MIDI events ready for real-time access.
   *
   * This is populated during finalize_changes() and contains all MIDI events
   * from all sequences, properly merged and with matched pairs.
   */
  juce::MidiMessageSequence merged_midi_events_;
};

} // namespace zrythm::dsp
