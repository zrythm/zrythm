// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/itransport.h"
#include "dsp/timeline_data_cache.h"
#include "structure/arrangement/region_renderer.h"
#include "utils/expandable_tick_range.h"
#include "utils/types.h"

#include <farbot/RealtimeObject.hpp>

namespace zrythm::structure::arrangement
{

/**
 * @brief Base class for timeline data providers.
 *
 * Provides common functionality for all timeline data provider types.
 * This is an abstract base class that defines the interface that
 * all derived provider classes must implement.
 */
class TimelineDataProvider
{
public:
  using IntervalType = std::pair<units::sample_t, units::sample_t>;

  virtual ~TimelineDataProvider ();

  /**
   * @brief Generate the event sequence to be used during realtime processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @tparam RegionType Either MidiRegion, AudioRegion, ChordRegion, or
   * AutomationRegion
   * @param self The derived class instance (explicit this parameter)
   * @param tempo_map The tempo map for timing conversion.
   * @param regions The regions to process.
   * @param affected_range The range of ticks to process.
   */
  template <RegionObject RegionType>
  void generate_events (
    this auto                       &self,
    const dsp::TempoMap             &tempo_map,
    RangeOf<const RegionType *> auto regions,
    utils::ExpandableTickRange       affected_range)
  {
    // Convert tick range to sample range
    const auto sample_interval = [&affected_range, &tempo_map] ()
      -> std::pair<units::sample_t, units::sample_t> {
      if (!affected_range.is_full_content ())
        {
          const auto tick_range = affected_range.range ().value ();
          return std::make_pair (
            tempo_map.tick_to_samples_rounded (units::ticks (tick_range.first)),
            tempo_map.tick_to_samples_rounded (units::ticks (tick_range.second)));
        }
      return std::make_pair (
        units::samples (static_cast<int64_t> (0)),
        units::samples (std::numeric_limits<int64_t>::max ()));
    }();

    // Remove existing caches at given interval (or all caches if no interval
    // given)

    if (affected_range.is_full_content ())
      {
        self.get_base_cache ()->clear ();
      }
    else
      {
        self.get_base_cache ()->remove_sequences_matching_interval (
          sample_interval);
      }

    const auto regions_inside_interval_filter_func =
      [&affected_range, sample_interval] (const auto &region) {
        if (affected_range.is_full_content ())
          return true;

        return region->bounds ()->is_hit_by_range (sample_interval);
      };

    const auto cache_region = [&] (const auto * r) {
      // Skip muted regions
      if (r->mute ()->muted ())
        return;

      if constexpr (
        std::is_same_v<RegionType, arrangement::MidiRegion>
        || std::is_same_v<RegionType, arrangement::ChordRegion>)
        {
          self.cache_midi_region (*r, tempo_map);
        }
      else if constexpr (std::is_same_v<RegionType, arrangement::AudioRegion>)
        {
          self.cache_audio_region (*r);
        }
      else if constexpr (
        std::is_same_v<RegionType, arrangement::AutomationRegion>)
        {
          self.cache_automation_region (*r, tempo_map);
        }
    };

    // Go through each region and add a cache
    std::ranges::for_each (
      std::views::filter (regions, regions_inside_interval_filter_func),
      cache_region);

    // Finalize
    self.get_base_cache ()->finalize_changes ();
  }

  virtual void clear_all_caches () = 0;
  virtual void
  remove_sequences_matching_interval_from_all_caches (IntervalType interval) = 0;

protected:
  virtual dsp::TimelineDataCache * get_base_cache () = 0;

protected:
  /** Last transport state we've seen */
  dsp::ITransport::PlayState last_seen_transport_state_{
    dsp::ITransport::PlayState::Paused
  };

  /** Next expected transport position (for detecting jumps) */
  units::sample_t next_expected_transport_position_;
};

/**
 * @brief MIDI-specific timeline data provider.
 *
 * Handles caching of MIDI sequences with thread-safe access and
 * range-based updates.
 */
class MidiTimelineDataProvider : public TimelineDataProvider
{
  friend class TimelineDataProvider;

public:
  /**
   * Process MIDI events for the given time range.
   */
  void process_midi_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::ITransport::PlayState   transport_state,
    dsp::MidiEventVector        &output_buffer) noexcept [[clang::nonblocking]];

  // Implementation of base class methods
  void clear_all_caches () override;
  void remove_sequences_matching_interval_from_all_caches (
    IntervalType interval) override;

  const juce::MidiMessageSequence &get_midi_events () const;

  /**
   * @brief Generate the MIDI event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param midi_regions The MIDI regions to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_midi_events (
    const dsp::TempoMap                          &tempo_map,
    RangeOf<const arrangement::MidiRegion *> auto midi_regions,
    utils::ExpandableTickRange                    affected_range)
  {
    generate_events<arrangement::MidiRegion> (
      tempo_map, midi_regions, affected_range);
    set_midi_events (midi_cache_.get_midi_events ());
  }

  /**
   * @brief Generate the MIDI event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param chord_regions The Chord regions to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_midi_events (
    const dsp::TempoMap                           &tempo_map,
    RangeOf<const arrangement::ChordRegion *> auto chord_regions,
    utils::ExpandableTickRange                     affected_range)
  {
    generate_events<arrangement::ChordRegion> (
      tempo_map, chord_regions, affected_range);
    set_midi_events (midi_cache_.get_midi_events ());
  }

protected:
  dsp::TimelineDataCache * get_base_cache () override { return &midi_cache_; }

private:
  /**
   * Caches a MIDI-like region (MidiRegion or ChordRegion) to the MIDI cache.
   */
  template <typename MidiRegionType>
  void
  cache_midi_region (const MidiRegionType &region, const dsp::TempoMap &tempo_map)
  {
    // MIDI region processing
    juce::MidiMessageSequence region_seq;

    // Serialize region (timings in ticks)
    arrangement::RegionRenderer::serialize_to_sequence (
      region, region_seq, std::nullopt, std::nullopt, true, true);

    // Convert timings to samples
    for (auto &event : region_seq)
      {
        event->message.setTimeStamp (
          static_cast<double> (
            tempo_map
              .tick_to_samples_rounded (
                units::ticks (event->message.getTimeStamp ()))
              .in (units::samples)));
      }

    // Add to cache
    midi_cache_.add_midi_sequence (
      std::make_pair (
        units::samples (region.position ()->samples ()),
        region.bounds ()->get_end_position_samples (true)),
      region_seq);
  }

  /**
   * @brief Set the MIDI events for realtime access.
   *
   * @param events The MIDI events sequence.
   */
  void set_midi_events (const juce::MidiMessageSequence &events);

  dsp::MidiTimelineDataCache midi_cache_;

  farbot::RealtimeObject<
    juce::MidiMessageSequence,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_midi_playback_sequence_;
};

/**
 * @brief Audio-specific timeline data provider.
 *
 * Handles caching of audio regions with thread-safe access and
 * range-based updates.
 */
class AudioTimelineDataProvider : public TimelineDataProvider
{
  friend class TimelineDataProvider;

public:
  /**
   * Process audio events for the given time range.
   */
  void process_audio_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::ITransport::PlayState   transport_state,
    std::span<float>             output_left,
    std::span<float>             output_right) noexcept [[clang::nonblocking]];

  // Implementation of base class methods
  void clear_all_caches () override;
  void remove_sequences_matching_interval_from_all_caches (
    IntervalType interval) override;

  const std::vector<dsp::AudioTimelineDataCache::AudioRegionEntry> &
  get_audio_regions () const;

  /**
   * @brief Generate the audio event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param audio_regions The audio regions to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_audio_events (
    const dsp::TempoMap                           &tempo_map,
    RangeOf<const arrangement::AudioRegion *> auto audio_regions,
    utils::ExpandableTickRange                     affected_range)
  {
    generate_events<arrangement::AudioRegion> (
      tempo_map, audio_regions, affected_range);
    set_audio_regions (audio_cache_.get_audio_regions ());
  }

protected:
  dsp::TimelineDataCache * get_base_cache () override { return &audio_cache_; }

private:
  /**
   * Caches an AudioRegion to the audio cache.
   */
  void cache_audio_region (const arrangement::AudioRegion &region)
  {
    // Audio region processing
    auto audio_buffer = std::make_unique<juce::AudioSampleBuffer> ();

    // Serialize the audio region
    arrangement::RegionRenderer::serialize_to_buffer (region, *audio_buffer);

    // Add to cache with proper timing
    audio_cache_.add_audio_region (
      std::make_pair (
        units::samples (region.position ()->samples ()),
        region.bounds ()->get_end_position_samples (true)),
      *audio_buffer);
  }

  /**
   * @brief Set the audio regions for realtime access.
   *
   * @param regions The audio region entries.
   */
  void set_audio_regions (
    const std::vector<dsp::AudioTimelineDataCache::AudioRegionEntry> &regions);

  dsp::AudioTimelineDataCache audio_cache_;

  farbot::RealtimeObject<
    std::vector<dsp::AudioTimelineDataCache::AudioRegionEntry>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_audio_regions_;
};

/**
 * @brief Automation-specific timeline data provider.
 *
 * Handles caching of automation sequences with thread-safe access and
 * range-based updates.
 */
class AutomationTimelineDataProvider : public TimelineDataProvider
{
  friend class TimelineDataProvider;

public:
  /**
   * Process automation events for the given time range.
   */
  void process_automation_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::ITransport::PlayState   transport_state,
    std::span<float>             output_values) noexcept [[clang::nonblocking]];

  /**
   * Get the automation value for a specific sample position.
   *
   * @param sample_position The sample position to get the value for
   * @return The automation value at the given position, if any
   */
  std::optional<float>
  get_automation_value_rt (units::sample_t sample_position) noexcept
    [[clang::nonblocking]];

  // Implementation of base class methods
  void clear_all_caches () override;
  void remove_sequences_matching_interval_from_all_caches (
    IntervalType interval) override;

  const std::vector<dsp::AutomationTimelineDataCache::AutomationCacheEntry> &
  get_automation_sequences () const;

  /**
   * @brief Generate the automation event sequence to be used during realtime
   * processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @param tempo_map The tempo map for timing conversion.
   * @param automation_regions The automation regions to process.
   * @param affected_range The range of ticks to process.
   */
  void generate_automation_events (
    const dsp::TempoMap                                &tempo_map,
    RangeOf<const arrangement::AutomationRegion *> auto automation_regions,
    utils::ExpandableTickRange                          affected_range)
  {
    generate_events<arrangement::AutomationRegion> (
      tempo_map, automation_regions, affected_range);
    set_automation_sequences (automation_cache_.get_automation_sequences ());
  }

protected:
  dsp::TimelineDataCache * get_base_cache () override
  {
    return &automation_cache_;
  }

private:
  /**
   * Caches an AutomationRegion to the automation cache.
   */
  void cache_automation_region (
    const arrangement::AutomationRegion &region,
    const dsp::TempoMap                 &tempo_map)
  {
    // Calculate number of samples needed
    const auto start_sample = units::samples (region.position ()->samples ());
    const auto end_sample = region.bounds ()->get_end_position_samples (true);
    const auto num_samples = end_sample - start_sample;

    std::vector<float> automation_values (num_samples.in (units::samples));

    // Serialize automation region to sample-accurate values
    arrangement::RegionRenderer::serialize_to_automation_values (
      region, automation_values, std::nullopt, std::nullopt);

    automation_cache_.add_automation_sequence (
      std::make_pair (start_sample, end_sample), automation_values);
  }

  /**
   * @brief Set the automation sequences for realtime access.
   *
   * @param sequences The automation cache entries.
   */
  void set_automation_sequences (
    const std::vector<dsp::AutomationTimelineDataCache::AutomationCacheEntry>
      &sequences);

  dsp::AutomationTimelineDataCache automation_cache_;

  farbot::RealtimeObject<
    std::vector<dsp::AutomationTimelineDataCache::AutomationCacheEntry>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_automation_sequences_;
};

} // namespace zrythm::structure::arrangement
