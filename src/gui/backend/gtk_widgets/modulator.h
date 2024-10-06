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

#include "common/dsp/track.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"
#include "gui/backend/gtk_widgets/two_col_expander_box.h"

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
  zrythm::plugins::Plugin * modulator;
} ModulatorWidget;

void
modulator_widget_refresh (ModulatorWidget * self);

ModulatorWidget *
modulator_widget_new (zrythm::plugins::Plugin * modulator);

/**
 * @}
 */

#endif
