// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include "utils/units.h"

namespace zrythm::dsp
{

/**
 * @brief One anchor in a source→output time-warp mapping.
 *
 * Both axes are sample frame indices. A time-stretch engine interpolates
 * between consecutive anchors to produce a continuously varying stretch
 * factor.
 */
struct WarpAnchor
{
  /** Frame index in the source (clip) audio. */
  units::sample_t source_frame;
  /** Frame index in the output (stretched) audio, relative to clip start. */
  units::sample_t output_frame;
};

/**
 * @brief A monotonic time-warp mapping from source samples to output samples.
 *
 * This is the single input consumed by every time-stretch engine, regardless
 * of how the mapping was produced (the tempo map for auto-timestretch, or in
 * future user-defined warp markers). Keeping the map in the sample domain
 * means engines never need to know about tempo maps or musical time.
 *
 * Anchors must be sorted ascending by @ref WarpAnchor::source_frame and
 * strictly increasing in @ref WarpAnchor::output_frame. The first anchor must
 * map `0 → 0` and the last must map `source_length → output_length`.
 *
 * @see to_time_warp_map()
 */
struct TimeWarpMap
{
  std::vector<WarpAnchor> anchors;

  /** Total source frames (length of the input clip). */
  units::sample_t source_length{};

  /** Total output frames (length of the stretched result). */
  units::sample_t output_length{};

  /**
   * @brief Validates the invariants of a well-formed warp map.
   *
   * Checks that anchors are non-empty, sorted ascending by source frame,
   * strictly increasing in output frame, and that the endpoints match
   * @ref source_length and @ref output_length (both of which must be
   * positive).
   *
   * @return true if the map is well-formed and safe to feed to an engine.
   */
  [[nodiscard]] bool is_valid () const noexcept;
};

/**
 * @brief Returns true if all anchors map source→output within @p tolerance
 * samples, i.e. the clip plays at approximately native speed.
 *
 * Used by the renderer to decide whether timestretching is needed.
 * A ±2 sample tolerance absorbs the rounding asymmetry between
 * @c to_time_warp_map's two conversion paths at non zero clip starts.
 *
 * @param tolerance Maximum per-anchor sample difference (default 2 samples).
 */
[[nodiscard]] bool
is_sample_space_identity (
  std::span<const WarpAnchor> anchors,
  units::sample_t             tolerance = units::samples (2));

} // namespace zrythm::dsp
