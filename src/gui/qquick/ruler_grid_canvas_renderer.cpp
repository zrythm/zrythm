// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map_qml_adapter.h"
#include "gui/qquick/ruler_grid_canvas_item.h"
#include "gui/qquick/ruler_grid_canvas_renderer.h"

namespace zrythm::gui::qquick
{

void
RulerGridCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * ruler_item = static_cast<RulerGridCanvasItem *> (item);

  text_color_ = ruler_item->textColor ();
  scroll_x_ = static_cast<float> (ruler_item->scrollX ());
  px_per_tick_ = static_cast<float> (ruler_item->pxPerTick ());
  bar_line_opacity_ = static_cast<float> (ruler_item->barLineOpacity ());
  beat_line_opacity_ = static_cast<float> (ruler_item->beatLineOpacity ());
  sixteenth_line_opacity_ =
    static_cast<float> (ruler_item->sixteenthLineOpacity ());
  detail_measure_px_threshold_ =
    static_cast<float> (ruler_item->detailMeasurePxThreshold ());
  detail_measure_label_px_threshold_ =
    static_cast<float> (ruler_item->detailMeasureLabelPxThreshold ());
  canvas_height_ = static_cast<float> (ruler_item->height ());
  bar_label_font_ = ruler_item->barLabelFont ();
  beat_label_font_ = ruler_item->beatLabelFont ();
  sixteenth_label_font_ = ruler_item->sixteenthLabelFont ();

  auto * tempo_map = ruler_item->tempoMap ();
  if (tempo_map == nullptr || px_per_tick_ <= 0.0)
    {
      grid_lines_.clear ();
      return;
    }

  const float visible_start_tick = scroll_x_ / px_per_tick_;
  const float visible_end_tick =
    static_cast<float> (ruler_item->scrollXPlusWidth ()) / px_per_tick_;

  compute_grid_lines (
    *tempo_map, px_per_tick_, visible_start_tick, visible_end_tick,
    detail_measure_px_threshold_, detail_measure_label_px_threshold_,
    grid_lines_);
}

void
RulerGridCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (canvas_height_ <= 0.0f)
    return;

  painter->setRenderHint (QCanvasPainter::RenderHint::Antialiasing, false);
  painter->setRenderHint (
    QCanvasPainter::RenderHint::HighQualityStroking, false);

  // The canvas is viewport-sized. Translate so content-space coordinates
  // map to the visible portion.
  painter->save ();
  painter->translate (-scroll_x_, 0.0f);

  // --- Sixteenth lines (back) ---
  if (!grid_lines_.sixteenth_lines.empty ())
    {
      QColor c = text_color_;
      c.setAlphaF (sixteenth_line_opacity_);
      painter->setStrokeStyle (c);
      painter->setLineWidth (1.0f);
      painter->beginPath ();
      for (const auto &line : grid_lines_.sixteenth_lines)
        {
          painter->moveTo (line.x, 0.0f);
          painter->lineTo (line.x, 8.0f);
        }
      painter->stroke ();
    }

  // --- Sixteenth labels ---
  if (grid_lines_.show_sixteenth_labels && !grid_lines_.sixteenth_lines.empty ())
    {
      QColor c = text_color_;
      c.setAlphaF (sixteenth_line_opacity_);
      painter->setFillStyle (c);
      painter->setFont (sixteenth_label_font_);
      painter->setTextAlign (QCanvasPainter::TextAlign::Left);
      painter->setTextBaseline (QCanvasPainter::TextBaseline::Top);
      for (const auto &line : grid_lines_.sixteenth_lines)
        {
          painter->fillText (
            QString ("%1.%2.%3").arg (line.bar).arg (line.beat).arg (line.sixteenth),
            line.x + 2.0f, 0.0f);
        }
    }

  // --- Beat lines ---
  if (!grid_lines_.beat_lines.empty ())
    {
      QColor c = text_color_;
      c.setAlphaF (beat_line_opacity_);
      painter->setStrokeStyle (c);
      painter->setLineWidth (1.0f);
      painter->beginPath ();
      for (const auto &line : grid_lines_.beat_lines)
        {
          painter->moveTo (line.x, 0.0f);
          painter->lineTo (line.x, 10.0f);
        }
      painter->stroke ();
    }

  // --- Beat labels ---
  if (!grid_lines_.beat_label_lines.empty ())
    {
      QColor c = text_color_;
      c.setAlphaF (beat_line_opacity_);
      painter->setFillStyle (c);
      painter->setFont (beat_label_font_);
      painter->setTextAlign (QCanvasPainter::TextAlign::Left);
      painter->setTextBaseline (QCanvasPainter::TextBaseline::Top);
      for (const auto &line : grid_lines_.beat_label_lines)
        {
          painter->fillText (
            QString ("%1.%2").arg (line.bar).arg (line.beat), line.x + 2.0f,
            0.0f);
        }
    }

  // --- Bar lines (front) ---
  if (!grid_lines_.bar_lines.empty ())
    {
      QColor c = text_color_;
      c.setAlphaF (bar_line_opacity_);
      painter->setStrokeStyle (c);
      painter->setLineWidth (2.0f);
      painter->beginPath ();
      for (const auto &line : grid_lines_.bar_lines)
        {
          painter->moveTo (line.x, 0.0f);
          painter->lineTo (line.x, 14.0f);
        }
      painter->stroke ();
    }

  // --- Bar labels ---
  if (!grid_lines_.bar_lines.empty ())
    {
      QColor c = text_color_;
      c.setAlphaF (bar_line_opacity_);
      painter->setFillStyle (c);
      painter->setFont (bar_label_font_);
      painter->setTextAlign (QCanvasPainter::TextAlign::Left);
      painter->setTextBaseline (QCanvasPainter::TextBaseline::Bottom);
      for (const auto &line : grid_lines_.bar_lines)
        {
          painter->fillText (QString::number (line.bar), line.x + 4.0f, 14.0f);
        }
    }

  painter->restore ();
}

} // namespace zrythm::gui::qquick
