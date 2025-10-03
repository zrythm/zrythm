// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_playback_cache.h"
#include "structure/arrangement/midi_region_serializer.h"
#include "utils/expandable_tick_range.h"
#include "utils/types.h"

#include <farbot/RealtimeObject.hpp>

namespace juce
{
class MidiMessageSequence;
}

namespace zrythm::structure::arrangement
{

/**
 * @brief Event provider for timeline-based MIDI events.
 *
 * This provider extracts MIDI events from the timeline arrangement,
 * using the existing playback cache system.
 */
class TimelineMidiEventProvider final
{
public:
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
  void generate_events (
    const dsp::TempoMap                          &tempo_map,
    RangeOf<const arrangement::MidiRegion *> auto midi_regions,
    utils::ExpandableTickRange                    affected_range)
  {
    const auto sample_interval = [&affected_range, &tempo_map] () {
      if (!affected_range.is_full_content ())
        {
          const auto tick_range = affected_range.range ().value ();
          return std::make_pair (
            tempo_map.tick_to_samples_rounded (units::ticks (tick_range.first))
              .in (units::samples),
            tempo_map.tick_to_samples_rounded (units::ticks (tick_range.second))
              .in (units::samples));
        }
      return std::make_pair (
        static_cast<int64_t> (0), std::numeric_limits<int64_t>::max ());
    }();

    // Remove existing caches at given interval (or all caches if no interval
    // given)
    if (affected_range.is_full_content ())
      {
        midi_playback_cache_.clear ();
      }
    else
      {
        midi_playback_cache_.remove_sequences_matching_interval (
          sample_interval);
      }

    const auto regions_inside_interval_filter_func =
      [&affected_range, sample_interval] (const auto &region) {
        if (affected_range.is_full_content ())
          return true;

        return region->bounds ()->is_hit_by_range (sample_interval);
      };

    const auto cache_region =
      [this, &tempo_map] (const arrangement::MidiRegion * r) {
        juce::MidiMessageSequence region_seq;

        // Serialize region (timings in ticks)
        arrangement::MidiRegionSerializer::serialize_to_sequence (
          *r, region_seq, std::nullopt, std::nullopt, true, true);

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
        midi_playback_cache_.add_sequence (
          std::make_pair (
            r->position ()->samples (),
            r->bounds ()->get_end_position_samples (true)),
          region_seq);
      };

    // Go through each MIDI region and add a cache
    std::ranges::for_each (
      std::views::filter (midi_regions, regions_inside_interval_filter_func),
      cache_region);

    // Finalize
    midi_playback_cache_.finalize_changes ();
    set_midi_events (midi_playback_cache_.cached_events ());
  }

  // TrackEventProvider interface
  void process_events (
    const EngineProcessTimeInfo &time_nfo,
    dsp::MidiEventVector        &output_buffer);

private:
  /**
   * @brief Set the MIDI events for realtime access.
   *
   * @param events The MIDI events sequence.
   */
  void set_midi_events (const juce::MidiMessageSequence &events);

private:
  /** Active MIDI playback sequence for realtime access. */
  farbot::RealtimeObject<
    juce::MidiMessageSequence,
    farbot::RealtimeObjectOptions::nonRealtimeMutatable>
    active_midi_playback_sequence_;

  dsp::MidiPlaybackCache midi_playback_cache_;
};

} // namespace zrythm::structure::tracks
