// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "utils/units.h"

#include <QObject>

#include "juce_wrapper.h"

namespace zrythm::dsp
{

/**
 * @brief Base class for timeline data caches.
 *
 * Provides common functionality for all timeline data cache types.
 * This is an abstract base class that defines the interface that
 * all derived cache classes must implement.
 */
class TimelineDataCache : public QObject
{
  Q_OBJECT

public:
  using IntervalType = std::pair<units::sample_t, units::sample_t>;

  explicit TimelineDataCache (QObject * parent = nullptr) : QObject (parent) { }

  ~TimelineDataCache () override;

  /**
   * @brief Clears all cached data and emits cachedRangesChanged.
   */
  void clear ()
  {
    clear_impl ();
    Q_EMIT cachedRangesChanged (compute_cached_sample_ranges ());
  }

  /**
   * @brief Removes cached data matching the given interval.
   *
   * @param interval The time interval to remove (in samples).
   *
   * Call finalize_changes() after one or more calls to this method.
   */
  virtual void remove_sequences_matching_interval (IntervalType interval) = 0;

  /**
   * @brief Finalizes changes, prepares cached data for access, and emits
   * cachedRangesChanged.
   *
   * This should be called after all modifications are complete to prepare the
   * cached data for real-time access.
   */
  void finalize_changes ()
  {
    finalize_changes_impl ();
    Q_EMIT cachedRangesChanged (compute_cached_sample_ranges ());
  }

  /**
   * @brief Checks if the cache has any content.
   *
   * @return True if the cache contains any data, false otherwise.
   */
  virtual bool has_content () const = 0;

  /**
   * @brief Emitted when the cache content changes (after finalize_changes or
   * clear).
   *
   * Carries the current list of sample intervals that are cached.
   * Used for debug visualization of cache coverage.
   */
  Q_SIGNAL void cachedRangesChanged (std::vector<IntervalType> ranges) const;

private:
  virtual void clear_impl () = 0;
  virtual void finalize_changes_impl () = 0;

  /**
   * @brief Computes the sample ranges currently cached.
   */
  virtual std::vector<IntervalType> compute_cached_sample_ranges () const = 0;
};

/**
 * @brief MIDI-specific timeline data cache.
 *
 * Handles caching of MIDI sequences with thread-safe access and
 * range-based updates.
 */
class MidiTimelineDataCache : public TimelineDataCache
{
public:
  explicit MidiTimelineDataCache (QObject * parent = nullptr)
      : TimelineDataCache (parent)
  {
  }

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
   * @brief Gets the cached MIDI events.
   *
   * @return Reference to the merged MIDI message sequence.
   */
  const juce::MidiMessageSequence &get_midi_events () const
  {
    return merged_midi_events_;
  }

  void remove_sequences_matching_interval (IntervalType interval) override;
  bool has_content () const override;

private:
  void                      clear_impl () override;
  void                      finalize_changes_impl () override;
  std::vector<IntervalType> compute_cached_sample_ranges () const override;
  /**
   * @brief MIDI sequences organized by time interval.
   *
   * The key is the time interval (start/end samples) and the value is the
   * MIDI message sequence for that interval.
   */
  std::map<IntervalType, juce::MidiMessageSequence> midi_sequences_;

  /**
   * @brief Merged MIDI events ready for real-time access.
   *
   * This is populated during finalize_changes() and contains all MIDI events
   * from all sequences, properly merged and with matched pairs.
   */
  juce::MidiMessageSequence merged_midi_events_;
};

/**
 * @brief Audio-specific timeline data cache.
 *
 * Handles caching of audio regions with thread-safe access and
 * range-based updates.
 */
class AudioTimelineDataCache : public TimelineDataCache
{
public:
  explicit AudioTimelineDataCache (QObject * parent = nullptr)
      : TimelineDataCache (parent)
  {
  }

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
   * @brief Gets the cached audio regions.
   *
   * @return Reference to the vector of audio region entries.
   */
  const std::vector<AudioRegionEntry> &get_audio_regions () const
  {
    return audio_regions_;
  }

  void remove_sequences_matching_interval (IntervalType interval) override;
  bool has_content () const override;

private:
  void                      clear_impl () override;
  void                      finalize_changes_impl () override;
  std::vector<IntervalType> compute_cached_sample_ranges () const override;
  /**
   * @brief Audio region entries for playback.
   *
   * These are stored in a vector and sorted by start position during
   * finalization for efficient processing.
   */
  std::vector<AudioRegionEntry> audio_regions_;
};

/**
 * @brief Automation-specific timeline data cache.
 *
 * Handles caching of automation sequences with thread-safe access and
 * range-based updates.
 */
class AutomationTimelineDataCache : public TimelineDataCache
{
public:
  explicit AutomationTimelineDataCache (QObject * parent = nullptr)
      : TimelineDataCache (parent)
  {
  }

  /**
   * @brief Automation cache entry for caching.
   *
   * Contains all the information needed to process automation data during
   * playback without accessing the automation objects directly in the real-time
   * thread.
   */
  struct AutomationCacheEntry
  {
    /** Automation values for the parameter. */
    std::vector<float> automation_values;

    /** Start position in samples. */
    units::sample_t start_sample;

    /** End position in samples. */
    units::sample_t end_sample;
  };

  /**
   * @brief Adds an automation sequence for the given interval.
   *
   * @param interval The time interval (in samples).
   * @param automation_values The automation values.
   */
  void add_automation_sequence (
    IntervalType              interval,
    const std::vector<float> &automation_values);

  /**
   * @brief Gets the cached automation sequences.
   *
   * @return Reference to the vector of automation cache entries.
   */
  const std::vector<AutomationCacheEntry> &get_automation_sequences () const
  {
    return automation_sequences_;
  }

  void remove_sequences_matching_interval (IntervalType interval) override;
  bool has_content () const override;

private:
  void                      clear_impl () override;
  void                      finalize_changes_impl () override;
  std::vector<IntervalType> compute_cached_sample_ranges () const override;
  /**
   * @brief Automation sequences for playback.
   *
   * These are stored in a vector and sorted by start position during
   * finalization for efficient processing.
   */
  std::vector<AutomationCacheEntry> automation_sequences_;
};

} // namespace zrythm::dsp

Q_DECLARE_METATYPE (std::vector<zrythm::dsp::TimelineDataCache::IntervalType>)
