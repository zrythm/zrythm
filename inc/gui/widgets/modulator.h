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

#ifndef __GUI_WIDGETS_MODULATOR_H__
#define __GUI_WIDGETS_MODULATOR_H__

#include "audio/track.h"
#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

#define MODULATOR_WIDGET_TYPE \
  (modulator_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorWidget,
  modulator_widget,
  Z, MODULATOR_WIDGET,
  TwoColExpanderBoxWidget)

typedef struct _KnobWithNameWidget KnobWithNameWidget;
typedef struct Modulator Modulator;

typedef struct _ModulatorWidget
{
  TwoColExpanderBoxWidget  parent_instance;

  /** The controls box on the left. */
  GtkBox *          controls_box;

  KnobWithNameWidget ** knobs;
  int               num_knobs;
  int               knobs_size;

  /** The graph on the right. */
  GtkDrawingArea *  graph;

  /** Width is 60 so 59 previous points per
   * CV out (max 16). */
  double            prev_points[16][60];

  /** Pointer back to the Modulator. */
  Plugin *          modulator;
} ModulatorWidget;

ModulatorWidget *
modulator_widget_new (
  Plugin * modulator);

#endif
