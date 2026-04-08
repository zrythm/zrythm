// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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
  scroll_x_ = static_cast<float> (grid_item->scrollX ());
  px_per_tick_ = static_cast<float> (grid_item->pxPerTick ());
  bar_line_opacity_ = static_cast<float> (grid_item->barLineOpacity ());
  beat_line_opacity_ = static_cast<float> (grid_item->beatLineOpacity ());
  sixteenth_line_opacity_ =
    static_cast<float> (grid_item->sixteenthLineOpacity ());
  detail_measure_px_threshold_ =
    static_cast<float> (grid_item->detailMeasurePxThreshold ());
  canvas_height_ = static_cast<float> (grid_item->height ());

  auto * tempo_map = grid_item->tempoMap ();
  if (tempo_map == nullptr || px_per_tick_ <= 0.0)
    {
      grid_lines_.clear ();
      return;
    }

  const float visible_start_tick = scroll_x_ / px_per_tick_;
  const float visible_end_tick =
    static_cast<float> (grid_item->scrollXPlusWidth ()) / px_per_tick_;

  compute_grid_lines (
    *tempo_map, px_per_tick_, visible_start_tick, visible_end_tick,
    detail_measure_px_threshold_, std::nullopt, grid_lines_);
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
  painter->translate (-scroll_x_, 0.0f);

  const float h = canvas_height_;

  // Draw a batch of vertical lines with the same color
  auto draw_lines =
    [painter, h] (
      const std::vector<GridLine> &lines, const QColor &base_color,
      float opacity) {
      if (lines.empty ())
        return;
      QColor c = base_color;
      c.setAlphaF (opacity);
      painter->setStrokeStyle (c);
      painter->beginPath ();
      for (const auto &line : lines)
        {
          painter->moveTo (line.x, 0.0f);
          painter->lineTo (line.x, h);
        }
      painter->stroke ();
    };

  // Draw back to front: sixteenth -> beat -> bar
  draw_lines (grid_lines_.sixteenth_lines, line_color_, sixteenth_line_opacity_);
  draw_lines (grid_lines_.beat_lines, line_color_, beat_line_opacity_);
  draw_lines (grid_lines_.bar_lines, line_color_, bar_line_opacity_);

  painter->restore ();
}

} // namespace zrythm::gui::qquick
