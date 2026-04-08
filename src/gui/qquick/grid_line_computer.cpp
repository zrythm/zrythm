// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "dsp/tempo_map_qml_adapter.h"
#include "gui/qquick/grid_line_computer.h"

namespace zrythm::gui::qquick
{

void
compute_grid_lines (
  const dsp::TempoMapWrapper &tempo_map,
  float                       px_per_tick,
  float                       visible_start_tick,
  float                       visible_end_tick,
  float                       detail_measure_px_threshold,
  std::optional<float>        detail_measure_label_px_threshold,
  ComputedGridLines          &result)
{
  result.clear ();

  constexpr float ticks_per_sixteenth =
    static_cast<float> (dsp::TempoMap::get_ppq ()) / 4.0f;
  const float px_per_sixteenth = ticks_per_sixteenth * px_per_tick;
  const bool  show_sixteenths = px_per_sixteenth > detail_measure_px_threshold;

  if (detail_measure_label_px_threshold.has_value ())
    result.show_sixteenth_labels =
      px_per_sixteenth > *detail_measure_label_px_threshold;

  const int start_bar = std::max (
    1,
    tempo_map.getMusicalPositionBar (static_cast<int64_t> (visible_start_tick)));
  const int end_bar =
    tempo_map.getMusicalPositionBar (static_cast<int64_t> (visible_end_tick))
    + 1;

  for (const auto bar : std::views::iota (start_bar, end_bar + 1))
    {
      const int64_t bar_tick =
        tempo_map.getTickFromMusicalPosition (bar, 1, 1, 0);
      const float bar_x = static_cast<float> (bar_tick) * px_per_tick;

      result.bar_lines.push_back (
        { .x = bar_x, .bar = bar, .beat = 1, .sixteenth = 1 });

      const int numerator = tempo_map.timeSignatureNumeratorAtTick (bar_tick);
      const int denominator =
        tempo_map.timeSignatureDenominatorAtTick (bar_tick);
      const int   sixteenths_per_beat = 16 / denominator;
      const float px_per_beat = px_per_sixteenth * sixteenths_per_beat;

      if (px_per_beat <= detail_measure_px_threshold)
        continue;

      for (const auto beat : std::views::iota (1, numerator + 1))
        {
          const int64_t beat_tick =
            tempo_map.getTickFromMusicalPosition (bar, beat, 1, 0);
          const float beat_x = static_cast<float> (beat_tick) * px_per_tick;

          // Beat 1 is the bar line — skip the beat line but still process
          // sixteenths below.
          if (beat > 1)
            {
              result.beat_lines.push_back (
                { .x = beat_x, .bar = bar, .beat = beat, .sixteenth = 1 });
              if (
                detail_measure_label_px_threshold.has_value ()
                && px_per_beat > *detail_measure_label_px_threshold)
                result.beat_label_lines.push_back (
                  { .x = beat_x, .bar = bar, .beat = beat, .sixteenth = 1 });
            }

          if (!show_sixteenths)
            continue;

          for (
            const auto sixteenth : std::views::iota (2, sixteenths_per_beat + 1))
            {
              const int64_t sixteenth_tick =
                tempo_map.getTickFromMusicalPosition (bar, beat, sixteenth, 0);
              const float sixteenth_x =
                static_cast<float> (sixteenth_tick) * px_per_tick;
              result.sixteenth_lines.push_back (
                { .x = sixteenth_x,
                  .bar = bar,
                  .beat = beat,
                  .sixteenth = sixteenth });
            }
        }
    }
}

} // namespace zrythm::gui::qquick
