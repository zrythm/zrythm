// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>

#include "gui/qquick/modular_grid_canvas_item.h"
#include "gui/qquick/modular_grid_canvas_renderer.h"

namespace zrythm::gui::qquick
{

void
ModularGridCanvasRenderer::synchronizeData (QCanvasPainterItem * item)
{
  const auto * grid_item = static_cast<ModularGridCanvasItem *> (item);
  dot_color_ = grid_item->dotColor ();
  pan_x_ = static_cast<float> (grid_item->panX ());
  pan_y_ = static_cast<float> (grid_item->panY ());
  zoom_ = static_cast<float> (grid_item->zoom ());
  canvas_width_ = static_cast<float> (grid_item->width ());
  canvas_height_ = static_cast<float> (grid_item->height ());
}

void
ModularGridCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (canvas_width_ <= 0.0f || canvas_height_ <= 0.0f || zoom_ <= 0.0f)
    return;

  float spacing = 24.0f * zoom_;
  while (spacing < 12.0f)
    spacing *= 2.0f;

  const auto wrap = [spacing] (float value) {
    const float wrapped = std::fmod (value, spacing);
    return wrapped < 0.0f ? wrapped + spacing : wrapped;
  };
  const float start_x = wrap (pan_x_) - spacing;
  const float start_y = wrap (pan_y_) - spacing;
  const float dot_size = std::clamp (1.5f * zoom_, 1.0f, 3.0f);

  painter->setAntialias (0.0);
  painter->setFillStyle (dot_color_);
  for (float x = start_x; x < canvas_width_ + spacing; x += spacing)
    {
      for (float y = start_y; y < canvas_height_ + spacing; y += spacing)
        painter->fillRect (x, y, dot_size, dot_size);
    }
}

} // namespace zrythm::gui::qquick
