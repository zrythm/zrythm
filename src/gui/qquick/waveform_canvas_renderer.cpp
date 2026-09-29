// SPDX-FileCopyrightText: © 2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cassert>
#include <ranges>

#include "gui/qquick/audio_clip_waveform_canvas_item.h"
#include "gui/qquick/waveform_canvas_item.h"
#include "gui/qquick/waveform_canvas_renderer.h"

namespace zrythm::gui::qquick
{

std::vector<int64_t>
compute_frame_mapping (
  int                             canvas_width,
  double                          reference_width,
  double                          reference_x,
  std::function<int64_t (double)> content_fraction_to_frame)
{
  if (canvas_width <= 0)
    return {};

  const double eff_width =
    (reference_width > 0) ? reference_width : canvas_width;

  std::vector<int64_t> frames (static_cast<size_t> (canvas_width) + 1);
  for (const auto px : std::views::iota (0, canvas_width + 1))
    {
      const double fraction =
        static_cast<double> (static_cast<double> (px) + reference_x) / eff_width;
      frames[static_cast<size_t> (px)] = content_fraction_to_frame (fraction);
    }
  return frames;
}

std::vector<int64_t>
compute_linear_frame_mapping (
  int     canvas_width,
  double  reference_width,
  double  reference_x,
  int64_t total_frames)
{
  return compute_frame_mapping (
    canvas_width, reference_width, reference_x, [total_frames] (double frac) {
      return static_cast<int64_t> (frac * static_cast<double> (total_frames));
    });
}

std::vector<int64_t>
compute_timeline_frame_mapping (
  int                  canvas_width,
  double               reference_width,
  double               reference_x,
  const dsp::TempoMap &tempo_map,
  dsp::TimelineTick    clip_start_tick,
  double               timeline_tick_duration)
{
  const auto clip_start_sample =
    tempo_map.tick_to_samples_rounded (clip_start_tick);

  return compute_frame_mapping (
    canvas_width, reference_width, reference_x, [&] (double fraction) {
      const auto tick = dsp::TimelineTick{ units::ticks (
        clip_start_tick.asDouble () + fraction * timeline_tick_duration) };
      const auto abs_sample = tempo_map.tick_to_samples_rounded (tick);
      return (abs_sample - clip_start_sample).in (units::samples);
    });
}

std::vector<std::vector<WaveformPeak>>
compute_waveform_peaks (
  const juce::AudioSampleBuffer &buffer,
  const std::vector<int64_t>    &pixel_frames,
  int                            canvas_width,
  int64_t                        loop_wrap_start,
  int64_t                        loop_wrap_length,
  bool                           has_loop)
{
  const int     num_channels = buffer.getNumChannels ();
  const int64_t total_frames = buffer.getNumSamples ();

  if (num_channels == 0 || total_frames == 0 || canvas_width <= 0)
    return {};

  if (static_cast<int> (pixel_frames.size ()) < canvas_width + 1)
    return {};

  // Clamp the loop region to buffer bounds. Non-looped clips may have a
  // loop_end that extends beyond the flat buffer — wrapping into that
  // region would read out of bounds.
  const int64_t eff_loop_start =
    has_loop ? std::clamp (loop_wrap_start, int64_t{ 0 }, total_frames - 1) : 0;
  const int64_t eff_loop_length =
    has_loop ? std::min (loop_wrap_length, total_frames - eff_loop_start) : 0;
  const bool can_wrap = has_loop && eff_loop_length > 0;

  std::vector<std::vector<WaveformPeak>> peaks (num_channels);
  for (auto &peak : peaks)
    peak.resize (canvas_width);

  for (const auto px : std::views::iota (0, canvas_width))
    {
      int64_t start_frame = pixel_frames[static_cast<size_t> (px)];
      int64_t end_frame = pixel_frames[static_cast<size_t> (px) + 1];

      // Left-side underflow: negative frame indices occur when reference_x is
      // negative (resize-from-start drag). Treat as silence.
      if (start_frame < 0)
        {
          for (auto &peak : peaks)
            peak[px] = { .min = 0.5f, .max = 0.5f };
          continue;
        }

      // Wrap into the loop region in the buffer if beyond the buffer end.
      if (start_frame >= total_frames && can_wrap)
        {
          const int64_t pixel_width =
            std::max (int64_t{ 1 }, end_frame - start_frame);
          const int64_t excess = start_frame - total_frames;
          start_frame = eff_loop_start + (excess % eff_loop_length);
          end_frame = std::min (
            eff_loop_start + eff_loop_length, start_frame + pixel_width);
        }
      else
        {
          if (start_frame >= total_frames)
            {
              for (auto &peak : peaks)
                peak[px] = { .min = 0.5f, .max = 0.5f };
              continue;
            }
          end_frame = std::min (end_frame, total_frames);
        }

      const int64_t count = std::max (int64_t{ 1 }, end_frame - start_frame);

      // Safety net: verify all buffer accesses are in bounds before
      // reaching juce::AudioSampleBuffer (whose assertions only log, not
      // abort, in some configurations).
      assert (start_frame >= 0 && start_frame < total_frames);
      assert (start_frame + count <= total_frames);

      for (const auto ch : std::views::iota (0, num_channels))
        {
          const float * samples =
            buffer.getReadPointer (ch, static_cast<int> (start_frame));
          const auto range = juce::FloatVectorOperations::findMinAndMax (
            samples, static_cast<int> (count));
          peaks[ch][px] = {
            .min = (std::clamp (range.getStart (), -1.0f, 1.0f) + 1.0f) * 0.5f,
            .max = (std::clamp (range.getEnd (), -1.0f, 1.0f) + 1.0f) * 0.5f,
          };
        }
    }

  return peaks;
}

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

  reference_width_ = waveform_item->effectiveReferenceWidth ();
  reference_x_ = waveform_item->referenceX ();
  const bool source_changed =
    !qFuzzyCompare (prev_reference_width_, reference_width_)
    || !qFuzzyCompare (prev_reference_x_, reference_x_);

  // Read loop info from AudioClipWaveformCanvasItem (if applicable).
  // For looped clips the buffer is loop-expanded, so wrapping goes to the
  // loop region AFTER the intro (loop_end - clip_start). For non-looped clips
  // with loopPreview (loop-resize drag), the buffer is flat, so wrapping goes
  // directly to loop_start_samples.
  const bool want_loop_wrap = waveform_item->loopPreview ();
  auto * audio_clip_item = qobject_cast<AudioClipWaveformCanvasItem *> (item);
  if (audio_clip_item != nullptr)
    {
      const bool valid_loop =
        audio_clip_item->loopEndFrame () > audio_clip_item->loopStartFrame ();
      if (audio_clip_item->hasLoop ())
        {
          has_loop_ = valid_loop;
          loop_wrap_start_ = audio_clip_item->loopRegionBufferStart ();
        }
      else if (want_loop_wrap && valid_loop)
        {
          has_loop_ = true;
          loop_wrap_start_ = audio_clip_item->loopStartFrame ();
        }
      else
        {
          has_loop_ = false;
        }
      loop_wrap_length_ =
        audio_clip_item->loopEndFrame () - audio_clip_item->loopStartFrame ();
    }
  else
    {
      has_loop_ = false;
      loop_wrap_start_ = 0;
      loop_wrap_length_ = 0;
    }

  // The raw pointer into the item's audio_buffer_ is valid only for the
  // duration of synchronize() (the GUI thread is blocked during this call).
  // compute_peaks() below copies the needed data into peaks_, so paint()
  // never dereferences this pointer — do not read audio_buffer_ from paint().
  waveform_color_ = waveform_item->waveformColor ();
  outline_color_ = waveform_item->outlineColor ();
  audio_buffer_ = waveform_item->audioBuffer ();

  // Compute per-pixel frame mapping. For audio clips, use non-linear mapping
  // that follows the tempo map's tick-to-sample conversion so the waveform
  // visually aligns with what plays at each timeline position. For other
  // waveform items (e.g. live viewer), use simple linear mapping.
  const int canvas_width_int = static_cast<int> (new_width);
  if (audio_clip_item != nullptr)
    {
      pixel_frames_ = audio_clip_item->computeTimelineFrameMapping (
        canvas_width_int, reference_width_, reference_x_);
    }
  else
    {
      pixel_frames_ = compute_linear_frame_mapping (
        canvas_width_int, reference_width_, reference_x_,
        audio_buffer_ ? audio_buffer_->getNumSamples () : 0);
    }

  prev_generation_ = new_generation;
  prev_width_ = new_width;
  prev_height_ = new_height;
  canvas_width_ = new_width;
  canvas_height_ = new_height;
  prev_reference_width_ = reference_width_;
  prev_reference_x_ = reference_x_;

  if (buffer_changed || size_changed || source_changed)
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

  peaks_ = compute_waveform_peaks (
    *audio_buffer_, pixel_frames_, static_cast<int> (canvas_width_),
    loop_wrap_start_, loop_wrap_length_, has_loop_);
  num_channels_ = static_cast<int> (peaks_.size ());
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
