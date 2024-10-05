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

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"
#include "gui/cpp/gtk_widgets/two_col_expander_box.h"

#include "common/dsp/track.h"

typedef struct _ModulatorInnerWidget ModulatorInnerWidget;

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
