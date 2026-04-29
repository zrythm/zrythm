// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/fade_overlay_canvas_item.h"
#include "gui/qquick/fade_overlay_canvas_renderer.h"
#include "structure/arrangement/arranger_object_all.h"
#include "structure/arrangement/audio_region.h"
#include "structure/arrangement/fadeable_object.h"

namespace zrythm::gui::qquick
{

FadeOverlayCanvasItem::FadeOverlayCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
FadeOverlayCanvasItem::createItemRenderer () const
{
  return new FadeOverlayCanvasRenderer ();
}

void
FadeOverlayCanvasItem::setRegion (QObject * region)
{
  if (region_ == region)
    return;
  region_ = region;

  audio_region_ =
    qobject_cast<zrythm::structure::arrangement::AudioRegion *> (region);

  // Disconnect old signal connections from previous region
  for (const auto &connection : region_connections_)
    QObject::disconnect (connection);
  region_connections_.clear ();

  if (audio_region_ != nullptr)
    {
      auto * fade_range = audio_region_->fadeRange ();
      if (fade_range != nullptr)
        {
          region_connections_.push_back (
            QObject::connect (
              fade_range,
              &structure::arrangement::ArrangerObjectFadeRange::
                fadePropertiesChanged,
              this, [this] () { update (); },
              Qt::ConnectionType::QueuedConnection));
        }
    }

  Q_EMIT regionChanged ();
  update ();
}

void
FadeOverlayCanvasItem::setPxPerTick (qreal px)
{
  if (qFuzzyCompare (px_per_tick_, px))
    return;
  px_per_tick_ = px;
  Q_EMIT pxPerTickChanged ();
  update ();
}

void
FadeOverlayCanvasItem::setFadeType (FadeType type)
{
  if (fade_type_ == type)
    return;
  fade_type_ = type;
  Q_EMIT fadeTypeChanged ();
  update ();
}

void
FadeOverlayCanvasItem::setHovered (bool hovered)
{
  if (hovered_ == hovered)
    return;
  hovered_ = hovered;
  Q_EMIT hoveredChanged ();
  update ();
}

void
FadeOverlayCanvasItem::setOverlayColor (const QColor &color)
{
  if (overlay_color_ == color)
    return;
  overlay_color_ = color;
  Q_EMIT overlayColorChanged ();
  update ();
}

void
FadeOverlayCanvasItem::setCurveColor (const QColor &color)
{
  if (curve_color_ == color)
    return;
  curve_color_ = color;
  Q_EMIT curveColorChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
