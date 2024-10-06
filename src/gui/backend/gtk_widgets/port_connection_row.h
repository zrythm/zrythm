// SPDX-FileCopyrightText: © 2019-2021, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_PORT_CONNECTION_ROW_H__
#define __GUI_WIDGETS_PORT_CONNECTION_ROW_H__

#include "common/utils/types.h"
#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define PORT_CONNECTION_ROW_WIDGET_TYPE (port_connection_row_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  PortConnectionRowWidget,
  port_connection_row_widget,
  Z,
  PORT_CONNECTION_ROW_WIDGET,
  GtkBox)

TYPEDEF_STRUCT_UNDERSCORED (KnobWidget);
TYPEDEF_STRUCT_UNDERSCORED (BarSliderWidget);
TYPEDEF_STRUCT_UNDERSCORED (PortConnectionsPopoverWidget);
class PortConnection;

using PortConnectionRowWidget = struct _PortConnectionRowWidget
{
  GtkBox parent_instance;

  std::unique_ptr<PortConnection> connection;

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
};

/**
 * Creates the popover.
 */
PortConnectionRowWidget *
port_connection_row_widget_new (
  PortConnectionsPopoverWidget * parent,
  const PortConnection          &connection,
  bool                           is_input);

#endif
