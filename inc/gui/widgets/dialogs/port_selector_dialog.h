/*
 * Copyright (C) 2019-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Port selector dialog.
 */

#ifndef __GUI_WIDGETS_PORT_SELECTOR_DIALOG_H__
#define __GUI_WIDGETS_PORT_SELECTOR_DIALOG_H__

#include <gtk/gtk.h>

#define PORT_SELECTOR_DIALOG_WIDGET_TYPE \
  (port_selector_dialog_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortSelectorDialogWidget,
  port_selector_dialog_widget,
  Z,
  PORT_SELECTOR_DIALOG_WIDGET,
  GtkDialog)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct Port Port;
typedef struct _PortConnectionsPopoverWidget
  PortConnectionsPopoverWidget;

/**
 * A GtkPopover to select Port's.
 */
typedef struct _PortSelectorDialogWidget
{
  GtkDialog parent_instance;

  /** The owner Port. */
  Port * port;

  GtkScrolledWindow * track_scroll;
  GtkTreeView *       track_treeview;
  GtkTreeModel *      track_model;
  GtkScrolledWindow * plugin_scroll;
  GtkTreeView *       plugin_treeview;
  GtkTreeModel *      plugin_model;
  GtkSeparator *      plugin_separator;
  GtkScrolledWindow * port_scroll;
  GtkTreeView *       port_treeview;
  GtkTreeModel *      port_model;

  /** Track selected in the Track treeview. */
  Track * selected_track;

  /**
   * Plugin selected in the Plugin treeview.
   *
   * If this is NULL, see track_ports_selected.
   */
  Plugin * selected_plugin;

  /**
   * Used if selected_plugin is NULL.
   *
   * If this is 1, the track ports are selected,
   * otherwise if this is 0 then nothing is selected
   * in the Plugin treeview.
   */
  int track_ports_selected;

  Port * selected_port;

  GtkButton * ok_btn;
  GtkButton * cancel_btn;

  /** Whether already set up (to avoid re-adding
   * existing widgets). */
  bool setup;

  PortConnectionsPopoverWidget * owner;
} PortSelectorDialogWidget;

void
port_selector_dialog_widget_refresh (
  PortSelectorDialogWidget * self,
  Port *                     port);

/**
 * Creates the popover.
 */
PortSelectorDialogWidget *
port_selector_dialog_widget_new (
  PortConnectionsPopoverWidget * owner);

/**
 * @}
 */

#endif
