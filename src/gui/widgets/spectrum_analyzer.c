// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
/*
 * This file incorporates work covered by the following copyright and
 * permission notices:
 *
 * ---
 *
 * SPDX-FileCopyrightText: © Patrick Desaulniers
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * ---
 */

/** \file
 */

#include "zrythm-config.h"

#include <stdio.h>

#include "audio/engine.h"
#include "audio/master_track.h"
#include "audio/peak_fall_smooth.h"
#include "gui/widgets/bot_bar.h"
#include "gui/widgets/spectrum_analyzer.h"
#include "project.h"
#include "utils/arrays.h"
#include "utils/dsp.h"
#include "utils/gtk.h"
#include "utils/objects.h"
#include "utils/ui.h"
#include "zrythm_app.h"

#include <zix/ring.h>

#define BUF_SIZE 65000

#define BLOCK_SIZE 256

static const float threshold = -90.f;

G_DEFINE_TYPE (
  SpectrumAnalyzerWidget,
  spectrum_analyzer_widget,
  GTK_TYPE_WIDGET)

static float
windowHanning (int i, int transformSize)
{
  return (
    0.5f
    * (1.0f - cosf (2.0f * M_PI * (float) i / (float) (transformSize - 1))));
}

static float
getPowerSpectrumdB (
  SpectrumAnalyzerWidget * self,
  kiss_fft_cpx *           out,
  const int                index,
  const int                transformSize)
{
  const float real = out[index].r * (2.0 / transformSize);
  const float complex = out[index].i * (2.0 / transformSize);

  const float powerSpectrum = real * real + complex * complex;
  float       powerSpectrumdB =
    10.0f / logf (10.0f) * logf (powerSpectrum + 1e-9);

  if (powerSpectrumdB <= threshold)
    {
      powerSpectrumdB = -90.0f;
    }
  else
    {
      //powerSpectrumdB = wolf::lerp(-90.0f, 0.0f, 1.0f - (powerSpectrumdB + threshold) / -90.0f);
    }

  // Normalize values
  powerSpectrumdB = 1.0 - powerSpectrumdB / -90.0;

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

  const float b = logf (max / min) / (max - min);
  const float a = max / expf (max * b);

  return logf (value / a) / b;
}

static float
getBinPos (
  const int   bin,
  const int   numBins,
  const float sampleRate)
{
  const float maxFreq = sampleRate / 2;
  const float hzPerBin = maxFreq / numBins;

  const float freq = hzPerBin * bin;
  const float scaledFreq =
    invLogScale (freq + 1, SPECTRUM_ANALYZER_MIN_FREQ, maxFreq)
    - 1;

  return numBins * scaledFreq / maxFreq;
}

static float
lerp (float a, float b, float f)
{
  f = CLAMP (f, 0.0f, 1.0f);

  return a * (1.0 - f) + (b * f);
}

static float
getBinPixelColor (const float powerSpectrumdB)
{
  const float euler = expf (1.0f);
  const float scaledSpectrum =
    (expf (powerSpectrumdB) - 1) / (euler - 1);

  float dB = -90.f + 90.f * scaledSpectrum;
  dB = CLAMP (dB, -90, 0);
#if 0
    dB = fabsf(dB);
#endif

  float amplitude = math_dbfs_to_fader_val (dB);

  return amplitude;
}

static void
spectrum_analyzer_snapshot (
  GtkWidget *   widget,
  GtkSnapshot * snapshot)
{
  SpectrumAnalyzerWidget * self =
    Z_SPECTRUM_ANALYZER_WIDGET (widget);

  int width = gtk_widget_get_allocated_width (widget);
  int height = gtk_widget_get_allocated_height (widget);

  GtkStyleContext * context =
    gtk_widget_get_style_context (widget);

  gtk_snapshot_render_background (
    snapshot, context, 0, 0, width, height);

  size_t   block_size = AUDIO_ENGINE->block_length;
  uint32_t block_size_in_bytes =
    sizeof (float) * (uint32_t) block_size;

  Port * port = NULL;
  g_return_if_fail (IS_TRACK_AND_NONNULL (P_MASTER_TRACK));
  if (!P_MASTER_TRACK->channel->stereo_out->l->write_ring_buffers)
    {
      P_MASTER_TRACK->channel->stereo_out->l
        ->write_ring_buffers = true;
      P_MASTER_TRACK->channel->stereo_out->r
        ->write_ring_buffers = true;
      return;
    }
  port = P_MASTER_TRACK->channel->stereo_out->l;

  g_return_if_fail (IS_PORT_AND_NONNULL (port));

  /* if ring not ready yet skip draw */
  if (!port->audio_ring)
    return;

  /* get the L buffer */
  uint32_t read_space_avail =
    zix_ring_read_space (port->audio_ring);
  uint32_t blocks_to_read =
    block_size_in_bytes == 0
      ? 0
      : read_space_avail / block_size_in_bytes;
  /* if buffer is not filled do not draw */
  if (blocks_to_read <= 0)
    return;

  while (read_space_avail > self->buf_sz[0])
    {
      array_double_size_if_full (
        self->bufs[0], self->buf_sz[0], self->buf_sz[0],
        float);
    }
  uint32_t lblocks_read = zix_ring_peek (
    port->audio_ring, &(self->bufs[0][0]), read_space_avail);
  lblocks_read /= block_size_in_bytes;
  uint32_t lstart_index = (lblocks_read - 1) * block_size;
  if (lblocks_read == 0)
    {
      return;
      /*g_return_val_if_reached (FALSE);*/
    }

  /* get the R buffer */
  port = P_MASTER_TRACK->channel->stereo_out->r;
  read_space_avail = zix_ring_read_space (port->audio_ring);
  blocks_to_read = read_space_avail / block_size_in_bytes;

  /* if buffer is not filled do not draw */
  if (blocks_to_read <= 0)
    return;

  while (read_space_avail > self->buf_sz[1])
    {
      array_double_size_if_full (
        self->bufs[1], self->buf_sz[1], self->buf_sz[1],
        float);
    }
  size_t rblocks_read = zix_ring_peek (
    port->audio_ring, &(self->bufs[1][0]), read_space_avail);
  rblocks_read /= block_size_in_bytes;
  size_t rstart_index = (rblocks_read - 1) * block_size;
  if (rblocks_read == 0)
    {
      return;
      /*g_return_val_if_reached (FALSE);*/
    }

  dsp_make_mono (
    &self->bufs[0][lstart_index],
    &self->bufs[1][rstart_index], block_size, true);
  const float * mono_buf = &self->bufs[0][lstart_index];

  /* --- process --- */

  if ((int) block_size != self->last_block_size)
    {
      kiss_fft_free (self->fft_config);
      self->fft_config =
        kiss_fft_alloc ((int) block_size, 0, NULL, NULL);
      for (size_t i = 0; i < block_size; i++)
        {
          peak_fall_smooth_calculate_coeff (
            self->bins[i],
            (float) AUDIO_ENGINE->sample_rate / 64.f,
            (float) AUDIO_ENGINE->sample_rate);
        }
    }

  int stepSize = block_size / 2;
  int half = block_size / 2;

  const float scaleX =
    (float) width / (float) block_size * 2.f;
  gtk_snapshot_scale (snapshot, scaleX, 1.f);

  // samples to throw away
  for (int i = 0; i < stepSize; ++i)
    {
      self->fft_in[i].r =
        mono_buf[i] * windowHanning (i, block_size);
      self->fft_in[i].i = 0;
    }

  // samples to keep
  for (int i = stepSize; i < stepSize * 2; ++i)
    {
      self->fft_in[i].r =
        mono_buf[i - stepSize] * windowHanning (i, block_size);
      self->fft_in[i].i = 0;
    }

  kiss_fft (self->fft_config, self->fft_in, self->fft_out);

  for (int i = 0; i < half; ++i)
    {
      peak_fall_smooth_set_value (
        self->bins[i],
        getPowerSpectrumdB (
          self, self->fft_out, i, block_size));
    }

  /* --- end process --- */

  /*g_message ("executing");*/
  /*fftwf_execute (zrythm_app->spectrum_analyzer_plan);*/

  GdkRGBA color_green;
  /*GdkRGBA color_red;*/
  gdk_rgba_parse (&color_green, "#11FF44");
  /*gdk_rgba_parse (&color_red, "#ff0F44");*/

  for (int i = 0; i < half; ++i)
    {
      const float powerSpectrumdB =
        peak_fall_smooth_get_smoothed_value (self->bins[i]);

      /*Color pixelColor = getBinPixelColor(powerSpectrumdB);*/
      float amp = getBinPixelColor (powerSpectrumdB);

      const int freqSize = 1;
      float     freqPos = i * freqSize;

      if (i < half - 1) // must interpolate to fill the gaps
        {
          const float nextPowerSpectrumdB =
            peak_fall_smooth_get_smoothed_value (
              self->bins[i + 1]);

          freqPos = (int) getBinPos (
            i, half, AUDIO_ENGINE->sample_rate);
          const int nextFreqPos = (int) getBinPos (
            i + 1, half, AUDIO_ENGINE->sample_rate);

          const int freqDelta =
            (int) nextFreqPos - (int) freqPos;

          for (int j = (int) freqPos; j < (int) nextFreqPos;
               ++j)
            {
              (void) freqDelta;
              (void) nextPowerSpectrumdB;
              (void) lerp;
              float lerped_amt = getBinPixelColor (lerp (
                powerSpectrumdB, nextPowerSpectrumdB,
                (j - freqPos) / (float) freqDelta));
              /*fScrollingTexture.drawPixelOnCurrentLine(j, lerpedColor);*/
              gtk_snapshot_append_color (
                snapshot, &color_green,
                &GRAPHENE_RECT_INIT (
                  (float) j, height - 2, 1,
                  -(float) height * lerped_amt));
            }
        }

      gtk_snapshot_append_color (
        snapshot, &color_green,
        &GRAPHENE_RECT_INIT (
          (float) freqPos, height - 2, 1,
          -(float) height * amp));

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
spectrum_analyzer_widget_setup_engine (
  SpectrumAnalyzerWidget * self)
{
}

SpectrumAnalyzerWidget *
spectrum_analyzer_widget_new_for_port (Port * port)
{
  SpectrumAnalyzerWidget * self =
    g_object_new (SPECTRUM_ANALYZER_WIDGET_TYPE, NULL);

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
  for (int i = 0; i < SPECTRUM_ANALYZER_MAX_BLOCK_SIZE / 2; i++)
    {
      object_free_w_func_and_null (
        peak_fall_smooth_free, self->bins[i]);
    }
  free (self->bins);

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
  self->fft_config =
    kiss_fft_alloc (BLOCK_SIZE, 0, NULL, NULL);
  self->fft_in = object_new_n (
    SPECTRUM_ANALYZER_MAX_BLOCK_SIZE, kiss_fft_cpx);
  self->fft_out = object_new_n (
    SPECTRUM_ANALYZER_MAX_BLOCK_SIZE, kiss_fft_cpx);
  self->bins = object_new_n (
    SPECTRUM_ANALYZER_MAX_BLOCK_SIZE / 2, PeakFallSmooth *);
  for (int i = 0; i < SPECTRUM_ANALYZER_MAX_BLOCK_SIZE / 2; i++)
    {
      self->bins[i] = peak_fall_smooth_new ();
    }

  gtk_widget_add_tick_callback (
    GTK_WIDGET (self), (GtkTickCallback) update_activity,
    self, NULL);
  gtk_widget_add_css_class (
    GTK_WIDGET (self), "signal-preview");
  gtk_widget_set_overflow (
    GTK_WIDGET (self), GTK_OVERFLOW_HIDDEN);
}

static void
spectrum_analyzer_widget_class_init (
  SpectrumAnalyzerWidgetClass * klass)
{
  GtkWidgetClass * wklass = GTK_WIDGET_CLASS (klass);
  wklass->snapshot = spectrum_analyzer_snapshot;
  gtk_widget_class_set_css_name (wklass, "spectrum-analyzer");

  GObjectClass * oklass = G_OBJECT_CLASS (klass);
  oklass->finalize = (GObjectFinalizeFunc) finalize;
}
