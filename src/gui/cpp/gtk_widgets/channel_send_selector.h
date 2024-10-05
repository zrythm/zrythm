// SPDX-FileCopyrightText: © 2020-2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Channel send selector widget.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SEND_SELECTOR_H__
#define __GUI_WIDGETS_CHANNEL_SEND_SELECTOR_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

#include "common/utils/types.h"

#define CHANNEL_SEND_SELECTOR_WIDGET_TYPE \
  (channel_send_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelSendSelectorWidget,
  channel_send_selector_widget,
  Z,
  CHANNEL_SEND_SELECTOR_WIDGET,
  GtkPopover)

typedef struct _ChannelSendWidget ChannelSendWidget;
class ItemFactory;

/**
 * @addtogroup widgets
 *
 * @{
 */

using ChannelSendSelectorWidget = struct _ChannelSendSelectorWidget
{
  GtkPopover parent_instance;

  /** Owner. */
  ChannelSendWidget * send_widget;

  /** Main vbox. */
  GtkBox * vbox;

  GtkSingleSelection *         view_model;
  GtkListView *                view;
  std::unique_ptr<ItemFactory> item_factory;
};

ChannelSendSelectorWidget *
channel_send_selector_widget_new (ChannelSendWidget * send);

void
channel_send_selector_widget_setup (ChannelSendSelectorWidget * self);

/**
 * @}
 */

#endif
