// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QVector>
#include <QtCanvasPainter/qcanvaspainteritem.h>
#include <QtQmlIntegration/qqmlintegration.h>

#include <kiss_fft.h>

namespace zrythm::dsp
{
class AudioPort;
class PortObservationManager;
}

namespace zrythm::gui::qquick
{

class SpectrumAnalyzerCanvasRenderer;

class SpectrumAnalyzerCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT
  QML_ELEMENT
  Q_PROPERTY (
    zrythm::dsp::AudioPort * stereoPort READ stereoPort WRITE setStereoPort
      NOTIFY stereoPortChanged REQUIRED)
  Q_PROPERTY (
    zrythm::dsp::PortObservationManager * portObservationManager READ
      portObservationManager WRITE setPortObservationManager NOTIFY
        portObservationManagerChanged REQUIRED)
  Q_PROPERTY (int fftSize READ fftSize WRITE setFftSize NOTIFY fftSizeChanged)
  Q_PROPERTY (
    float sampleRate READ sampleRate WRITE setSampleRate NOTIFY
      sampleRateChanged REQUIRED)
  Q_PROPERTY (
    QColor spectrumColor READ spectrumColor WRITE setSpectrumColor NOTIFY
      spectrumColorChanged)

public:
  explicit SpectrumAnalyzerCanvasItem (QQuickItem * parent = nullptr);
  ~SpectrumAnalyzerCanvasItem () override;

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  zrythm::dsp::AudioPort * stereoPort () const;
  void                     setStereoPort (zrythm::dsp::AudioPort * port);

  zrythm::dsp::PortObservationManager * portObservationManager () const;
  void
  setPortObservationManager (zrythm::dsp::PortObservationManager * manager);

  int    fftSize () const;
  float  sampleRate () const;
  QColor spectrumColor () const;

  void setFftSize (int size);
  void setSampleRate (float rate);
  void setSpectrumColor (const QColor &color);

  Q_INVOKABLE float getScaledFrequency (
    int   bin,
    int   num_bins,
    float min_frequency,
    float max_frequency) const;
  Q_INVOKABLE float getFrequencyForBin (int bin, int num_bins) const;

  static float scaled_frequency (
    float sample_rate,
    int   bin,
    int   num_bins,
    float min_frequency,
    float max_frequency);

  const QVector<float> &spectrumData () const;
  uint64_t              spectrumGeneration () const;

Q_SIGNALS:
  void stereoPortChanged ();
  void portObservationManagerChanged ();
  void fftSizeChanged ();
  void sampleRateChanged ();
  void spectrumColorChanged ();

private:
  void process_audio ();
  void try_create_token ();
  void init_fft (int size);

  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
