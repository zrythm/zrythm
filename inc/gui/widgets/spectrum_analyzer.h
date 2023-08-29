// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * SpectrumAnalyzer widget.
 */

#ifndef __GUI_WIDGETS_SPECTRUM_ANALYZER_H__
#define __GUI_WIDGETS_SPECTRUM_ANALYZER_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#include <kiss_fft.h>

TYPEDEF_STRUCT (Port);
TYPEDEF_STRUCT (PeakFallSmooth);

/**
 * @addtogroup widgets
 *
 * @{
 */

#define SPECTRUM_ANALYZER_WIDGET_TYPE \
  (spectrum_analyzer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SpectrumAnalyzerWidget,
  spectrum_analyzer_widget,
  Z,
  SPECTRUM_ANALYZER_WIDGET,
  GtkWidget)

#define SPECTRUM_ANALYZER_MAX_BLOCK_SIZE 32768
#define SPECTRUM_ANALYZER_MIN_FREQ 20.f

typedef struct _SpectrumAnalyzerWidget
{
  GtkWidget parent_instance;

  /** Port to read buffers from. */
  Port * port;

  float * bufs[2];

  /** Current buffer sizes. */
  size_t buf_sz[2];

  kiss_fft_cpx * fft_in;
  kiss_fft_cpx * fft_out;

  kiss_fft_cfg fft_config;
  int          last_block_size;

  PeakFallSmooth ** bins;

} SpectrumAnalyzerWidget;

/**
 * Creates a spectrum analyzer for the AudioEngine.
 */
void
spectrum_analyzer_widget_setup_engine (
  SpectrumAnalyzerWidget * self);

SpectrumAnalyzerWidget *
spectrum_analyzer_widget_new_for_port (Port * port);

/**
 * @}
 */

#endif
