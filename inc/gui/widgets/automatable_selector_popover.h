/*
 * Copyright (C) 2019 Alexandros Theodotou <alex at zrythm dot org>
 *
 * This file is part of Zrythm
 *
 * Zrythm is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Zrythm is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef __GUI_WIDGETS_AUTOMATABLE_SELECTOR_POPOVER_H__
#define __GUI_WIDGETS_AUTOMATABLE_SELECTOR_POPOVER_H__


#include <gtk/gtk.h>

#define AUTOMATABLE_SELECTOR_POPOVER_WIDGET_TYPE \
  (automatable_selector_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomatableSelectorPopoverWidget,
  automatable_selector_popover_widget,
  Z, AUTOMATABLE_SELECTOR_POPOVER_WIDGET,
  GtkPopover)

typedef struct _AutomatableSelectorButtonWidget
  AutomatableSelectorButtonWidget;

typedef enum AutomatableSelectorType
{
  AS_TYPE_CHANNEL,
  AS_TYPE_PLUGIN_0,
  AS_TYPE_PLUGIN_1,
  AS_TYPE_PLUGIN_2,
  AS_TYPE_PLUGIN_3,
  AS_TYPE_PLUGIN_4,
  AS_TYPE_PLUGIN_5,
  AS_TYPE_PLUGIN_6,
  AS_TYPE_PLUGIN_7,
  AS_TYPE_PLUGIN_8,
} AutomatableSelectorType;

typedef struct _AutomatableSelectorPopoverWidget
{
  GtkPopover              parent_instance;

  /** The owner button. */
  AutomatableSelectorButtonWidget * owner;

  GtkBox *                type_treeview_box;
  GtkTreeView *           type_treeview;
  GtkTreeModel *          type_model;
  GtkBox *                automatable_treeview_box;
  GtkTreeView *           automatable_treeview;
  GtkTreeModel *          automatable_model;

  GtkLabel *              info;

  AutomatableSelectorType selected_type;
} AutomatableSelectorPopoverWidget;

/**
 * Creates the popover.
 */
AutomatableSelectorPopoverWidget *
automatable_selector_popover_widget_new (
  AutomatableSelectorButtonWidget * owner);

#endif

