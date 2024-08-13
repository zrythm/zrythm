// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
 * SPDX-FileCopyrightText: © 2023 Patrick Desaulniers
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ---
 */

/** @file
 */

#include "zrythm-config.h"

#include "dsp/engine.h"
#include "dsp/master_track.h"
#include "dsp/peak_fall_smooth.h"
#include "dsp/tracklist.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/spectrum_analyzer.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

// Buffer size for spectrum analyzer
constexpr size_t BUF_SIZE = 65000;

// Block size for processing
constexpr size_t BLOCK_SIZE = 256;

// Threshold for power spectrum in dB
constexpr float threshold = -90.f;

G_DEFINE_TYPE (SpectrumAnalyzerWidget, spectrum_analyzer_widget, GTK_TYPE_WIDGET)

static float
windowHanning (int i, int transformSize)
{
  return (
    0.5f
    * (1.0f - cosf (2.0f * (float) M_PI * (float) i / (float) (transformSize - 1))));
}

static float
getPowerSpectrumdB (
  SpectrumAnalyzerWidget * self,
  kiss_fft_cpx *           out,
  const int                index,
  const int                transformSize)
{
  const float real = (float) out[index].r * (2.f / transformSize);
  const float complex = (float) out[index].i * (2.f / transformSize);

  const float powerSpectrum = real * real + complex * complex;
  float       powerSpectrumdB =
    10.0f / math_fast_log (10.0f) * math_fast_log (powerSpectrum + 1e-9f);

  if (powerSpectrumdB <= threshold)
    {
      powerSpectrumdB = -90.0f;
    }
  else
    {
      // powerSpectrumdB = wolf::lerp(-90.0f, 0.0f, 1.0f - (powerSpectrumdB +
      // threshold) / -90.0f);
    }

  // Normalize values
  powerSpectrumdB = 1.f - powerSpectrumdB / -90.f;

  if (powerSpectrumdB > 1)
    {
      powerSpectrumdB = 1;
    }

  return powerSpectrumdB;
}

static float
invLogScale (const float value, const float min, const float max)
{
  if (value < min)
    return min;
  if (value > max)
    return max;

  const float b = math_fast_log (max / min) / (max - min);
  const float a = max / expf (max * b);

  return math_fast_log (value / a) / b;
}

static float
getBinPos (const int bin, const int numBins, const float sampleRate)
{
  const float maxFreq = sampleRate / 2;
  const float hzPerBin = maxFreq / numBins;

  const float freq = hzPerBin * bin;
  const float scaledFreq =
    invLogScale (freq + 1, SPECTRUM_ANALYZER_MIN_FREQ, maxFreq) - 1;

  return numBins * scaledFreq / maxFreq;
}

static float
_lerp (float a, float b, float f)
{
  f = std::clamp<float> (f, 0.0f, 1.0f);

  return a * (1.f - f) + (b * f);
}

static float
getBinPixelColor (const float powerSpectrumdB)
{
  const float euler = expf (1.0f);
  const float scaledSpectrum = (expf (powerSpectrumdB) - 1) / (euler - 1);

  float dB = -90.f + 90.f * scaledSpectrum;
  dB = CLAMP (dB, -90, 0);
#if 0
    dB = fabsf(dB);
#endif

  float amplitude = math_dbfs_to_fader_val (dB);

  return amplitude;
}

static void
spectrum_analyzer_snapshot (GtkWidget * widget, GtkSnapshot * snapshot)
{
  SpectrumAnalyzerWidget * self = Z_SPECTRUM_ANALYZER_WIDGET (widget);

  int width = gtk_widget_get_width (widget);
  int height = gtk_widget_get_height (widget);

  size_t   block_size = AUDIO_ENGINE->block_length_;
  uint32_t block_size_in_bytes = sizeof (float) * (uint32_t) block_size;

  z_return_if_fail (IS_TRACK_AND_NONNULL (P_MASTER_TRACK));
  if (!P_MASTER_TRACK->channel_->stereo_out_->get_l ().write_ring_buffers_)
    {
      P_MASTER_TRACK->channel_->stereo_out_->set_write_ring_buffers (true);
      return;
    }
  auto &lport = P_MASTER_TRACK->channel_->stereo_out_->get_l ();

  /* if ring not ready yet skip draw */
  // if (!lport.audio_ring_)
  //   return;

  /* get the L buffer */
  uint32_t read_space_avail = lport.audio_ring_->read_space ();
  uint32_t blocks_to_read =
    block_size_in_bytes == 0 ? 0 : read_space_avail / block_size_in_bytes;
  /* if buffer is not filled do not draw */
  if (blocks_to_read <= 0)
    return;

  while (read_space_avail > self->buf_sz[0])
    {
      array_double_size_if_full (
        self->bufs[0], self->buf_sz[0], self->buf_sz[0], float);
    }
  uint32_t lblocks_read =
    lport.audio_ring_->peek_multiple (&(self->bufs[0][0]), read_space_avail);
  lblocks_read /= block_size_in_bytes;
  uint32_t lstart_index = (lblocks_read - 1) * block_size;
  if (lblocks_read == 0)
    {
      return;
      /*z_return_val_if_reached (FALSE);*/
    }

  /* get the R buffer */
  auto &rport = P_MASTER_TRACK->channel_->stereo_out_->get_r ();
  read_space_avail = rport.audio_ring_->read_space ();
  blocks_to_read = read_space_avail / block_size_in_bytes;

  /* if buffer is not filled do not draw */
  if (blocks_to_read <= 0)
    return;

  while (read_space_avail > self->buf_sz[1])
    {
      array_double_size_if_full (
        self->bufs[1], self->buf_sz[1], self->buf_sz[1], float);
    }
  size_t rblocks_read =
    rport.audio_ring_->peek_multiple (&(self->bufs[1][0]), read_space_avail);
  rblocks_read /= block_size_in_bytes;
  size_t rstart_index = (rblocks_read - 1) * block_size;
  if (rblocks_read == 0)
    {
      return;
      /*z_return_val_if_reached (FALSE);*/
    }

  dsp_make_mono (
    &self->bufs[0][lstart_index], &self->bufs[1][rstart_index], block_size,
    true);
  const float * mono_buf = &self->bufs[0][lstart_index];

  /* --- process --- */

  if ((int) block_size != self->last_block_size)
    {
      kiss_fft_free (self->fft_config);
      self->fft_config = kiss_fft_alloc ((int) block_size, 0, nullptr, nullptr);
      for (size_t i = 0; i < block_size; i++)
        {
          z_return_if_fail (block_size <= SPECTRUM_ANALYZER_MAX_BLOCK_SIZE / 2);
          self->bins[i].calculate_coeff (
            (float) AUDIO_ENGINE->sample_rate_ / 64.f,
            (float) AUDIO_ENGINE->sample_rate_);
        }
    }

  int stepSize = block_size / 2;
  int half = block_size / 2;

  const float scaleX = (float) width / (float) block_size * 2.f;
  gtk_snapshot_scale (snapshot, scaleX, 1.f);

  // samples to throw away
  for (int i = 0; i < stepSize; ++i)
    {
      self->fft_in[i].r = mono_buf[i] * windowHanning (i, block_size);
      self->fft_in[i].i = 0;
    }

  // samples to keep
  for (int i = stepSize; i < stepSize * 2; ++i)
    {
      self->fft_in[i].r = mono_buf[i - stepSize] * windowHanning (i, block_size);
      self->fft_in[i].i = 0;
    }

  kiss_fft (self->fft_config, self->fft_in, self->fft_out);

  for (int i = 0; i < half; ++i)
    {
      self->bins[i].set_value (
        getPowerSpectrumdB (self, self->fft_out, i, block_size));
    }

  /* --- end process --- */

  /*z_info ("executing");*/
  /*fftwf_execute (zrythm_app->spectrum_analyzer_plan);*/

  GdkRGBA color_green;
  gtk_widget_get_color (widget, &color_green);

  for (int i = 0; i < half; ++i)
    {
      const float powerSpectrumdB = self->bins[i].get_smoothed_value ();

      /*Color pixelColor = getBinPixelColor(powerSpectrumdB);*/
      float amp = getBinPixelColor (powerSpectrumdB);

      const int freqSize = 1;
      float     freqPos = i * freqSize;

      if (i < half - 1) // must interpolate to fill the gaps
        {
          const float nextPowerSpectrumdB =
            self->bins[i + 1].get_smoothed_value ();

          freqPos = (int) getBinPos (i, half, AUDIO_ENGINE->sample_rate_);
          const int nextFreqPos =
            (int) getBinPos (i + 1, half, AUDIO_ENGINE->sample_rate_);

          const int freqDelta = (int) nextFreqPos - (int) freqPos;

          for (int j = (int) freqPos; j < (int) nextFreqPos; ++j)
            {
              (void) freqDelta;
              (void) nextPowerSpectrumdB;
              (void) _lerp;
              float lerped_amt = getBinPixelColor (_lerp (
                powerSpectrumdB, nextPowerSpectrumdB,
                (j - freqPos) / (float) freqDelta));
              /*fScrollingTexture.drawPixelOnCurrentLine(j, lerpedColor);*/
              const float     px_amt = (float) height * lerped_amt;
              graphene_rect_t draw_rect = Z_GRAPHENE_RECT_INIT (
                (float) j, (float) height - px_amt, 1, px_amt);
              gtk_snapshot_append_color (snapshot, &color_green, &draw_rect);
            }
        }

      const float     px_amt = (float) height * amp;
      graphene_rect_t draw_rect = Z_GRAPHENE_RECT_INIT (
        (float) freqPos, (float) height - px_amt, 1, px_amt);
      gtk_snapshot_append_color (snapshot, &color_green, &draw_rect);

      /*fScrollingTexture.drawPixelOnCurrentLine(freqPos, pixelColor);*/
    }

  self->last_block_size = block_size;
}

static int
update_activity (
  GtkWidget *              widget,
  GdkFrameClock *          frame_clock,
  SpectrumAnalyzerWidget * self)
{
  gtk_widget_queue_draw (widget);

  return G_SOURCE_CONTINUE;
}

/**
 * Creates a spectrum analyzer for the AudioEngine.
 */
void
spectrum_analyzer_widget_setup_engine (SpectrumAnalyzerWidget * self)
{
}

SpectrumAnalyzerWidget *
spectrum_analyzer_widget_new_for_port (Port * port)
{
  SpectrumAnalyzerWidget * self = Z_SPECTRUM_ANALYZER_WIDGET (
    g_object_new (SPECTRUM_ANALYZER_WIDGET_TYPE, nullptr));

  self->port = port;

  return self;
}

static void
finalize (SpectrumAnalyzerWidget * self)
{
  object_zero_and_free (self->bufs[0]);
  object_zero_and_free (self->bufs[1]);
  object_zero_and_free (self->fft_in);
  object_zero_and_free (self->fft_out);
  std::destroy_at (&self->bins);

  kiss_fft_free (self->fft_config);

  G_OBJECT_CLASS (spectrum_analyzer_widget_parent_class)
    ->finalize (G_OBJECT (self));
}

static void
spectrum_analyzer_widget_init (SpectrumAnalyzerWidget * self)
{
  self->bufs[0] = object_new_n (BUF_SIZE, float);
  self->bufs[1] = object_new_n (BUF_SIZE, float);
  self->buf_sz[0] = BUF_SIZE;
  self->buf_sz[1] = BUF_SIZE;
  self->fft_config = kiss_fft_alloc (BLOCK_SIZE, 0, nullptr, nullptr);
  self->fft_in = object_new_n (SPECTRUM_ANALYZER_MAX_BLOCK_SIZE, kiss_fft_cpx);
  self->fft_out = object_new_n (SPECTRUM_ANALYZER_MAX_BLOCK_SIZE, kiss_fft_cpx);

  std::construct_at (&self->bins);
  for (int i = 0; i < SPECTRUM_ANALYZER_MAX_BLOCK_SIZE / 2; ++i)
    {
      self->bins.emplace_back ();
    }

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) update_activity, self, nullptr);
  gtk_widget_set_overflow (GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

static void
spectrum_analyzer_widget_class_init (SpectrumAnalyzerWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = spectrum_analyzer_snapshot;
  gtk_widget_class_set_css_name (wklass, "spectrum-analyzer");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
