// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "dsp/tempo_warp_map.h"

namespace zrythm::dsp
{

TimeWarpMap
to_time_warp_map (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  const TempoMap                             &tempo_map,
  units::precise_tick_t                       region_start_tick,
  double                                      source_bpm,
  units::sample_t                             num_source_frames)
{
  if (source_bpm <= 0.0)
    throw std::invalid_argument ("source_bpm must be > 0");

  const auto sr = tempo_map.get_sample_rate ();
  const auto bpm = source_bpm;
  const auto ppq = static_cast<double> (TempoMap::get_ppq ());

  const auto start_samples =
    tempo_map.tick_to_samples_rounded (region_start_tick);

  const auto source_frame_for =
    [&] (units::precise_tick_t content_ticks) -> units::sample_t {
    return units::samples (
      static_cast<int64_t> (std::round (
        content_ticks.in (units::ticks) / ppq * (60.0 / bpm) * sr)));
  };

  const auto output_frame_for =
    [&] (units::precise_tick_t delta_ticks) -> units::sample_t {
    const auto tick = region_start_tick + delta_ticks;
    return tempo_map.tick_to_samples_rounded (tick) - start_samples;
  };

  std::vector<WarpAnchor> anchors;
  anchors.reserve (warp_points.size ());

  for (const auto &wp : warp_points)
    {
      anchors.push_back (
        { source_frame_for (wp.content_ticks),
          output_frame_for (wp.timeline_delta_ticks) });
    }

  // Sort + drop non-strictly-increasing anchors (rounding near boundaries).
  // The terminal anchor (last after sorting = max source_frame) is always kept:
  // if its output_frame tied a predecessor due to sub-sample rounding, dropping
  // it would break the source_length -> output_length invariant.
  std::ranges::sort (anchors, {}, &WarpAnchor::source_frame);
  std::vector<WarpAnchor> cleaned;
  cleaned.reserve (anchors.size ());
  for (size_t i = 0; i < anchors.size (); ++i)
    {
      const auto &a = anchors[i];
      const bool  is_terminal = (i + 1 == anchors.size ());
      if (!cleaned.empty ())
        {
          const auto &last = cleaned.back ();
          if (a.source_frame <= last.source_frame)
            continue;
          if (!is_terminal && a.output_frame <= last.output_frame)
            continue;
        }
      cleaned.push_back (a);
    }

  const auto total_output =
    cleaned.empty () ? units::samples (int64_t{ 0 }) : cleaned.back ().output_frame;

  TimeWarpMap map;
  map.anchors = std::move (cleaned);
  map.source_length = num_source_frames;
  map.output_length = total_output;
  return map;
}

} // namespace zrythm::dsp
