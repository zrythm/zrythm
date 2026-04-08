// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <optional>
#include <vector>

namespace zrythm::dsp
{
class TempoMapWrapper;
}

namespace zrythm::gui::qquick
{

/**
 * @brief A single grid line with its pixel position and musical coordinates.
 */
struct GridLine
{
  float x{ 0.0f };      ///< Pixel position in content space
  int   bar{ 0 };       ///< 1-indexed bar number
  int   beat{ 0 };      ///< 1-indexed beat number
  int   sixteenth{ 0 }; ///< 1-indexed sixteenth number
};

/**
 * @brief Pre-computed sets of grid lines for a visible region.
 */
struct ComputedGridLines
{
  std::vector<GridLine> bar_lines;
  std::vector<GridLine> beat_lines;
  std::vector<GridLine> sixteenth_lines;
  // Beat lines whose labels are visible (empty if label threshold was not
  // requested).
  std::vector<GridLine> beat_label_lines;
  // True when sixteenth lines are wide enough for labels (only set when label
  // threshold was requested)
  bool show_sixteenth_labels{ false };

  void clear ()
  {
    bar_lines.clear ();
    beat_lines.clear ();
    sixteenth_lines.clear ();
    beat_label_lines.clear ();
    show_sixteenth_labels = false;
  }
};

/**
 * @brief Computes grid line positions for a visible tick range.
 *
 * Reuses the existing vectors in @p result (clears and refills) to avoid
 * repeated heap allocations across calls.
 *
 * Uses the tempo map to determine time signatures per bar and calculates
 * correct beat/sixteenth subdivisions. Lines outside the visible range
 * are culled. Beat/sixteenth lines that would be too dense are omitted
 * based on @p detail_measure_px_threshold.
 *
 * @param tempo_map  Tempo map for tick/musical-position conversion.
 * @param px_per_tick  Current zoom level in pixels per tick.
 * @param visible_start_tick  Start of the visible range in ticks.
 * @param visible_end_tick  End of the visible range in ticks.
 * @param detail_measure_px_threshold  Minimum pixels per subdivision to show it.
 * @param detail_measure_label_px_threshold  When set, beat lines whose labels
 * are wide enough are also added to result.beat_label_lines.
 * @param result  Output grid lines to reuse.
 */
void
compute_grid_lines (
  const dsp::TempoMapWrapper &tempo_map,
  float                       px_per_tick,
  float                       visible_start_tick,
  float                       visible_end_tick,
  float                       detail_measure_px_threshold,
  std::optional<float>        detail_measure_label_px_threshold,
  ComputedGridLines          &result);

} // namespace zrythm::gui::qquick
