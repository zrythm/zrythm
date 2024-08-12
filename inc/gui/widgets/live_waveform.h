// SPDX-FileCopyrightText: © 2019, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_LIVE_WAVEFORM_H__
#define __GUI_WIDGETS_LIVE_WAVEFORM_H__

/**
 * @file
 *
 * Live waveform display like LMMS.
 */

#include "gtk_wrapper.h"

#define LIVE_WAVEFORM_WIDGET_TYPE (live_waveform_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  LiveWaveformWidget,
  live_waveform_widget,
  Z,
  LIVE_WAVEFORM_WIDGET,
  GtkDrawingArea)

class AudioPort;

/**
 * @addtogroup widgets
 *
 * @{
 */

enum class LiveWaveformType
{
  LIVE_WAVEFORM_ENGINE,
  LIVE_WAVEFORM_PORT,
};

using LiveWaveformWidget = struct _LiveWaveformWidget
{
  GtkDrawingArea parent_instance;

  LiveWaveformType type;

  /** Draw border or not. */
  int draw_border;

  float * bufs[2];

  /** Current buffer sizes. */
  size_t buf_sz[2];

  /** Port, if port. */
  AudioPort * port;
};

/**
 * Creates a LiveWaveformWidget for the AudioEngine.
 */
void
live_waveform_widget_setup_engine (LiveWaveformWidget * self);

/**
 * Creates a LiveWaveformWidget for a port.
 */
LiveWaveformWidget *
live_waveform_widget_new_port (AudioPort * port);

/**
 * @}
 */

#endif
