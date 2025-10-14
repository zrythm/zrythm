// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/itransport.h"
#include "dsp/timeline_data_cache.h"
#include "structure/arrangement/region_serializer.h"
#include "utils/expandable_tick_range.h"
#include "utils/types.h"

#include <farbot/RealtimeObject.hpp>

namespace zrythm::structure::arrangement
{

/**
 * @brief Unified data provider for timeline-based MIDI and audio events.
 *
 * This provider extracts both MIDI and audio events from the timeline
 * arrangement, using the unified playback cache system for thread-safe access.
 */
class TimelineDataProvider final
{
public:
  /**
   * @brief Generate the event sequence to be used during realtime processing.
   *
   * To be called as needed from the UI thread when a new cache is requested.
   *
   * @tparam RegionType Either MidiRegion or AudioRegion
   * @param tempo_map The tempo map for timing conversion.
   * @param regions The regions to process.
   * @param affected_range The range of ticks to process.
   */
  template <RegionObject RegionType>
  void generate_events (
    const dsp::TempoMap             &tempo_map,
    RangeOf<const RegionType *> auto regions,
    utils::ExpandableTickRange       affected_range)
    requires std::is_same_v<RegionType, arrangement::MidiRegion>
             || std::is_same_v<RegionType, arrangement::AudioRegion>
             || std::is_same_v<RegionType, arrangement::ChordRegion>
  {
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
        unified_playback_cache_.clear ();
      }
    else
      {
        unified_playback_cache_.remove_sequences_matching_interval (
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
          cache_midi_region (*r, tempo_map);
        }
      else if constexpr (std::is_same_v<RegionType, arrangement::AudioRegion>)
        {
          cache_audio_region (*r);
        }
    };

    // Go through each region and add a cache
    std::ranges::for_each (
      std::views::filter (regions, regions_inside_interval_filter_func),
      cache_region);

    // Finalize
    unified_playback_cache_.finalize_changes ();

    if constexpr (
      std::is_same_v<RegionType, arrangement::MidiRegion>
      || std::is_same_v<RegionType, arrangement::ChordRegion>)
      {
        set_midi_events (unified_playback_cache_.get_midi_events ());
      }
    else if constexpr (std::is_same_v<RegionType, arrangement::AudioRegion>)
      {
        set_audio_regions (unified_playback_cache_.get_audio_regions ());
      }
  }

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
  }

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
  }

  /**
   * @brief Process MIDI events for the given time range.
   *
   * @param time_nfo Time information for processing.
   * @param transport_state Current transport state.
   * @param output_buffer Output buffer for MIDI events.
   */
  void process_midi_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::ITransport::PlayState   transport_state,
    dsp::MidiEventVector        &output_buffer) noexcept [[clang::nonblocking]];

  /**
   * @brief Process audio events for the given time range.
   *
   * @param time_nfo Time information for processing.
   * @param transport_state Current transport state.
   * @param output_left Left channel output buffer.
   * @param output_right Right channel output buffer.
   */
  void process_audio_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::ITransport::PlayState   transport_state,
    std::span<float>             output_left,
    std::span<float>             output_right) noexcept [[clang::nonblocking]];

private:
  /**
   * Caches a MIDI-like region (MidiRegion or ChordRegion) to the unified cache.
   */
  template <typename MidiRegionType>
  void
  cache_midi_region (const MidiRegionType &region, const dsp::TempoMap &tempo_map)
  {
    // MIDI region processing
    juce::MidiMessageSequence region_seq;

    // Serialize region (timings in ticks)
    arrangement::RegionSerializer::serialize_to_sequence (
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
    unified_playback_cache_.add_midi_sequence (
      std::make_pair (
        units::samples (region.position ()->samples ()),
        region.bounds ()->get_end_position_samples (true)),
      region_seq);
  }

  /**
   * Caches an AudioRegion to the unified cache.
   */
  void cache_audio_region (const arrangement::AudioRegion &region)
  {
    // Audio region processing
    auto audio_buffer = std::make_unique<juce::AudioSampleBuffer> ();

    // Serialize the audio region
    arrangement::RegionSerializer::serialize_to_buffer (region, *audio_buffer);

    // Add to cache with proper timing
    unified_playback_cache_.add_audio_region (
      std::make_pair (
        units::samples (region.position ()->samples ()),
        region.bounds ()->get_end_position_samples (true)),
      *audio_buffer);
  }

  /**
   * @brief Set the MIDI events for realtime access.
   *
   * @param events The MIDI events sequence.
   */
  void set_midi_events (const juce::MidiMessageSequence &events);

  /**
   * @brief Set the audio regions for realtime access.
   *
   * @param regions The audio region entries.
   */
  void set_audio_regions (
    const std::vector<dsp::TimelineDataCache::AudioRegionEntry> &regions);

private:
  /** Active MIDI playback sequence for realtime access. */
  farbot::RealtimeObject<
    juce::MidiMessageSequence,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_midi_playback_sequence_;

  /** Active audio regions for realtime access. */
  farbot::RealtimeObject<
    std::vector<dsp::TimelineDataCache::AudioRegionEntry>,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_audio_regions_;

  dsp::TimelineDataCache unified_playback_cache_;

  /** Last transport state we've seen */
  dsp::ITransport::PlayState last_seen_transport_state_{
    dsp::ITransport::PlayState::Paused
  };

  /** Next expected transport position (for detecting jumps) */
  units::sample_t next_expected_transport_position_;
};

} // namespace zrythm::structure::arrangement
