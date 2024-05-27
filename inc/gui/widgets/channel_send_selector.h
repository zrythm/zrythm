// clang-format off
// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 *
 * Channel send selector widget.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SEND_SELECTOR_H__
#define __GUI_WIDGETS_CHANNEL_SEND_SELECTOR_H__

#include "gtk_wrapper.h"

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
TYPEDEF_STRUCT (ItemFactory);

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

  GtkSingleSelection * view_model;
  GtkListView *        view;
  ItemFactory *        item_factory;

} ChannelSendSelectorWidget;

ChannelSendSelectorWidget *
channel_send_selector_widget_new (ChannelSendWidget * send);

void
channel_send_selector_widget_setup (ChannelSendSelectorWidget * self);

/**
 * @}
 */

#endif
