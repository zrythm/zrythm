// SPDX-FileCopyrightText: © 2019, 2021, 2023-2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_H__
#define __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_H__

#include "common/utils/types.h"
#include "gui/cpp/gtk_widgets/libadwaita_wrapper.h"

#define ROUTE_TARGET_SELECTOR_WIDGET_TYPE \
  (route_target_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RouteTargetSelectorWidget,
  route_target_selector_widget,
  Z,
  ROUTE_TARGET_SELECTOR_WIDGET,
  AdwBin)

TYPEDEF_STRUCT_UNDERSCORED (RouteTargetSelectorPopoverWidget);
TYPEDEF_STRUCT_UNDERSCORED (ChannelWidget);
class ChannelTrack;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Dropdown to select the direct output of a track.
 */
using RouteTargetSelectorWidget = struct _RouteTargetSelectorWidget
{
  AdwBin parent_instance;

  GtkDropDown * dropdown;

  /** Owner track. */
  ChannelTrack * track;
};

void
route_target_selector_widget_refresh (
  RouteTargetSelectorWidget * self,
  ChannelTrack *              track);

/**
 * @}
 */

#endif
