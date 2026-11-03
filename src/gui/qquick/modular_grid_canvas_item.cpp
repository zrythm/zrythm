// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "gui/qquick/modular_grid_canvas_item.h"
#include "gui/qquick/modular_grid_canvas_renderer.h"

namespace zrythm::gui::qquick
{

ModularGridCanvasItem::ModularGridCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
ModularGridCanvasItem::createItemRenderer () const
{
  return new ModularGridCanvasRenderer ();
}

void
ModularGridCanvasItem::setPanX (double x)
{
  if (qFuzzyCompare (pan_x_, x))
    return;
  pan_x_ = x;
  Q_EMIT panXChanged ();
  update ();
}

void
ModularGridCanvasItem::setPanY (double y)
{
  if (qFuzzyCompare (pan_y_, y))
    return;
  pan_y_ = y;
  Q_EMIT panYChanged ();
  update ();
}

void
ModularGridCanvasItem::setZoom (double zoom)
{
  if (qFuzzyCompare (zoom_, zoom))
    return;
  zoom_ = zoom;
  Q_EMIT zoomChanged ();
  update ();
}

void
ModularGridCanvasItem::setDotColor (const QColor &color)
{
  if (dot_color_ == color)
    return;
  dot_color_ = color;
  Q_EMIT dotColorChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
