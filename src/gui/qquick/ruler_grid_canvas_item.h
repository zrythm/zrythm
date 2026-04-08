// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QFont>
#include <QPointer>
#include <QtCanvasPainter/qcanvaspainteritem.h>

namespace zrythm::dsp
{
class TempoMapWrapper;
}

namespace zrythm::gui::qquick
{

class RulerGridCanvasRenderer;

/**
 * @brief QML-visible canvas item that renders the ruler grid lines and labels.
 *
 * Draws bar, beat, and sixteenth lines with text labels imperatively using
 * the GPU-accelerated QCanvasPainter API, replacing the previous
 * Repeater + Rectangle approach.
 */
class RulerGridCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (RulerGridCanvas)

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
    QColor textColor READ textColor WRITE setTextColor NOTIFY textColorChanged)
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
  Q_PROPERTY (
    qreal detailMeasureLabelPxThreshold READ detailMeasureLabelPxThreshold WRITE
      setDetailMeasureLabelPxThreshold NOTIFY detailMeasureLabelPxThresholdChanged)
  Q_PROPERTY (
    QFont barLabelFont READ barLabelFont WRITE setBarLabelFont NOTIFY
      barLabelFontChanged)
  Q_PROPERTY (
    QFont beatLabelFont READ beatLabelFont WRITE setBeatLabelFont NOTIFY
      beatLabelFontChanged)
  Q_PROPERTY (
    QFont sixteenthLabelFont READ sixteenthLabelFont WRITE setSixteenthLabelFont
      NOTIFY sixteenthLabelFontChanged)

public:
  explicit RulerGridCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  dsp::TempoMapWrapper * tempoMap () const { return tempo_map_; }
  void                   setTempoMap (dsp::TempoMapWrapper * map);
  qreal                  pxPerTick () const { return px_per_tick_; }
  void                   setPxPerTick (qreal px);
  qreal                  scrollX () const { return scroll_x_; }
  void                   setScrollX (qreal x);
  qreal  scrollXPlusWidth () const { return scroll_x_plus_width_; }
  void   setScrollXPlusWidth (qreal w);
  QColor textColor () const { return text_color_; }
  void   setTextColor (const QColor &color);
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
  void  setDetailMeasurePxThreshold (qreal threshold);
  qreal detailMeasureLabelPxThreshold () const
  {
    return detail_measure_label_px_threshold_;
  }
  void  setDetailMeasureLabelPxThreshold (qreal threshold);
  QFont barLabelFont () const { return bar_label_font_; }
  void  setBarLabelFont (const QFont &font);
  QFont beatLabelFont () const { return beat_label_font_; }
  void  setBeatLabelFont (const QFont &font);
  QFont sixteenthLabelFont () const { return sixteenth_label_font_; }
  void  setSixteenthLabelFont (const QFont &font);

Q_SIGNALS:
  void tempoMapChanged ();
  void pxPerTickChanged ();
  void scrollXChanged ();
  void scrollXPlusWidthChanged ();
  void textColorChanged ();
  void barLineOpacityChanged ();
  void beatLineOpacityChanged ();
  void sixteenthLineOpacityChanged ();
  void detailMeasurePxThresholdChanged ();
  void detailMeasureLabelPxThresholdChanged ();
  void barLabelFontChanged ();
  void beatLabelFontChanged ();
  void sixteenthLabelFontChanged ();

private:
  QPointer<dsp::TempoMapWrapper> tempo_map_;
  qreal                          px_per_tick_ = 0.0;
  qreal                          scroll_x_ = 0.0;
  qreal                          scroll_x_plus_width_ = 0.0;
  QColor                         text_color_ = Qt::black;
  qreal                          bar_line_opacity_ = 0.8;
  qreal                          beat_line_opacity_ = 0.6;
  qreal                          sixteenth_line_opacity_ = 0.4;
  qreal                          detail_measure_px_threshold_ = 32.0;
  qreal                          detail_measure_label_px_threshold_ = 64.0;
  QFont                          bar_label_font_;
  QFont                          beat_label_font_;
  QFont                          sixteenth_label_font_;
};

} // namespace zrythm::gui::qquick
