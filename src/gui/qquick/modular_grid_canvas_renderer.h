// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class ModularGridCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  void synchronizeData (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  QColor dot_color_;
  float  pan_x_ = 0.0f;
  float  pan_y_ = 0.0f;
  float  zoom_ = 1.0f;
  float  canvas_width_ = 0.0f;
  float  canvas_height_ = 0.0f;
};

} // namespace zrythm::gui::qquick
