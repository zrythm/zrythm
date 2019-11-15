/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
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

#ifndef __GUI_WIDGETS_AUTOMATABLE_SELECTOR_BUTTON_H__
#define __GUI_WIDGETS_AUTOMATABLE_SELECTOR_BUTTON_H__

#include <gtk/gtk.h>

#define AUTOMATABLE_SELECTOR_BUTTON_WIDGET_TYPE \
  (automatable_selector_button_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomatableSelectorButtonWidget,
  automatable_selector_button_widget,
  Z, AUTOMATABLE_SELECTOR_BUTTON_WIDGET,
  GtkMenuButton)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _AutomatableSelectorPopoverWidget AutomatableSelectorPopoverWidget;
typedef struct Automatable Automatable;
typedef struct _AutomationTrackWidget
  AutomationTrackWidget;

typedef struct _AutomatableSelectorButtonWidget
{
  GtkMenuButton       parent_instance;
  GtkBox *            box;
  /** Image to show next to the label. */
  GtkImage *          img;

  /** Label to show. */
  GtkLabel *          label;
  AutomatableSelectorPopoverWidget   * popover; ///< the popover to show
  GtkBox *            content; ///< popover content holder
  AutomationTrack *   owner;
} AutomatableSelectorButtonWidget;

void
automatable_selector_button_widget_setup (
  AutomatableSelectorButtonWidget * self,
  AutomationTrackWidget *            owner);

void
automatable_selector_button_widget_refresh (
  AutomatableSelectorButtonWidget * self);

/**
 * Sets the Automatable for this automation track
 * lane.
 *
 * If different from the current one, it will hide
 * the current AutomationTrack and show the
 * one corresponding to this Automatable.
 */
void
automatable_selector_button_set_automatable (
  AutomatableSelectorButtonWidget * self,
  Automatable *                     a);

/**
 * @}
 */

#endif
