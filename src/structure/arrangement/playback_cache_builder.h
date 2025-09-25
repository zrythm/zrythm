// SPDX-FileCopyrightText: © 2025 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/midi_playback_cache.h"
#include "structure/arrangement/arranger_object_owner.h"
#include "structure/arrangement/midi_region_serializer.h"
#include "utils/expandable_tick_range.h"

namespace zrythm::structure::arrangement
{

/**
 * @brief A track processor cache generator/updater task.
 */
class PlaybackCacheBuilder
{
public:
  using MidiRegionOwner =
    arrangement::ArrangerObjectOwner<arrangement::MidiRegion>;

  static void generate_midi_cache_for_midi_region_collections (
    dsp::MidiPlaybackCache               &cache_to_modify,
    RangeOf<const MidiRegionOwner *> auto midi_region_owners,
    const dsp::TempoMap                  &tempo_map,
    const utils::ExpandableTickRange     &affected_range)
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
        cache_to_modify.clear ();
      }
    else
      {
        cache_to_modify.remove_sequences_matching_interval (sample_interval);
      }

    const auto regions_inside_interval_filter_func =
      [&affected_range, sample_interval] (const auto &region) {
        if (affected_range.is_full_content ())
          return true;

        return region->bounds ()->is_hit_by_range (sample_interval);
      };

    const auto cache_region =
      [&cache_to_modify, &tempo_map] (const arrangement::MidiRegion * r) {
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
        cache_to_modify.add_sequence (
          std::make_pair (
            r->position ()->samples (),
            r->bounds ()->get_end_position_samples (true)),
          region_seq);
      };

    // Go through each MIDI region and add a cache
    for (const auto &midi_region_collection : midi_region_owners)
      {
        std::ranges::for_each (
          std::views::filter (
            midi_region_collection->MidiRegionOwner::get_children_view (),
            regions_inside_interval_filter_func),
          cache_region);
      }

    // Finalize
    cache_to_modify.finalize_changes ();
  }
};

}
