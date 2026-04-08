// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class FadeOverlayCanvasItem;

class FadeOverlayCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  FadeOverlayCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (FadeOverlayCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  int    fade_type_ = 0;
  bool   hovered_ = false;
  QColor overlay_color_{ 51, 51, 51, 153 };
  QColor curve_color_{ 255, 255, 255, 200 };
  float  canvas_width_ = 0.0f;
  float  canvas_height_ = 0.0f;
  bool   has_valid_fade_ = false;

  // Pre-computed curve Y values (copied from item during synchronize)
  std::vector<double> cached_curve_y_;
};

} // namespace zrythm::gui::qquick
