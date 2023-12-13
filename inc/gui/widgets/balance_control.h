// clang-format off
// SPDX-FileCopyrightText: © 2019-2020, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/** \file
 */

#ifndef __GUI_WIDGETS_BALANCE_CONTROL_H__
#define __GUI_WIDGETS_BALANCE_CONTROL_H__

#include "utils/types.h"

#include <gtk/gtk.h>

#define BALANCE_CONTROL_WIDGET_TYPE (balance_control_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  BalanceControlWidget,
  balance_control_widget,
  Z,
  BALANCE_CONTROL_WIDGET,
  GtkWidget)

typedef struct _BalanceControlWidget
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

  Port * port;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;
} BalanceControlWidget;

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
  Port *             port,
  int                height);

#endif
