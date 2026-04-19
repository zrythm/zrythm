// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <QColor>
#include <QtCanvasPainter/qcanvaspainteritem.h>

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::gui::qquick
{

class WaveformCanvasRenderer;

/**
 * @brief Generic base for hardware-accelerated waveform rendering.
 *
 * Owns the audio buffer storage and visual properties.
 * Subclasses provide the buffer data by calling setAudioBuffer().
 * Not registered to QML — use a derived class for QML usage.
 */
class WaveformCanvasItem : public QCanvasPainterItem
{
  Q_OBJECT

  Q_PROPERTY (
    QColor waveformColor READ waveformColor WRITE setWaveformColor NOTIFY
      waveformColorChanged)
  Q_PROPERTY (
    QColor outlineColor READ outlineColor WRITE setOutlineColor NOTIFY
      outlineColorChanged)

public:
  explicit WaveformCanvasItem (QQuickItem * parent = nullptr);

  QCanvasPainterItemRenderer * createItemRenderer () const override;

  QColor waveformColor () const { return waveform_color_; }
  void   setWaveformColor (const QColor &color);
  QColor outlineColor () const { return outline_color_; }
  void   setOutlineColor (const QColor &color);

  const juce::AudioSampleBuffer * audioBuffer () const
  {
    return (audio_buffer_.getNumSamples () > 0) ? &audio_buffer_ : nullptr;
  }

  /**
   * @brief Monotonically increasing counter bumped on each buffer change.
   *
   * The renderer compares this against its own stored value to decide
   * whether peak recomputation is needed — pure read-only access.
   */
  uint64_t bufferGeneration () const { return buffer_generation_; }

protected:
  /**
   * @brief Bumps the generation counter and schedules a repaint.
   *
   * Call after writing directly into audio_buffer_.
   */
  void notifyBufferChanged ();

  juce::AudioSampleBuffer audio_buffer_;

Q_SIGNALS:
  void waveformColorChanged ();
  void outlineColorChanged ();

private:
  QColor   waveform_color_;
  QColor   outline_color_;
  uint64_t buffer_generation_ = 0;
};

} // namespace zrythm::gui::qquick
