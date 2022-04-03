/*
 * Copyright (C) 2020-2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * Channel send selector widget.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SEND_SELECTOR_H__
#define __GUI_WIDGETS_CHANNEL_SEND_SELECTOR_H__

#include <gtk/gtk.h>

#define CHANNEL_SEND_SELECTOR_WIDGET_TYPE \
  (channel_send_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelSendSelectorWidget,
  channel_send_selector_widget,
  Z,
  CHANNEL_SEND_SELECTOR_WIDGET,
  GtkPopover)

typedef struct ChannelSend        ChannelSend;
typedef struct _ChannelSendWidget ChannelSendWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _ChannelSendSelectorWidget
{
  GtkPopover parent_instance;

  /** Owner. */
  ChannelSendWidget * send_widget;

  /** Main vbox. */
  GtkBox * vbox;

  /** Button group below the list box. */
  GtkBox * btn_box;

  GtkTreeModel * model;
  GtkTreeView *  treeview;

  GtkButton * ok_btn;

} ChannelSendSelectorWidget;

ChannelSendSelectorWidget *
channel_send_selector_widget_new (
  ChannelSendWidget * send);

void
channel_send_selector_widget_setup (
  ChannelSendSelectorWidget * self);

/**
 * @}
 */

#endif
