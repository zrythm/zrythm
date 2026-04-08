// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/qquick/grid_line_computer.h"

#include <QFont>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class RulerGridCanvasItem;

/**
 * @brief Renders ruler grid lines and labels using QCanvasPainter.
 *
 * Grid line positions are computed in synchronize() (render thread blocked)
 * and drawn in paint() (render thread).
 */
class RulerGridCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  RulerGridCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (RulerGridCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  // Cached visual state from the item
  QColor text_color_;
  float  scroll_x_{ 0.0f };
  float  px_per_tick_{ 0.0f };
  float  bar_line_opacity_{ 0.8f };
  float  beat_line_opacity_{ 0.6f };
  float  sixteenth_line_opacity_{ 0.4f };
  float  detail_measure_px_threshold_{ 32.0f };
  float  detail_measure_label_px_threshold_{ 64.0f };
  float  canvas_height_{ 0.0f };
  QFont  bar_label_font_;
  QFont  beat_label_font_;
  QFont  sixteenth_label_font_;

  // Pre-computed grid lines (computed in synchronize)
  ComputedGridLines grid_lines_;
};

} // namespace zrythm::gui::qquick
