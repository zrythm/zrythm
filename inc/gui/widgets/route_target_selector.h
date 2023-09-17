// SPDX-FileCopyrightText: © 2019, 2021, 2023 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

#ifndef __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_H__
#define __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_H__

#include <adwaita.h>

#define ROUTE_TARGET_SELECTOR_WIDGET_TYPE \
  (route_target_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RouteTargetSelectorWidget,
  route_target_selector_widget,
  Z,
  ROUTE_TARGET_SELECTOR_WIDGET,
  AdwBin)

typedef struct _RouteTargetSelectorPopoverWidget RouteTargetSelectorPopoverWidget;
typedef struct _ChannelWidget ChannelWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Dropdown to select the direct output of a track.
 */
typedef struct _RouteTargetSelectorWidget
{
  AdwBin parent_instance;

  GtkDropDown * dropdown;

  /** Owner track. */
  Track * track;
} RouteTargetSelectorWidget;

void
route_target_selector_widget_refresh (
  RouteTargetSelectorWidget * self,
  Track *                     track);

/**
 * @}
 */

#endif
