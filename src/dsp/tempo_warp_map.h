// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "dsp/tempo_map.h"
#include "dsp/time_warp_map.h"
#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief Produces a TimeWarpMap for musical-mode auto-timestretch.
 *
 * Derives the source→output sample mapping from the tempo map and the clip's
 * permanent source BPM. The clip's musical content is assumed to be at a
 * constant @p source_bpm. Tempo changes within the region's span — including
 * linear ramps — are followed exactly: an anchor is placed at every
 * tempo-event boundary, and additional anchors are densely placed inside
 * linear-curve segments so the mapping tracks the ramp to sub-sample accuracy
 * between anchors.
 *
 * @param tempo_map Project tempo map.
 * @param region_start_tick Absolute tick position of the region start.
 * @param source_bpm The clip's permanent source BPM (must be > 0).
 * @param num_source_frames Number of source frames in the clip.
 * @return A validated TimeWarpMap.
 * @throws std::invalid_argument if @p source_bpm <= 0.
 */
[[nodiscard]] TimeWarpMap
compute_tempo_warp_map (
  const TempoMap       &tempo_map,
  units::precise_tick_t region_start_tick,
  float                 source_bpm,
  units::sample_t       num_source_frames);

} // namespace zrythm::dsp
