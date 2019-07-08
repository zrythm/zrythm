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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_PORT_CONNECTION_ROW_H__
#define __GUI_WIDGETS_PORT_CONNECTION_ROW_H__

#include <gtk/gtk.h>

#define PORT_CONNECTION_ROW_WIDGET_TYPE \
  (port_connection_row_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionRowWidget,
  port_connection_row_widget,
  Z, PORT_CONNECTION_ROW_WIDGET,
  GtkEventBox)

typedef struct _KnobWidget KnobWidget;

typedef struct _PortConnectionRowWidget
{
  GtkEventBox      parent_instance;

  Port *           src;
  Port *           dest;

  /**
   * If this is 1, src is the input port and dest
   * is the current one, otherwise dest is the
   * output port and src is the current one.
   */
  int              is_input;

  /** The knob. */
  KnobWidget *     knob;
} PortConnectionRowWidget;

/**
 * Creates the popover.
 */
PortConnectionRowWidget *
port_connection_row_widget_new (
  Port * src,
  Port * dest,
  int    is_input);

#endif
