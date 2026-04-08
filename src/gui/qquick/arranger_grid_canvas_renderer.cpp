// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "dsp/tempo_map_qml_adapter.h"
#include "gui/qquick/arranger_grid_canvas_item.h"
#include "gui/qquick/arranger_grid_canvas_renderer.h"

namespace zrythm::gui::qquick
{

void
ArrangerGridCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * grid_item = static_cast<ArrangerGridCanvasItem *> (item);

  line_color_ = grid_item->lineColor ();
  scroll_x_ = grid_item->scrollX ();
  px_per_tick_ = grid_item->pxPerTick ();
  bar_line_opacity_ = grid_item->barLineOpacity ();
  beat_line_opacity_ = grid_item->beatLineOpacity ();
  sixteenth_line_opacity_ = grid_item->sixteenthLineOpacity ();
  detail_measure_px_threshold_ = grid_item->detailMeasurePxThreshold ();
  canvas_height_ = static_cast<float> (grid_item->height ());

  bar_lines_.clear ();
  beat_lines_.clear ();
  sixteenth_lines_.clear ();

  auto * tempo_map = grid_item->tempoMap ();
  if (tempo_map == nullptr || px_per_tick_ <= 0.0)
    return;

  // Ticks per sixteenth note (constant regardless of time signature)
  const qreal ticks_per_sixteenth = tempo_map->getPpq () / 4.0;
  const qreal px_per_sixteenth = ticks_per_sixteenth * px_per_tick_;
  const bool  show_sixteenths = px_per_sixteenth > detail_measure_px_threshold_;

  // Visibility bounds in ticks
  const qreal visible_start_tick = scroll_x_ / px_per_tick_;
  const qreal visible_end_tick = grid_item->scrollXPlusWidth () / px_per_tick_;

  const int start_bar = std::max (
    1,
    tempo_map->getMusicalPositionBar (static_cast<int64_t> (visible_start_tick)));
  const int end_bar =
    tempo_map->getMusicalPositionBar (static_cast<int64_t> (visible_end_tick))
    + 1;

  for (const auto bar : std::views::iota (start_bar, end_bar + 1))
    {
      const int64_t bar_tick =
        tempo_map->getTickFromMusicalPosition (bar, 1, 1, 0);
      const qreal bar_x = bar_tick * px_per_tick_;

      bar_lines_.push_back (bar_x);

      // Per-bar time signature for correct beat sizing
      const int numerator = tempo_map->timeSignatureNumeratorAtTick (bar_tick);
      const int denominator =
        tempo_map->timeSignatureDenominatorAtTick (bar_tick);
      const int   sixteenths_per_beat = 16 / denominator;
      const qreal px_per_beat = px_per_sixteenth * sixteenths_per_beat;

      if (px_per_beat <= detail_measure_px_threshold_)
        continue;

      for (const auto beat : std::views::iota (1, numerator + 1))
        {
          const int64_t beat_tick =
            tempo_map->getTickFromMusicalPosition (bar, beat, 1, 0);
          const qreal beat_x = beat_tick * px_per_tick_;

          // Beat 1 is the bar line — skip the beat line but still process
          // sixteenths below.
          if (beat > 1)
            beat_lines_.push_back (beat_x);

          if (!show_sixteenths)
            continue;

          for (
            const auto sixteenth : std::views::iota (2, sixteenths_per_beat + 1))
            {
              const int64_t sixteenth_tick =
                tempo_map->getTickFromMusicalPosition (bar, beat, sixteenth, 0);
              sixteenth_lines_.push_back (sixteenth_tick * px_per_tick_);
            }
        }
    }
}

void
ArrangerGridCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (canvas_height_ <= 0.0f)
    return;

  painter->setRenderHint (QCanvasPainter::RenderHint::Antialiasing, false);
  painter->setLineWidth (1.0f);

  // The canvas is viewport-sized. Translate so content-space coordinates
  // map to the visible portion.
  painter->save ();
  painter->translate (static_cast<float> (-scroll_x_), 0.0f);

  const float h = canvas_height_;

  // Draw a batch of vertical lines with the same color
  auto draw_lines =
    [painter, h] (
      const std::vector<qreal> &x_positions, const QColor &base_color,
      qreal opacity) {
      if (x_positions.empty ())
        return;
      QColor c = base_color;
      c.setAlphaF (opacity);
      painter->setStrokeStyle (c);
      painter->beginPath ();
      for (const auto x : x_positions)
        {
          const float fx = static_cast<float> (x);
          painter->moveTo (fx, 0.0f);
          painter->lineTo (fx, h);
        }
      painter->stroke ();
    };

  // Draw back to front: sixteenth -> beat -> bar
  draw_lines (sixteenth_lines_, line_color_, sixteenth_line_opacity_);
  draw_lines (beat_lines_, line_color_, beat_line_opacity_);
  draw_lines (bar_lines_, line_color_, bar_line_opacity_);

  painter->restore ();
}

} // namespace zrythm::gui::qquick
