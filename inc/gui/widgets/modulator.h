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

typedef struct _ModulatorInnerWidget
                         ModulatorInnerWidget;
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
  Z,
  MODULATOR_WIDGET,
  TwoColExpanderBoxWidget)

/**
 * Modulator.
 */
typedef struct _ModulatorWidget
{
  TwoColExpanderBoxWidget parent_instance;

  ModulatorInnerWidget * inner;

  /** Pointer back to the Modulator. */
  Plugin * modulator;
} ModulatorWidget;

void
modulator_widget_refresh (ModulatorWidget * self);

ModulatorWidget *
modulator_widget_new (Plugin * modulator);

/**
 * @}
 */

#endif
