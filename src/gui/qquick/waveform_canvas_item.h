// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QColor>
#include <QPointer>
#include <QtCanvasPainter/qcanvaspainteritem.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>
#include <QtQmlIntegration/qqmlintegration.h>

#include "juce_wrapper.h"

namespace zrythm::structure::arrangement
{
class AudioRegion;
}

namespace zrythm::gui::qquick
{

class WaveformCanvasRenderer;

/**
 * @brief QML-visible canvas item that renders an audio waveform.
 *
 * Exposes properties for the audio region and scroll/zoom state.
 * The actual painting is done by WaveformCanvasRenderer.
 */
class WaveformCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_NAMED_ELEMENT (WaveformCanvas)

  Q_PROPERTY (QObject * region READ region WRITE setRegion NOTIFY regionChanged)
  Q_PROPERTY (
    qreal pxPerTick READ pxPerTick WRITE setPxPerTick NOTIFY pxPerTickChanged)
  Q_PROPERTY (
    QColor waveformColor READ waveformColor WRITE setWaveformColor NOTIFY
      waveformColorChanged)
  Q_PROPERTY (
    QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY
      outlineColorChanged)

public:
  explicit WaveformCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  QObject * region () const { return region_; }
  void      setRegion (QObject * region);
  qreal     pxPerTick () const { return px_per_tick_; }
  void      setPxPerTick (qreal px);
  QColor    waveformColor () const { return waveform_color_; }
  void      setWaveformColor (const QColor &color);
  QColor    outlineColor () const { return outline_color_; }
  void      setOutlineColor (const QColor &color);

  const juce::AudioSampleBuffer * audioBuffer () const
  {
    return audio_buffer_ ? &*audio_buffer_ : nullptr;
  }

private:
  void re_serialize_buffer ();

Q_SIGNALS:
  void regionChanged ();
  void pxPerTickChanged ();
  void waveformColorChanged ();
  void outlineColorChanged ();

private:
  QObject *                                     region_ = nullptr;
  QPointer<structure::arrangement::AudioRegion> audio_region_;
  qreal                                         px_per_tick_ = 1.0;
  QColor                                        waveform_color_;
  QColor                                        outline_color_;

  std::optional<juce::AudioSampleBuffer> audio_buffer_;
  std::vector<QMetaObject::Connection>   region_connections_;
};

} // namespace zrythm::gui::qquick
