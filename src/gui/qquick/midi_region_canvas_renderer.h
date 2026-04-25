// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QColor>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

namespace zrythm::gui::qquick
{

class MidiRegionCanvasItem;

class MidiRegionCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  MidiRegionCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (MidiRegionCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  struct NoteRect
  {
    float x;
    float y;
    float width;
    float height;
    bool  muted;
  };

  std::vector<NoteRect> note_rects_;
  QColor                note_color_;
  QColor                dimmed_color_;
  float                 canvas_width_ = 0.0f;
  float                 canvas_height_ = 0.0f;
};

} // namespace zrythm::gui::qquick
