/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file
 *
 * Modulator.
 */

#ifndef __GUI_WIDGETS_MODULATOR_H__
#define __GUI_WIDGETS_MODULATOR_H__

#include "audio/track.h"
#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

typedef struct _KnobWithNameWidget
  KnobWithNameWidget;
typedef struct _LiveWaveformWidget
  LiveWaveformWidget;
typedef struct _PortConnectionsPopoverWidget
  PortConnectionsPopoverWidget;
typedef struct Modulator Modulator;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MODULATOR_WIDGET_TYPE \
  (modulator_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorWidget,
  modulator_widget,
  Z, MODULATOR_WIDGET,
  TwoColExpanderBoxWidget)

/**
 * Modulator.
 */
typedef struct _ModulatorWidget
{
  TwoColExpanderBoxWidget  parent_instance;

  /** Main box (top toolbar and bot content box. */
  GtkBox *          main_box;

  /** Toolbar above the content. */
  GtkToolbar *      toolbar;

  /** Scrolled window for the content box. */
  GtkScrolledWindow * content_scroll;

  /** Box containing the content (controls +
   * waveforms). */
  GtkBox *          content_box;

  /** The controls box on the left. */
  GtkBox *          controls_box;

  KnobWithNameWidget ** knobs;
  int               num_knobs;
  int               knobs_size;

  /** The graphs on the right. */
  GtkBox *          waveforms_box;
  GtkOverlay *      waveform_overlays[16];
  GtkButton *       waveform_automate_buttons[16];
  //PortConnectionsPopoverWidget *
    //waveform_popovers[16];
  LiveWaveformWidget * waveforms[16];
  int               num_waveforms;

  Port *            ports[16];

  //GtkDrawingArea *  graph;

  /** Width is 60 so 59 previous points per
   * CV out (max 16). */
  //double            prev_points[16][60];

  /** Pointer back to the Modulator. */
  Plugin *          modulator;
} ModulatorWidget;

ModulatorWidget *
modulator_widget_new (
  Plugin * modulator);

/**
 * @}
 */

#endif
