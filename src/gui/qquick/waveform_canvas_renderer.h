// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <vector>

#include <QColor>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>
#include <QtQmlIntegration/qqmlintegration.h>

#include "juce_wrapper.h"

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
  explicit WaveformCanvasRenderer ();
  ~WaveformCanvasRenderer () override;

  void initializeResources (QCanvasPainter * painter) override;
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

  // Cached state from QML item
  QObject * region_ = nullptr;
  qreal     px_per_tick_ = 1.0;
  QColor    waveform_color_;
  QColor    outline_color_;
  float     canvas_width_ = 0.0f;
  float     canvas_height_ = 0.0f;

  // Cached pointer to the item's serialized audio buffer (owned by the item)
  const juce::AudioSampleBuffer * audio_buffer_ = nullptr;

  // Cached peak data
  std::vector<ChannelPeaks> peaks_;
  int                       num_channels_ = 0;

  // Cache invalidation
  bool needs_recompute_ = true;
};

} // namespace zrythm::gui::qquick
