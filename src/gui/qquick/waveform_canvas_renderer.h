// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#pragma once

#include <functional>
#include <vector>

#include "dsp/tempo_map.h"
#include "dsp/tick_types.h"

#include <QColor>
#include <QtCanvasPainter/qcanvaspainter.h>
#include <QtCanvasPainter/qcanvaspainteritemrenderer.h>

#include <juce_audio_basics/juce_audio_basics.h>

namespace zrythm::gui::qquick
{

class WaveformCanvasItem;

/// A min/max peak pair for one pixel column, normalized to [0, 1].
struct WaveformPeak
{
  float min;
  float max;
};

/// Computes per-pixel buffer frame indices for waveform rendering.
///
/// For each pixel X (0 to @p canvas_width), maps to a content fraction
/// via @p (X + reference_x) / reference_width, then applies
/// @p content_fraction_to_frame to get the buffer frame index.
///
/// For constant tempo, the converter is linear (@p frac * total_frames).
/// For tempo ramps, it follows the clip's ContentTimeWarp, producing a
/// non-linear mapping so the waveform matches what plays at each timeline
/// position.
///
/// @return Vector of @p canvas_width + 1 frame indices. Pixel X covers
///         frames [result[X], result[X + 1]).
std::vector<int64_t>
compute_frame_mapping (
  int                             canvas_width,
  double                          reference_width,
  double                          reference_x,
  std::function<int64_t (double)> content_fraction_to_frame);

/// Convenience for the common case: linear mapping where each content
/// fraction maps to @p frac * @p total_frames. Equivalent to calling
/// @ref compute_frame_mapping with a linear converter.
std::vector<int64_t>
compute_linear_frame_mapping (
  int     canvas_width,
  double  reference_width,
  double  reference_x,
  int64_t total_frames);

/// Maps per-pixel content fractions to buffer frames using the tempo map's
/// tick-to-sample conversion.
///
/// Each pixel X maps to a timeline tick via
/// @p clip_start_tick + fraction * @p timeline_tick_duration, then to
/// a buffer-relative sample via @p tempo_map.tick_to_samples_rounded().
/// This produces a non-linear mapping when tempo changes within the clip,
/// so the waveform visually aligns with what plays at each timeline position.
///
/// @return Vector of @p canvas_width + 1 frame indices.
std::vector<int64_t>
compute_timeline_frame_mapping (
  int                  canvas_width,
  double               reference_width,
  double               reference_x,
  const dsp::TempoMap &tempo_map,
  dsp::TimelineTick    clip_start_tick,
  double               timeline_tick_duration);

/// Computes waveform peaks for an audio buffer using a precomputed
/// per-pixel frame mapping.
///
/// @param buffer         Source audio (can have multiple channels).
/// @param pixel_frames   Frame indices for each pixel boundary (size must be
///                       at least @p canvas_width + 1). Pixel X covers
///                       frames [pixel_frames[X], pixel_frames[X + 1]).
/// @param canvas_width   Number of pixel columns to compute.
/// @param loop_wrap_start  Where the loop region begins in the buffer
///                         (buffer-relative index).
/// @param loop_wrap_length Length of one loop iteration in the buffer.
/// @param has_loop       Whether to wrap out-of-range pixels into the loop
///                         region.
///
/// @return Peaks indexed as `[channel][pixel]` — outer dimension is one per
///         audio channel, inner dimension is one per pixel column.
std::vector<std::vector<WaveformPeak>>
compute_waveform_peaks (
  const juce::AudioSampleBuffer &buffer,
  const std::vector<int64_t>    &pixel_frames,
  int                            canvas_width,
  int64_t                        loop_wrap_start,
  int64_t                        loop_wrap_length,
  bool                           has_loop);

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
  void compute_peaks ();

  // Cached visual state from the item
  QColor waveform_color_;
  QColor outline_color_;
  float  canvas_width_ = 0.0f;
  float  canvas_height_ = 0.0f;

  // Content density decoupling (from ClipCanvasItemBase)
  double reference_width_ = 0;
  double reference_x_ = 0;

  // Loop wrapping (from AudioClipWaveformCanvasItem)
  bool    has_loop_ = false;
  int64_t loop_wrap_start_ = 0;  // where loop region begins in buffer
  int64_t loop_wrap_length_ = 0; // length of one loop iteration in buffer

  // Cached pointer to the item's serialized audio buffer (owned by the item)
  const juce::AudioSampleBuffer * audio_buffer_ = nullptr;

  // Precomputed per-pixel frame mapping (size = canvas_width + 1)
  std::vector<int64_t> pixel_frames_;

  // Cached peak data
  std::vector<std::vector<WaveformPeak>> peaks_;
  int                                    num_channels_ = 0;

  // Change detection
  uint64_t prev_generation_ = 0;
  float    prev_width_ = 0.0f;
  float    prev_height_ = 0.0f;
  double   prev_reference_width_ = 0;
  double   prev_reference_x_ = 0;
};

} // namespace zrythm::gui::qquick
