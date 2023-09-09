// SPDX-FileCopyrightText: © 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * \file
 *
 * Channel slot activate button.
 */

#ifndef __GUI_WIDGETS_CHANNEL_SLOT_ACTIVATE_BUTTON_H__
#define __GUI_WIDGETS_CHANNEL_SLOT_ACTIVATE_BUTTON_H__

#include <gtk/gtk.h>

#define CHANNEL_SLOT_ACTIVATE_BUTTON_WIDGET_TYPE \
  (channel_slot_activate_button_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ChannelSlotActivateButtonWidget,
  channel_slot_activate_button_widget,
  Z,
  CHANNEL_SLOT_ACTIVATE_BUTTON_WIDGET,
  GtkToggleButton)

TYPEDEF_STRUCT_UNDERSCORED (ChannelSlotWidget);

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _ChannelSlotActivateButtonWidget
{
  GtkToggleButton parent_instance;

  ChannelSlotWidget * owner;

  gulong toggled_id;
} ChannelSlotActivateButtonWidget;

/**
 * Creates a new ChannelSlotActivateButton widget.
 */
ChannelSlotActivateButtonWidget *
channel_slot_activate_button_widget_new (ChannelSlotWidget * owner);

/**
 * @}
 */

#endif
