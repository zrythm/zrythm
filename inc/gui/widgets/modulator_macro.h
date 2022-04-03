/*
 * Copyright (C) 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * ModulatorMacro macro knob.
 */

#ifndef __GUI_WIDGETS_MODULATOR_MACRO_H__
#define __GUI_WIDGETS_MODULATOR_MACRO_H__

#include "audio/track.h"
#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

typedef struct _KnobWithNameWidget KnobWithNameWidget;
typedef struct _PortConnectionsPopoverWidget
  PortConnectionsPopoverWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MODULATOR_MACRO_WIDGET_TYPE \
  (modulator_macro_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorMacroWidget,
  modulator_macro_widget,
  Z,
  MODULATOR_MACRO_WIDGET,
  GtkGrid)

/**
 * ModulatorMacro.
 */
typedef struct _ModulatorMacroWidget
{
  GtkGrid parent_instance;

  KnobWithNameWidget * knob_with_name;

  GtkDrawingArea * inputs;
  GtkDrawingArea * output;

  /** Button to show an unused modulator macro. */
  GtkButton * add_input;

  GtkButton * outputs;

  /** Index of the modulator macro in the track. */
  int modulator_macro_idx;

  PangoLayout * layout;

  PortConnectionsPopoverWidget * connections_popover;
} ModulatorMacroWidget;

void
modulator_macro_widget_refresh (
  ModulatorMacroWidget * self);

ModulatorMacroWidget *
modulator_macro_widget_new (
  int modulator_macro_index);

/**
 * @}
 */

#endif
