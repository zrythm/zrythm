// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "gui/qquick/fade_overlay_canvas_item.h"
#include "gui/qquick/fade_overlay_canvas_renderer.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/fadeable_object.h"

namespace zrythm::gui::qquick
{

void
FadeOverlayCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * fade_item = static_cast<FadeOverlayCanvasItem *> (item);
  fade_type_ = static_cast<int> (fade_item->fadeType ());
  hovered_ = fade_item->hovered ();
  overlay_color_ = fade_item->overlayColor ();
  curve_color_ = fade_item->curveColor ();
  canvas_width_ = static_cast<float> (fade_item->width ());
  canvas_height_ = static_cast<float> (fade_item->height ());

  // Compute curve Y values on the main thread (render thread is blocked).
  // This avoids accessing the live AudioRegion/FadeRange from paint().
  cached_curve_y_.clear ();
  has_valid_fade_ = false;

  auto * audio_region = fade_item->audioRegion ();
  if (audio_region != nullptr)
    {
      auto * fade_range = audio_region->fadeRange ();
      if (fade_range != nullptr)
        {
          const bool fade_in = (fade_type_ == FadeOverlayCanvasItem::FadeIn);
          const int  steps = std::max (2, static_cast<int> (canvas_width_ / 2));
          cached_curve_y_.resize (steps + 1);
          for (const auto i : std::views::iota (0, steps + 1))
            {
              const double normalized_x = static_cast<double> (i) / steps;
              cached_curve_y_[i] =
                fade_range->get_normalized_y_for_fade (normalized_x, fade_in);
            }
          has_valid_fade_ = true;
        }
    }
}

void
FadeOverlayCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (!has_valid_fade_ || cached_curve_y_.empty ())
    return;

  const float w = canvas_width_;
  const float h = canvas_height_;
  if (w <= 0 || h <= 0)
    return;

  const int steps = static_cast<int> (cached_curve_y_.size ()) - 1;

  // Filled overlay: top edge + curve traced in reverse
  painter->beginPath ();
  painter->moveTo (0.0f, 0.0f);
  painter->lineTo (w, 0.0f);

  for (const auto i : std::views::iota (0, steps + 1) | std::views::reverse)
    {
      const float x = static_cast<float> (i) * w / static_cast<float> (steps);
      painter->lineTo (x, static_cast<float> (1.0 - cached_curve_y_[i]) * h);
    }

  painter->closePath ();
  painter->setFillStyle (overlay_color_);
  painter->fill ();

  if (!hovered_)
    return;

  // Hovered curve stroke
  painter->beginPath ();
  for (const auto i : std::views::iota (0, steps + 1))
    {
      const float x = static_cast<float> (i) * w / static_cast<float> (steps);
      const float y = static_cast<float> (1.0 - cached_curve_y_[i]) * h;
      if (i == 0)
        painter->moveTo (x, y);
      else
        painter->lineTo (x, y);
    }

  painter->setStrokeStyle (curve_color_);
  painter->setLineWidth (2.0f);
  painter->stroke ();
}

} // namespace zrythm::gui::qquick
