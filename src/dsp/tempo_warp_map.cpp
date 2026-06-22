// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>

#include "dsp/tempo_warp_map.h"

namespace zrythm::dsp
{

namespace
{
/**
 * @brief Cadence at which extra anchors are placed inside linear tempo ramps.
 *
 * Dense enough that linear interpolation between adjacent anchors tracks the
 * ramp to sub-sample accuracy.
 */
constexpr double k_linear_ramp_anchor_cadence_secs = 0.05;
} // namespace

TimeWarpMap
compute_tempo_warp_map (
  const TempoMap       &tempo_map,
  units::precise_tick_t region_start_tick,
  float                 source_bpm,
  units::sample_t       num_source_frames)
{
  if (source_bpm <= 0.f)
    throw std::invalid_argument ("source_bpm must be > 0");

  const double sr = tempo_map.get_sample_rate ();
  const double bpm = static_cast<double> (source_bpm);
  const double ppq = static_cast<double> (TempoMap::get_ppq ());
  const double nframes =
    static_cast<double> (num_source_frames.in (units::samples));

  // The clip's fixed musical length, in ticks.
  const auto musical_ticks = units::ticks ((nframes / sr) * (bpm / 60.0) * ppq);
  const auto region_start = region_start_tick;
  const auto region_end = region_start + musical_ticks;

  const auto start_samples = tempo_map.tick_to_samples_rounded (region_start);
  const auto end_samples = tempo_map.tick_to_samples_rounded (region_end);
  const auto total_output = end_samples - start_samples;

  // Source frame (double) → output frame, relative to region start.
  const auto output_for_source = [&] (double src_frame) -> units::sample_t {
    const auto tick =
      region_start + units::ticks ((src_frame / sr) * (bpm / 60.0) * ppq);
    return tempo_map.tick_to_samples_rounded (tick) - start_samples;
  };

  // Tick offset from region start (double) → source frame (double).
  const auto source_frame_for_offset = [&] (double offset_ticks) -> double {
    return (offset_ticks / ppq) * (60.0 / bpm) * sr;
  };

  const auto events = tempo_map.tempo_events ();

  // Curve type active at tick @p t: the curve of the last tempo event at or
  // before @p t (Constant when none precede it).
  const auto curve_at = [&] (units::precise_tick_t t) -> TempoMap::CurveType {
    const double t_ticks = t.in (units::ticks);
    auto         curve = TempoMap::CurveType::Constant;
    for (const auto &ev : events)
      {
        if (static_cast<double> (ev.tick.in (units::ticks)) <= t_ticks)
          curve = ev.curve;
        else
          break; // events are sorted by tick
      }
    return curve;
  };

  // Segment boundaries in tick space: region start, tempo events strictly
  // inside the region, region end.
  std::vector<units::precise_tick_t> boundaries;
  boundaries.push_back (region_start);
  for (const auto &ev : events)
    {
      const auto ev_tick =
        units::ticks (static_cast<double> (ev.tick.in (units::ticks)));
      if (ev_tick > region_start && ev_tick < region_end)
        boundaries.push_back (ev_tick);
    }
  boundaries.push_back (region_end);

  std::vector<WarpAnchor> anchors;

  // For each segment [b0, b1], add b0's anchor plus dense anchors if the
  // segment is a linear ramp. The final region_end anchor is appended after
  // the loop so shared boundaries aren't duplicated.
  for (size_t i = 0; i + 1 < boundaries.size (); ++i)
    {
      const auto b0 = boundaries[i];
      const auto b1 = boundaries[i + 1];
      const auto curve = curve_at (b0);

      const double sf0 =
        source_frame_for_offset ((b0 - region_start).in (units::ticks));
      const double sf1 =
        source_frame_for_offset ((b1 - region_start).in (units::ticks));

      anchors.push_back (
        { units::samples (static_cast<int64_t> (std::round (sf0))),
          output_for_source (sf0) });

      if (curve == TempoMap::CurveType::Linear)
        {
          const double stride =
            std::max (1.0, sr * k_linear_ramp_anchor_cadence_secs);
          for (double sf = sf0 + stride; sf < sf1 - 0.5; sf += stride)
            anchors.push_back (
              { units::samples (static_cast<int64_t> (std::round (sf))),
                output_for_source (sf) });
        }
    }

  // Final anchor at the region end.
  anchors.push_back ({ num_source_frames, total_output });

  // Construction already orders anchors, but rounding near boundaries can cause
  // ties, so normalize: sort by source frame and drop any anchor that is not
  // strictly increasing in both axes.
  std::ranges::sort (anchors, {}, &WarpAnchor::source_frame);
  std::vector<WarpAnchor> cleaned;
  cleaned.reserve (anchors.size ());
  for (const auto &a : anchors)
    {
      if (!cleaned.empty ())
        {
          const auto &last = cleaned.back ();
          if (a.source_frame <= last.source_frame)
            continue;
          if (a.output_frame <= last.output_frame)
            continue;
        }
      cleaned.push_back (a);
    }

  TimeWarpMap map;
  map.anchors = std::move (cleaned);
  map.source_length = num_source_frames;
  map.output_length = total_output;
  return map;
}

} // namespace zrythm::dsp
