// SPDX-FileCopyrightText: © 2019, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_LIVE_WAVEFORM_H__
#define __GUI_WIDGETS_LIVE_WAVEFORM_H__

/**
 * \file
 *
 * Live waveform display like LMMS.
 */

#include <gtk/gtk.h>

#define LIVE_WAVEFORM_WIDGET_TYPE (live_waveform_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  LiveWaveformWidget,
  live_waveform_widget,
  Z,
  LIVE_WAVEFORM_WIDGET,
  GtkDrawingArea)

typedef struct Port Port;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum LiveWaveformType
{
  LIVE_WAVEFORM_ENGINE,
  LIVE_WAVEFORM_PORT,
} LiveWaveformType;

typedef struct _LiveWaveformWidget
{
  GtkDrawingArea parent_instance;

  LiveWaveformType type;

  /** Draw border or not. */
  int draw_border;

  float * bufs[2];

  /** Current buffer sizes. */
  size_t buf_sz[2];

  /** Used for drawing. */
  GdkRGBA color_green;

  /** Port, if port. */
  Port * port;

} LiveWaveformWidget;

/**
 * Creates a LiveWaveformWidget for the AudioEngine.
 */
void
live_waveform_widget_setup_engine (LiveWaveformWidget * self);

/**
 * Creates a LiveWaveformWidget for a port.
 */
LiveWaveformWidget *
live_waveform_widget_new_port (Port * port);

/**
 * @}
 */

#endif
