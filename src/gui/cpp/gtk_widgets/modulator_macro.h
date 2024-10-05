// SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * ModulatorMacro macro knob.
 */

#ifndef __GUI_WIDGETS_MODULATOR_MACRO_H__
#define __GUI_WIDGETS_MODULATOR_MACRO_H__

#include "dsp/track.h"
#include "gui/cpp/gtk_widgets/two_col_expander_box.h"

#include "gtk_wrapper.h"

typedef struct _KnobWithNameWidget           KnobWithNameWidget;
typedef struct _PortConnectionsPopoverWidget PortConnectionsPopoverWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

#define MODULATOR_MACRO_WIDGET_TYPE (modulator_macro_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ModulatorMacroWidget,
  modulator_macro_widget,
  Z,
  MODULATOR_MACRO_WIDGET,
  GtkWidget)

/**
 * ModulatorMacro.
 */
typedef struct _ModulatorMacroWidget
{
  GtkWidget parent_instance;

  GtkGrid * grid;

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

  /** Used for context menu. */
  GtkPopoverMenu * popover_menu;
} ModulatorMacroWidget;

void
modulator_macro_widget_refresh (ModulatorMacroWidget * self);

ModulatorMacroWidget *
modulator_macro_widget_new (int modulator_macro_index);

/**
 * @}
 */

#endif
