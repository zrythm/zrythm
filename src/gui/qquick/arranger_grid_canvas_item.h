// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QPointer>
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
    zrythm::dsp::TempoMapWrapper * tempoMap READ tempoMap WRITE setTempoMap
      NOTIFY tempoMapChanged)
  Q_PROPERTY (
    double pxPerTick READ pxPerTick WRITE setPxPerTick NOTIFY pxPerTickChanged)
  Q_PROPERTY (double scrollX READ scrollX WRITE setScrollX NOTIFY scrollXChanged)
  Q_PROPERTY (
    double scrollXPlusWidth READ scrollXPlusWidth WRITE setScrollXPlusWidth
      NOTIFY scrollXPlusWidthChanged)
  Q_PROPERTY (
    QColor lineColor READ lineColor WRITE setLineColor NOTIFY lineColorChanged)
  Q_PROPERTY (
    QColor barShadeColor READ barShadeColor WRITE setBarShadeColor NOTIFY
      barShadeColorChanged)
  Q_PROPERTY (
    double barLineOpacity READ barLineOpacity WRITE setBarLineOpacity NOTIFY
      barLineOpacityChanged)
  Q_PROPERTY (
    double beatLineOpacity READ beatLineOpacity WRITE setBeatLineOpacity NOTIFY
      beatLineOpacityChanged)
  Q_PROPERTY (
    double sixteenthLineOpacity READ sixteenthLineOpacity WRITE
      setSixteenthLineOpacity NOTIFY sixteenthLineOpacityChanged)
  Q_PROPERTY (
    double detailMeasurePxThreshold READ detailMeasurePxThreshold WRITE
      setDetailMeasurePxThreshold NOTIFY detailMeasurePxThresholdChanged)

public:
  explicit ArrangerGridCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  zrythm::dsp::TempoMapWrapper * tempoMap () const { return tempo_map_; }
  void   setTempoMap (zrythm::dsp::TempoMapWrapper * map);
  double pxPerTick () const { return px_per_tick_; }
  void   setPxPerTick (double px);
  double scrollX () const { return scroll_x_; }
  void   setScrollX (double x);
  double scrollXPlusWidth () const { return scroll_x_plus_width_; }
  void   setScrollXPlusWidth (double w);
  QColor lineColor () const { return line_color_; }
  void   setLineColor (const QColor &color);
  QColor barShadeColor () const { return bar_shade_color_; }
  void   setBarShadeColor (const QColor &color);
  double barLineOpacity () const { return bar_line_opacity_; }
  void   setBarLineOpacity (double opacity);
  double beatLineOpacity () const { return beat_line_opacity_; }
  void   setBeatLineOpacity (double opacity);
  double sixteenthLineOpacity () const { return sixteenth_line_opacity_; }
  void   setSixteenthLineOpacity (double opacity);
  double detailMeasurePxThreshold () const
  {
    return detail_measure_px_threshold_;
  }
  void setDetailMeasurePxThreshold (double threshold);

Q_SIGNALS:
  void tempoMapChanged ();
  void pxPerTickChanged ();
  void scrollXChanged ();
  void scrollXPlusWidthChanged ();
  void lineColorChanged ();
  void barShadeColorChanged ();
  void barLineOpacityChanged ();
  void beatLineOpacityChanged ();
  void sixteenthLineOpacityChanged ();
  void detailMeasurePxThresholdChanged ();

private:
  QPointer<zrythm::dsp::TempoMapWrapper> tempo_map_;
  double                                 px_per_tick_ = 0.0;
  double                                 scroll_x_ = 0.0;
  double                                 scroll_x_plus_width_ = 0.0;
  QColor                                 line_color_ = Qt::gray;
  QColor                                 bar_shade_color_ = Qt::transparent;
  double                                 bar_line_opacity_ = 0.8;
  double                                 beat_line_opacity_ = 0.6;
  double                                 sixteenth_line_opacity_ = 0.4;
  double                                 detail_measure_px_threshold_ = 32.0;
};

} // namespace zrythm::gui::qquick
