// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <ranges>

#include "gui/qquick/waveform_canvas_item.h"
#include "gui/qquick/waveform_canvas_renderer.h"

namespace zrythm::gui::qquick
{

void
WaveformCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * waveform_item = static_cast<WaveformCanvasItem *> (item);

  const float new_width = static_cast<float> (waveform_item->width ());
  const float new_height = static_cast<float> (waveform_item->height ());

  const bool size_changed =
    !qFuzzyCompare (prev_width_, new_width)
    || !qFuzzyCompare (prev_height_, new_height);
  const uint64_t new_generation = waveform_item->bufferGeneration ();
  const bool     buffer_changed = (new_generation != prev_generation_);

  // Qt Scene Graph blocks the GUI thread during synchronize(), so the raw
  // pointer into the item's audio_buffer_ cannot be invalidated between
  // here and paint().
  waveform_color_ = waveform_item->waveformColor ();
  outline_color_ = waveform_item->outlineColor ();
  audio_buffer_ = waveform_item->audioBuffer ();

  prev_generation_ = new_generation;
  prev_width_ = new_width;
  prev_height_ = new_height;
  canvas_width_ = new_width;
  canvas_height_ = new_height;

  if (buffer_changed || size_changed)
    {
      compute_peaks ();
    }
}

void
WaveformCanvasRenderer::compute_peaks ()
{
  if (audio_buffer_ == nullptr)
    {
      peaks_.clear ();
      num_channels_ = 0;
      return;
    }

  const auto *  buffer = audio_buffer_;
  const int     num_channels = buffer->getNumChannels ();
  const int64_t total_frames = buffer->getNumSamples ();

  if (num_channels == 0 || total_frames == 0)
    {
      peaks_.clear ();
      num_channels_ = 0;
      return;
    }

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
