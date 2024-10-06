// SPDX-FileCopyrightText: © 2019-2020 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_ACTIVE_HARDWARE_POPOVER_H__
#define __GUI_WIDGETS_ACTIVE_HARDWARE_POPOVER_H__

#include "gui/backend/gtk_widgets/gtk_wrapper.h"

#define ACTIVE_HARDWARE_POPOVER_WIDGET_TYPE \
  (active_hardware_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ActiveHardwarePopoverWidget,
  active_hardware_popover_widget,
  Z,
  ACTIVE_HARDWARE_POPOVER_WIDGET,
  GtkPopover)

typedef struct _ActiveHardwareMbWidget ActiveHardwareMbWidget;

typedef struct _ActiveHardwarePopoverWidget
{
  GtkPopover               parent_instance;
  ActiveHardwareMbWidget * owner; ///< the owner
  GtkBox *                 controllers_box;
  GtkButton *              rescan;
} ActiveHardwarePopoverWidget;

/**
 * Creates the popover.
 */
ActiveHardwarePopoverWidget *
active_hardware_popover_widget_new (ActiveHardwareMbWidget * owner);

#endif
