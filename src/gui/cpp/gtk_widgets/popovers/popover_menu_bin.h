/*
 * SPDX-FileCopyrightText: © 2021-2022 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Plugin browser.
 */

#ifndef __GUI_WIDGETS_POPOVER_MENU_BIN_H__
#define __GUI_WIDGETS_POPOVER_MENU_BIN_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#define POPOVER_MENU_BIN_WIDGET_TYPE (popover_menu_bin_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PopoverMenuBinWidget,
  popover_menu_bin_widget,
  Z,
  POPOVER_MENU_BIN_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Container for widgets that wish to have a right
 * click menu.
 */
typedef struct _PopoverMenuBinWidget
{
  GtkWidget parent_instance;

  GtkWidget * child;

  GMenuModel * menu_model;

  /** Context menu. */
  GtkPopoverMenu * popover_menu;
} PopoverMenuBinWidget;

GMenuModel *
popover_menu_bin_widget_get_menu_model (PopoverMenuBinWidget * self);

void
popover_menu_bin_widget_set_menu_model (
  PopoverMenuBinWidget * self,
  GMenuModel *           model);

void
popover_menu_bin_widget_set_child (
  PopoverMenuBinWidget * self,
  GtkWidget *            child);

GtkWidget *
popover_menu_bin_widget_get_child (PopoverMenuBinWidget * self);

PopoverMenuBinWidget *
popover_menu_bin_widget_new (void);

/**
 * @}
 */

#endif
