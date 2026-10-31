// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include "gui/qquick/grid_line_computer.h"

#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class ArrangerGridCanvasItem;

/**
 * @brief Renders arranger background grid lines using QCanvasPainter.
 *
 * Grid line positions are computed in synchronizeData() (render thread blocked)
 * and drawn in paint() (render thread).
 */
class ArrangerGridCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  ArrangerGridCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (ArrangerGridCanvasRenderer)

  void synchronizeData (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  // Cached visual state from the item
  QColor line_color_;
  QColor bar_shade_color_;
  float  scroll_x_{ 0.0f };
  float  px_per_tick_{ 0.0f };
  float  bar_line_opacity_{ 0.8f };
  float  beat_line_opacity_{ 0.6f };
  float  sixteenth_line_opacity_{ 0.4f };
  float  detail_measure_px_threshold_{ 32.0f };
  float  canvas_height_{ 0.0f };

  // Pre-computed grid lines (computed in synchronizeData)
  ComputedGridLines grid_lines_;
};

} // namespace zrythm::gui::qquick
