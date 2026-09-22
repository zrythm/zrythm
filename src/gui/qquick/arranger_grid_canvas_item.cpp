// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map_qml_adapter.h"
#include "gui/qquick/arranger_grid_canvas_item.h"
#include "gui/qquick/arranger_grid_canvas_renderer.h"

namespace zrythm::gui::qquick
{

ArrangerGridCanvasItem::ArrangerGridCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
ArrangerGridCanvasItem::createItemRenderer () const
{
  return new ArrangerGridCanvasRenderer ();
}

void
ArrangerGridCanvasItem::setTempoMap (dsp::TempoMapWrapper * map)
{
  if (tempo_map_ == map)
    return;

  if (tempo_map_ != nullptr)
    QObject::disconnect (tempo_map_, nullptr, this, nullptr);

  tempo_map_ = map;

  if (tempo_map_ != nullptr)
    {
      QObject::connect (
        tempo_map_, &dsp::TempoMapWrapper::timeSignatureEventsChanged, this,
        &ArrangerGridCanvasItem::update, Qt::UniqueConnection);
    }

  Q_EMIT tempoMapChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setPxPerTick (double px)
{
  if (qFuzzyCompare (px_per_tick_, px))
    return;
  px_per_tick_ = px;
  Q_EMIT pxPerTickChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setScrollX (double x)
{
  if (qFuzzyCompare (scroll_x_, x))
    return;
  scroll_x_ = x;
  Q_EMIT scrollXChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setScrollXPlusWidth (double w)
{
  if (qFuzzyCompare (scroll_x_plus_width_, w))
    return;
  scroll_x_plus_width_ = w;
  Q_EMIT scrollXPlusWidthChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setLineColor (const QColor &color)
{
  if (line_color_ == color)
    return;
  line_color_ = color;
  Q_EMIT lineColorChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setBarShadeColor (const QColor &color)
{
  if (bar_shade_color_ == color)
    return;
  bar_shade_color_ = color;
  Q_EMIT barShadeColorChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setBarLineOpacity (double opacity)
{
  if (qFuzzyCompare (bar_line_opacity_, opacity))
    return;
  bar_line_opacity_ = opacity;
  Q_EMIT barLineOpacityChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setBeatLineOpacity (double opacity)
{
  if (qFuzzyCompare (beat_line_opacity_, opacity))
    return;
  beat_line_opacity_ = opacity;
  Q_EMIT beatLineOpacityChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setSixteenthLineOpacity (double opacity)
{
  if (qFuzzyCompare (sixteenth_line_opacity_, opacity))
    return;
  sixteenth_line_opacity_ = opacity;
  Q_EMIT sixteenthLineOpacityChanged ();
  update ();
}

void
ArrangerGridCanvasItem::setDetailMeasurePxThreshold (double threshold)
{
  if (qFuzzyCompare (detail_measure_px_threshold_, threshold))
    return;
  detail_measure_px_threshold_ = threshold;
  Q_EMIT detailMeasurePxThresholdChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
