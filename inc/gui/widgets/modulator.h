/*
 * SPDX-FileCopyrightText: © 2019 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Modulator.
 */

#ifndef __GUI_WIDGETS_MODULATOR_H__
#define __GUI_WIDGETS_MODULATOR_H__

#include "dsp/track.h"
#include "gui/widgets/two_col_expander_box.h"

#include <gtk/gtk.h>

typedef struct _ModulatorInnerWidget ModulatorInnerWidget;
typedef struct Modulator             Modulator;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MODULATOR_WIDGET_TYPE (modulator_widget_get_type ())
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
