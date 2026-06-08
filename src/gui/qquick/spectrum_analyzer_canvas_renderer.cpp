// SPDX-FileCopyrightText: © 2025-2026 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#include <algorithm>
#include <cmath>

#include "gui/qquick/spectrum_analyzer_canvas_item.h"
#include "gui/qquick/spectrum_analyzer_canvas_renderer.h"
#include "utils/math_utils.h"
#include "utils/tracy.h"

namespace zrythm::gui::qquick
{

void
SpectrumAnalyzerCanvasRenderer::synchronize (QCanvasPainterItem * item)
{
  auto * spectrum_item = static_cast<SpectrumAnalyzerCanvasItem *> (item);

  spectrum_color_ = spectrum_item->spectrumColor ();
  sample_rate_ = spectrum_item->sampleRate ();
  canvas_width_ = static_cast<float> (spectrum_item->width ());
  canvas_height_ = static_cast<float> (spectrum_item->height ());

  const uint64_t new_gen = spectrum_item->spectrumGeneration ();
  if (new_gen != prev_generation_)
    {
      spectrum_data_ = spectrum_item->spectrumData ();
      prev_generation_ = new_gen;
    }
}

void
SpectrumAnalyzerCanvasRenderer::paint (QCanvasPainter * painter)
{
  ZoneScoped;
  const int bin_count = spectrum_data_.size ();
  if (bin_count == 0)
    return;

  const float w = canvas_width_;
  const float h = canvas_height_;
  if (w <= 0.f || h <= 0.f)
    return;

  constexpr float min_freq = 40.0f;
  const float     max_freq = sample_rate_ / 2.0f;

  painter->setRenderHint (QCanvasPainter::RenderHint::Antialiasing, true);
  painter->setFillStyle (spectrum_color_);

  for (int i = 0; i < bin_count - 1; ++i)
    {
      const float freq_pos =
        SpectrumAnalyzerCanvasItem::scaled_frequency (
          sample_rate_, i, bin_count, min_freq, max_freq)
        * w;
      const float next_freq_pos =
        SpectrumAnalyzerCanvasItem::scaled_frequency (
          sample_rate_, i + 1, bin_count, min_freq, max_freq)
        * w;

      const float power = spectrum_data_[i];
      const float next_power = spectrum_data_[i + 1];

      const float freq_delta = next_freq_pos - freq_pos;

      const int start_px = static_cast<int> (std::floor (freq_pos));
      const int end_px = static_cast<int> (std::floor (next_freq_pos));

      for (int j = start_px; j < end_px; ++j)
        {
          const float t =
            (freq_delta > 0.f)
              ? (static_cast<float> (j) - freq_pos) / freq_delta
              : 0.f;
          const float interpolated = power * (1.0f - t) + next_power * t;
          const float bar_height = h * interpolated;
          painter->fillRect (
            static_cast<float> (j), h - bar_height, 1.0f, bar_height);
        }
    }

  // Last bin
  if (bin_count > 0)
    {
      const float last_freq_pos =
        SpectrumAnalyzerCanvasItem::scaled_frequency (
          sample_rate_, bin_count - 1, bin_count, min_freq, max_freq)
        * w;
      const float last_power = spectrum_data_[bin_count - 1];
      const float last_bar_height = h * last_power;
      painter->fillRect (
        last_freq_pos, h - last_bar_height, 1.0f, last_bar_height);
    }
}

}
