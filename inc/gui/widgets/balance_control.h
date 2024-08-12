// SPDX-FileCopyrightText: © 2019-2020, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/** @file
 */

#ifndef __GUI_WIDGETS_BALANCE_CONTROL_H__
#define __GUI_WIDGETS_BALANCE_CONTROL_H__

#include "utils/color.h"
#include "utils/types.h"

#include "gtk_wrapper.h"

class ControlPort;

#define BALANCE_CONTROL_WIDGET_TYPE (balance_control_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BalanceControlWidget,
  balance_control_widget,
  Z,
  BALANCE_CONTROL_WIDGET,
  GtkWidget)

using BalanceControlWidget = struct _BalanceControlWidget
{
  GtkWidget        parent_instance;
  GtkGestureDrag * drag;

  /** Getter. */
  GenericFloatGetter getter;

  /** Setter. */
  GenericFloatSetter setter;

  /** Object to call get/set with. */
  void *      object;
  double      last_x;
  double      last_y;
  GtkWindow * tooltip_win;
  GtkLabel *  tooltip_label;
  GdkRGBA     start_color;
  GdkRGBA     end_color;

  /** Currently hovered or not. */
  int hovered;

  /** Currently being dragged or not. */
  int dragged;

  /** Layout for drawing text. */
  PangoLayout * layout;

  /** Balance at start of drag. */
  float balance_at_start;

  ControlPort * port;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
};

/**
 * Creates a new BalanceControl widget and binds it
 * to the given value.
 *
 * @param port Optional port to use in MIDI CC
 *   binding dialog.
 */
BalanceControlWidget *
balance_control_widget_new (
  GenericFloatGetter getter,
  GenericFloatSetter setter,
  void *             object,
  ControlPort *      port,
  int                height);

#endif
