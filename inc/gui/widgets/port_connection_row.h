/*
 * SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

#ifndef __GUI_WIDGETS_PORT_CONNECTION_ROW_H__
#define __GUI_WIDGETS_PORT_CONNECTION_ROW_H__

#include <gtk/gtk.h>

#define PORT_CONNECTION_ROW_WIDGET_TYPE (port_connection_row_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionRowWidget,
  port_connection_row_widget,
  Z,
  PORT_CONNECTION_ROW_WIDGET,
  GtkBox)

typedef struct _KnobWidget                   KnobWidget;
typedef struct _BarSliderWidget              BarSliderWidget;
typedef struct _PortConnectionsPopoverWidget PortConnectionsPopoverWidget;
typedef struct PortConnection                PortConnection;

typedef struct _PortConnectionRowWidget
{
  GtkBox parent_instance;

  PortConnection * connection;

  /**
   * If this is 1, src is the input port and dest
   * is the current one, otherwise dest is the
   * output port and src is the current one.
   */
  bool is_input;

  /** Overlay to hold the slider and other
   * widgets. */
  GtkOverlay * overlay;

  GtkButton * delete_btn;

  /** The slider. */
  BarSliderWidget * slider;

  PortConnectionsPopoverWidget * parent;
} PortConnectionRowWidget;

/**
 * Creates the popover.
 */
PortConnectionRowWidget *
port_connection_row_widget_new (
  PortConnectionsPopoverWidget * parent,
  const PortConnection *         connection,
  bool                           is_input);

#endif
