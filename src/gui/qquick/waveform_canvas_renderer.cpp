// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "gui/qquick/waveform_canvas_item.h"
#include "gui/qquick/waveform_canvas_renderer.h"

namespace zrythm::gui::qquick
{

WaveformCanvasRenderer::WaveformCanvasRenderer () = default;
WaveformCanvasRenderer::~WaveformCanvasRenderer () = default;

void
WaveformCanvasRenderer::initializeResources (QCanvasPainter * painter)
{
  Q_UNUSED (painter)
}

void
WaveformCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * waveform_item = static_cast<WaveformCanvasItem *> (item);

  canvas_width_ = static_cast<float> (waveform_item->width ());
  canvas_height_ = static_cast<float> (waveform_item->height ());

  const bool region_changed = (region_ != waveform_item->region ());
  const bool zoom_changed =
    !qFuzzyCompare (px_per_tick_, waveform_item->pxPerTick ());
  const bool size_changed =
    !qFuzzyCompare (static_cast<qreal> (canvas_width_), waveform_item->width ())
    || !qFuzzyCompare (
      static_cast<qreal> (canvas_height_), waveform_item->height ());

  region_ = waveform_item->region ();
  px_per_tick_ = waveform_item->pxPerTick ();
  waveform_color_ = waveform_item->waveformColor ();
  outline_color_ = waveform_item->outlineColor ();
  audio_buffer_ = waveform_item->audioBuffer ();

  if (region_changed || zoom_changed || size_changed)
    {
      needs_recompute_ = true;
    }

  if (needs_recompute_ && region_ != nullptr && px_per_tick_ > 0)
    {
      compute_peaks ();
      needs_recompute_ = false;
    }
}

void
WaveformCanvasRenderer::compute_peaks ()
{
  if (audio_buffer_ == nullptr)
    return;

  const auto *  buffer = audio_buffer_;
  const int     num_channels = buffer->getNumChannels ();
  const int64_t total_frames = buffer->getNumSamples ();

  if (num_channels == 0 || total_frames == 0)
    return;

  num_channels_ = num_channels;

  const int total_px = static_cast<int> (canvas_width_);
  if (total_px <= 0)
    return;

  const float samples_per_pixel =
    static_cast<float> (total_frames) / static_cast<float> (total_px);

  // Resize peaks storage
  peaks_.resize (num_channels);
  for (auto &peak : peaks_)
    peak.resize (total_px);

  for (const auto px : std::views::iota (0, total_px))
    {
      const int64_t start_frame =
        static_cast<int64_t> (static_cast<float> (px) * samples_per_pixel);
      const int64_t end_frame = std::min (
        static_cast<int64_t> (static_cast<float> (px + 1) * samples_per_pixel),
        total_frames);

      if (start_frame >= total_frames)
        {
          for (auto &peak : peaks_)
            peak[px] = { .min = 0.5f, .max = 0.5f };
          continue;
        }

      const int64_t count = std::max (int64_t{ 1 }, end_frame - start_frame);

      for (const auto ch : std::views::iota (0, num_channels))
        {
          const float * samples =
            buffer->getReadPointer (ch, static_cast<int> (start_frame));
          const auto range = juce::FloatVectorOperations::findMinAndMax (
            samples, static_cast<int> (count));
          // Map from [-1, 1] to [0, 1] for normalized drawing
          peaks_[ch][px] = {
            .min = (std::clamp (range.getStart (), -1.0f, 1.0f) + 1.0f) * 0.5f,
            .max = (std::clamp (range.getEnd (), -1.0f, 1.0f) + 1.0f) * 0.5f,
          };
        }
    }
}

void
WaveformCanvasRenderer::paint (QCanvasPainter * painter)
{
  if (peaks_.empty () || num_channels_ == 0)
    return;

  const float w = canvas_width_;
  const float h = canvas_height_;
  const float channel_height = h / static_cast<float> (num_channels_);
  const int   num_steps = static_cast<int> (peaks_[0].size ());
  const float px_step = w / static_cast<float> (num_steps);

  painter->setFillStyle (waveform_color_);
  painter->setStrokeStyle (outline_color_);
  painter->setLineWidth (1.0f);
  painter->setRenderHint (QCanvasPainter::RenderHint::Antialiasing, false);

  for (const auto ch : std::views::iota (0, num_channels_))
    {
      painter->save ();
      painter->translate (0.0f, static_cast<float> (ch) * channel_height);

      painter->beginPath ();

      const auto &ch_peaks = peaks_.at (ch);

      // Upper envelope (max values) left-to-right
      for (const auto step : std::views::iota (0, num_steps))
        {
          const float px = static_cast<float> (step) * px_step;
          const float y = (1.0f - ch_peaks[step].max) * channel_height;
          if (step == 0)
            painter->moveTo (px, y);
          else
            painter->lineTo (px, y);
        }

      // Lower envelope (min values) right-to-left to close the shape
      for (
        const auto step : std::views::iota (0, num_steps) | std::views::reverse)
        {
          const float px = static_cast<float> (step) * px_step;
          const float y = (1.0f - ch_peaks[step].min) * channel_height;
          painter->lineTo (px, y);
        }

      painter->closePath ();
      painter->fill ();
      painter->stroke ();

      painter->restore ();
    }
}

} // namespace zrythm::gui::qquick
