/*
 * SPDX-FileCopyrightText: © 2020 Alexandros Theodotou <alex@zrythm.org>
 *
 * SPDX-License-Identifier: LicenseRef-ZrythmLicense
 */

/**
 * @file
 *
 * Channel sends.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SENDS_EXPANDER_H__
#define __GUI_WIDGETS_CHANNEL_SENDS_EXPANDER_H__

#include "gui/widgets/expander_box.h"

#include "gtk_wrapper.h"

#define CHANNEL_SENDS_EXPANDER_WIDGET_TYPE \
  (channel_sends_expander_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelSendsExpanderWidget,
  channel_sends_expander_widget,
  Z,
  CHANNEL_SENDS_EXPANDER_WIDGET,
  ExpanderBoxWidget);

class Track;
typedef struct _ChannelSendWidget ChannelSendWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

enum class ChannelSendsExpanderPosition
{
  CSE_POSITION_CHANNEL,
  CSE_POSITION_INSPECTOR,
};

/**
 * A TwoColExpanderBoxWidget for showing the ports
 * in the InspectorWidget.
 */
using ChannelSendsExpanderWidget = struct _ChannelSendsExpanderWidget
{
  ExpanderBoxWidget parent_instance;

  ChannelSendsExpanderPosition position;

  /** Scrolled window for the vbox inside. */
  GtkScrolledWindow * scroll;
  GtkViewport *       viewport;

  /** VBox containing each slot. */
  GtkBox * box;

  /** 1 box for each item. */
  std::vector<GtkBox *> strip_boxes;

  /** Send slots. */
  std::vector<ChannelSendWidget *> slots;

  /** Owner track. */
  Track * track;
};

/**
 * Refreshes each field.
 */
void
channel_sends_expander_widget_refresh (ChannelSendsExpanderWidget * self);

/**
 * Sets up the ChannelSendsExpanderWidget.
 */
void
channel_sends_expander_widget_setup (
  ChannelSendsExpanderWidget * self,
  ChannelSendsExpanderPosition position,
  Track *                      track);

/**
 * @}
 */

#endif
