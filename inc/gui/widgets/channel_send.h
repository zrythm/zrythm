/*
 * Copyright (C) 2020-2022 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with Zrythm.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * \file
 *
 * Channel send widget.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SEND_H__
#define __GUI_WIDGETS_CHANNEL_SEND_H__

#include <gtk/gtk.h>

#define CHANNEL_SEND_WIDGET_TYPE \
  (channel_send_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelSendWidget,
  channel_send_widget,
  Z,
  CHANNEL_SEND_WIDGET,
  GtkWidget)

typedef struct ChannelSend ChannelSend;
typedef struct _ChannelSendSelectorWidget
  ChannelSendSelectorWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _ChannelSendWidget
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

  /** Layout cache for empty slot. */
  PangoLayout * empty_slot_layout;
  /** Layout cache for name. */
  PangoLayout * name_layout;

  /** Popover to be reused for context menus. */
  GtkPopoverMenu * popover_menu;

  ChannelSendSelectorWidget * selector_popover;
} ChannelSendWidget;

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
