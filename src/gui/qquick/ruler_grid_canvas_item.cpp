// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include "dsp/tempo_map_qml_adapter.h"
#include "gui/qquick/ruler_grid_canvas_item.h"
#include "gui/qquick/ruler_grid_canvas_renderer.h"

namespace zrythm::gui::qquick
{

RulerGridCanvasItem::RulerGridCanvasItem (QQuickItem * parent)
    : QCanvasPainterItem (parent)
{
  setFillColor (Qt::transparent);
  setAlphaBlending (true);
}

QCanvasPainterItemRenderer *
RulerGridCanvasItem::createItemRenderer () const
{
  return new RulerGridCanvasRenderer ();
}

void
RulerGridCanvasItem::setTempoMap (dsp::TempoMapWrapper * map)
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
        &RulerGridCanvasItem::update, Qt::UniqueConnection);
    }

  Q_EMIT tempoMapChanged ();
  update ();
}

void
RulerGridCanvasItem::setPxPerTick (qreal px)
{
  if (qFuzzyCompare (px_per_tick_, px))
    return;
  px_per_tick_ = px;
  Q_EMIT pxPerTickChanged ();
  update ();
}

void
RulerGridCanvasItem::setScrollX (qreal x)
{
  if (qFuzzyCompare (scroll_x_, x))
    return;
  scroll_x_ = x;
  Q_EMIT scrollXChanged ();
  update ();
}

void
RulerGridCanvasItem::setScrollXPlusWidth (qreal w)
{
  if (qFuzzyCompare (scroll_x_plus_width_, w))
    return;
  scroll_x_plus_width_ = w;
  Q_EMIT scrollXPlusWidthChanged ();
  update ();
}

void
RulerGridCanvasItem::setTextColor (const QColor &color)
{
  if (text_color_ == color)
    return;
  text_color_ = color;
  Q_EMIT textColorChanged ();
  update ();
}

void
RulerGridCanvasItem::setBarLineOpacity (qreal opacity)
{
  if (qFuzzyCompare (bar_line_opacity_, opacity))
    return;
  bar_line_opacity_ = opacity;
  Q_EMIT barLineOpacityChanged ();
  update ();
}

void
RulerGridCanvasItem::setBeatLineOpacity (qreal opacity)
{
  if (qFuzzyCompare (beat_line_opacity_, opacity))
    return;
  beat_line_opacity_ = opacity;
  Q_EMIT beatLineOpacityChanged ();
  update ();
}

void
RulerGridCanvasItem::setSixteenthLineOpacity (qreal opacity)
{
  if (qFuzzyCompare (sixteenth_line_opacity_, opacity))
    return;
  sixteenth_line_opacity_ = opacity;
  Q_EMIT sixteenthLineOpacityChanged ();
  update ();
}

void
RulerGridCanvasItem::setDetailMeasurePxThreshold (qreal threshold)
{
  if (qFuzzyCompare (detail_measure_px_threshold_, threshold))
    return;
  detail_measure_px_threshold_ = threshold;
  Q_EMIT detailMeasurePxThresholdChanged ();
  update ();
}

void
RulerGridCanvasItem::setDetailMeasureLabelPxThreshold (qreal threshold)
{
  if (qFuzzyCompare (detail_measure_label_px_threshold_, threshold))
    return;
  detail_measure_label_px_threshold_ = threshold;
  Q_EMIT detailMeasureLabelPxThresholdChanged ();
  update ();
}

void
RulerGridCanvasItem::setBarLabelFont (const QFont &font)
{
  if (bar_label_font_ == font)
    return;
  bar_label_font_ = font;
  Q_EMIT barLabelFontChanged ();
  update ();
}

void
RulerGridCanvasItem::setBeatLabelFont (const QFont &font)
{
  if (beat_label_font_ == font)
    return;
  beat_label_font_ = font;
  Q_EMIT beatLabelFontChanged ();
  update ();
}

void
RulerGridCanvasItem::setSixteenthLabelFont (const QFont &font)
{
  if (sixteenth_label_font_ == font)
    return;
  sixteenth_label_font_ = font;
  Q_EMIT sixteenthLabelFontChanged ();
  update ();
}

} // namespace zrythm::gui::qquick
