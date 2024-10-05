// SPDX-FileCopyrightText: © 2020-2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Channel send widget.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SEND_H__
#define __GUI_WIDGETS_CHANNEL_SEND_H__

#include "gtk_wrapper.h"

#define CHANNEL_SEND_WIDGET_TYPE (channel_send_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelSendWidget,
  channel_send_widget,
  Z,
  CHANNEL_SEND_WIDGET,
  GtkWidget)

class ChannelSend;
using ChannelSendSelectorWidget = struct _ChannelSendSelectorWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

using ChannelSendWidget = struct _ChannelSendWidget
{
  GtkWidget parent_instance;

  /** Owner. */
  ChannelSend * send;

  GtkGestureClick * click;
  GtkGestureDrag *  drag;

  double start_x;
  double last_offset_x;

  float send_amount_at_start;

  /** For multipress. */
  int n_press;

  GtkGestureClick * right_mouse_mp;

  /** Cache tooltip string. */
  char * cache_tooltip;

  PangoLayout * txt_layout;

  /** Flag used for adding/removing .empty CSS
   * class. */
  bool was_empty;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;

  ChannelSendSelectorWidget * selector_popover;
};

/**
 * Creates a new ChannelSend widget and binds it to
 * the given value.
 */
ChannelSendWidget *
channel_send_widget_new (ChannelSend * send);

/**
 * @}
 */

#endif
