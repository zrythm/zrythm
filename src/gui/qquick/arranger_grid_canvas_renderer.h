// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QColor>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class ArrangerGridCanvasItem;

/**
 * @brief Renders arranger background grid lines using QCanvasPainter.
 *
 * Grid line positions are computed in synchronize() (render thread blocked)
 * and drawn in paint() (render thread).
 */
class ArrangerGridCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  ArrangerGridCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (ArrangerGridCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  // Cached visual state from the item
  QColor line_color_;
  qreal  scroll_x_ = 0.0;
  qreal  px_per_tick_ = 0.0;
  qreal  bar_line_opacity_ = 0.8;
  qreal  beat_line_opacity_ = 0.6;
  qreal  sixteenth_line_opacity_ = 0.4;
  qreal  detail_measure_px_threshold_ = 32.0;
  float  canvas_height_ = 0.0f;

  // Pre-computed grid line x-positions (computed in synchronize)
  std::vector<qreal> bar_lines_;
  std::vector<qreal> beat_lines_;
  std::vector<qreal> sixteenth_lines_;
};

} // namespace zrythm::gui::qquick
