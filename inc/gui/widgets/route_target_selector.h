/*
 * Copyright (C) 2019, 2021 Alexandros Theodotou <alex at zrythm dot org>
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_H__
#define __GUI_WIDGETS_ROUTE_TARGET_SELECTOR_H__

#include <gtk/gtk.h>

#define ROUTE_TARGET_SELECTOR_WIDGET_TYPE \
  (route_target_selector_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  RouteTargetSelectorWidget,
  route_target_selector_widget,
  Z,
  ROUTE_TARGET_SELECTOR_WIDGET,
  GtkBox)

typedef struct _RouteTargetSelectorPopoverWidget
  RouteTargetSelectorPopoverWidget;
typedef struct _ChannelWidget ChannelWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _RouteTargetSelectorWidget
{
  GtkBox parent_instance;

  GtkMenuButton * menu_button;

  /** The box. */
  GtkBox * box;
  /** Image to show next to the label. */
  GtkImage * img;
  /** Label. */
  GtkLabel * label;
  /** Popover to be shown when clicked. */
  RouteTargetSelectorPopoverWidget * popover;

  /** Popover content holder. */
  GtkBox *  content;
  Channel * channel;
} RouteTargetSelectorWidget;

RouteTargetSelectorWidget *
route_target_selector_widget_new (
  Channel * channel);

void
route_target_selector_widget_setup (
  RouteTargetSelectorWidget * self,
  Channel *                   channel);

void
route_target_selector_widget_refresh (
  RouteTargetSelectorWidget * self,
  Channel *                   channel);

/**
 * @}
 */

#endif
