// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <span>

#include "dsp/content_time_warp.h"
#include "dsp/tempo_map.h"
#include "dsp/tick_types.h"
#include "dsp/time_warp_map.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Convert ContentTimeWarp's tick-space warp points to a sample-space
 * TimeWarpMap for the timestretch engine.
 *
 * Pure mechanical conversion — two linear scalings per warp point:
 *   source_frame = content_ticks / ppq * (60 / source_bpm) * sr
 *   output_frame = tick_to_samples(clip_start + delta_ticks) -
 * clip_start_samples
 *
 * No tempo-event enumeration or curve-type logic — all curve information is
 * already baked into the warp points (including dense sampling for Linear
 * ramps). This is why drift between the two paths is structurally impossible.
 *
 * @throws std::invalid_argument if @p source_bpm <= 0.
 */
[[nodiscard]] TimeWarpMap
to_time_warp_map (
  std::span<const ContentTimeWarp::WarpPoint> warp_points,
  const TempoMap                             &tempo_map,
  TimelineTick                                clip_start_tick,
  units::bpm_t                                source_bpm,
  units::sample_t                             num_source_frames);

} // namespace zrythm::dsp
