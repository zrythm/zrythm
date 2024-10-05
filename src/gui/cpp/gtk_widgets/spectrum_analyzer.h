// SPDX-FileCopyrightText: © 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_SPECTRUM_ANALYZER_H__
#define __GUI_WIDGETS_SPECTRUM_ANALYZER_H__

#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "juce/juce.h"
#include <kiss_fft.h>

class Port;
class PeakFallSmooth;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define SPECTRUM_ANALYZER_WIDGET_TYPE (spectrum_analyzer_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  SpectrumAnalyzerWidget,
  spectrum_analyzer_widget,
  Z,
  SPECTRUM_ANALYZER_WIDGET,
  GtkWidget)

#define SPECTRUM_ANALYZER_MAX_BLOCK_SIZE 32768
#define SPECTRUM_ANALYZER_MIN_FREQ 20.f

/**
 * @brief Spectrum analyzer.
 */
using SpectrumAnalyzerWidget = struct _SpectrumAnalyzerWidget
{
  GtkWidget parent_instance;

  /** Port to read buffers from. */
  Port * port;

  std::unique_ptr<juce::AudioSampleBuffer> buffer;

  kiss_fft_cpx * fft_in;
  kiss_fft_cpx * fft_out;

  kiss_fft_cfg fft_config;
  int          last_block_size;

  std::vector<PeakFallSmooth> bins;
};

/**
 * Creates a spectrum analyzer for the AudioEngine.
 */
void
spectrum_analyzer_widget_setup_engine (SpectrumAnalyzerWidget * self);

SpectrumAnalyzerWidget *
spectrum_analyzer_widget_new_for_port (Port * port);

/**
 * @}
 */

#endif
