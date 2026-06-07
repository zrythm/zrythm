// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QVector>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class SpectrumAnalyzerCanvasItem;

class SpectrumAnalyzerCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  SpectrumAnalyzerCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (SpectrumAnalyzerCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  QColor         spectrum_color_;
  float          canvas_width_ = 0.0f;
  float          canvas_height_ = 0.0f;
  QVector<float> spectrum_data_;
  float          sample_rate_ = 44100.0f;
  uint64_t       prev_generation_ = 0;
};

}
