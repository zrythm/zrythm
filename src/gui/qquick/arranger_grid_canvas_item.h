// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QtCanvasPainter/qcanvaspainteritem.h>

namespace zrythm::dsp
{
class TempoMapWrapper;
}

namespace zrythm::gui::qquick
{

class ArrangerGridCanvasRenderer;

/**
 * @brief QML-visible canvas item that renders the arranger background grid.
 *
 * Draws bar, beat, and sixteenth lines imperatively using the GPU-accelerated
 * QCanvasPainter API, replacing the previous Repeater + Rectangle approach.
 */
class ArrangerGridCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (ArrangerGridCanvas)

  Q_PROPERTY (
    dsp::TempoMapWrapper * tempoMap READ tempoMap WRITE setTempoMap NOTIFY
      tempoMapChanged)
  Q_PROPERTY (
    qreal pxPerTick READ pxPerTick WRITE setPxPerTick NOTIFY pxPerTickChanged)
  Q_PROPERTY (qreal scrollX READ scrollX WRITE setScrollX NOTIFY scrollXChanged)
  Q_PROPERTY (
    qreal scrollXPlusWidth READ scrollXPlusWidth WRITE setScrollXPlusWidth
      NOTIFY scrollXPlusWidthChanged)
  Q_PROPERTY (
    QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
  Q_PROPERTY (
    qreal barLineOpacity READ barLineOpacity WRITE setBarLineOpacity NOTIFY
      barLineOpacityChanged)
  Q_PROPERTY (
    qreal beatLineOpacity READ beatLineOpacity WRITE setBeatLineOpacity NOTIFY
      beatLineOpacityChanged)
  Q_PROPERTY (
    qreal sixteenthLineOpacity READ sixteenthLineOpacity WRITE
      setSixteenthLineOpacity NOTIFY sixteenthLineOpacityChanged)
  Q_PROPERTY (
    qreal detailMeasurePxThreshold READ detailMeasurePxThreshold WRITE
      setDetailMeasurePxThreshold NOTIFY detailMeasurePxThresholdChanged)

public:
  explicit ArrangerGridCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  dsp::TempoMapWrapper * tempoMap () const { return tempo_map_; }
  void                   setTempoMap (dsp::TempoMapWrapper * map);
  qreal                  pxPerTick () const { return px_per_tick_; }
  void                   setPxPerTick (qreal px);
  qreal                  scrollX () const { return scroll_x_; }
  void                   setScrollX (qreal x);
  qreal  scrollXPlusWidth () const { return scroll_x_plus_width_; }
  void   setScrollXPlusWidth (qreal w);
  QColor lineColor () const { return line_color_; }
  void   setLineColor (const QColor &color);
  qreal  barLineOpacity () const { return bar_line_opacity_; }
  void   setBarLineOpacity (qreal opacity);
  qreal  beatLineOpacity () const { return beat_line_opacity_; }
  void   setBeatLineOpacity (qreal opacity);
  qreal  sixteenthLineOpacity () const { return sixteenth_line_opacity_; }
  void   setSixteenthLineOpacity (qreal opacity);
  qreal  detailMeasurePxThreshold () const
  {
    return detail_measure_px_threshold_;
  }
  void setDetailMeasurePxThreshold (qreal threshold);

Q_SIGNALS:
  void tempoMapChanged ();
  void pxPerTickChanged ();
  void scrollXChanged ();
  void scrollXPlusWidthChanged ();
  void lineColorChanged ();
  void barLineOpacityChanged ();
  void beatLineOpacityChanged ();
  void sixteenthLineOpacityChanged ();
  void detailMeasurePxThresholdChanged ();

private:
  dsp::TempoMapWrapper * tempo_map_ = nullptr;
  qreal                  px_per_tick_ = 0.0;
  qreal                  scroll_x_ = 0.0;
  qreal                  scroll_x_plus_width_ = 0.0;
  QColor                 line_color_ = Qt::gray;
  qreal                  bar_line_opacity_ = 0.8;
  qreal                  beat_line_opacity_ = 0.6;
  qreal                  sixteenth_line_opacity_ = 0.4;
  qreal                  detail_measure_px_threshold_ = 32.0;
};

} // namespace zrythm::gui::qquick
