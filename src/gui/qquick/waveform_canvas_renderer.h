// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QColor>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::gui::qquick
{

class WaveformCanvasItem;

/**
 * @brief Renders audio waveform peaks using QCanvasPainter.
 *
 * Peak computation happens in synchronize() (render thread).
 * Drawing happens in paint() (render thread).
 */
class WaveformCanvasRenderer : public QCanvasPainterItemRenderer
{
public:
  WaveformCanvasRenderer () = default;
  Q_DISABLE_COPY_MOVE (WaveformCanvasRenderer)

  void synchronize (QCanvasPainterItem * item) override;
  void paint (QCanvasPainter * painter) override;

private:
  struct Peak
  {
    float min; // normalized 0-1
    float max; // normalized 0-1
  };

  using ChannelPeaks = std::vector<Peak>;

  void compute_peaks ();

  // Cached visual state from the item
  QColor waveform_color_;
  QColor outline_color_;
  float  canvas_width_ = 0.0f;
  float  canvas_height_ = 0.0f;

  // Cached pointer to the item's serialized audio buffer (owned by the item)
  const juce::AudioSampleBuffer * audio_buffer_ = nullptr;

  // Cached peak data
  std::vector<ChannelPeaks> peaks_;
  int                       num_channels_ = 0;

  // Change detection
  uint64_t prev_generation_ = 0;
  float    prev_width_ = 0.0f;
  float    prev_height_ = 0.0f;
};

} // namespace zrythm::gui::qquick
