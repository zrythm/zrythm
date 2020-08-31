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

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef enum AutomatableSelectorType
{
  AS_TYPE_MIDI,
  AS_TYPE_CHANNEL,
  AS_TYPE_MIDI_FX_0,
  AS_TYPE_MIDI_FX_1,
  AS_TYPE_MIDI_FX_2,
  AS_TYPE_MIDI_FX_3,
  AS_TYPE_MIDI_FX_4,
  AS_TYPE_MIDI_FX_5,
  AS_TYPE_MIDI_FX_6,
  AS_TYPE_MIDI_FX_7,
  AS_TYPE_MIDI_FX_8,
  AS_TYPE_INSTRUMENT,
  AS_TYPE_INSERT_0,
  AS_TYPE_INSERT_1,
  AS_TYPE_INSERT_2,
  AS_TYPE_INSERT_3,
  AS_TYPE_INSERT_4,
  AS_TYPE_INSERT_5,
  AS_TYPE_INSERT_6,
  AS_TYPE_INSERT_7,
  AS_TYPE_INSERT_8,
  AS_TYPE_MODULATOR_0,
} AutomatableSelectorType;

/**
 * A popover for selecting the automation track
 * to automate.
 */
typedef struct _AutomatableSelectorPopoverWidget
{
  GtkPopover              parent_instance;

  /** The owner button. */
  AutomationTrack *       owner;

  GtkBox *                type_treeview_box;
  GtkTreeView *           type_treeview;
  GtkTreeModel *          type_model;
  GtkBox *                port_treeview_box;
  GtkTreeView *           port_treeview;
  GtkTreeModel *          port_model;

  GtkLabel *              info;

  /** Flag to set to ignore the selection change
   * callbacks when setting a selection manually */
  int                     selecting_manually;

  AutomatableSelectorType selected_type;

  /**
   * The selected Port will be stored here
   * and passed to the button when closing so that
   * it can hide the current AutomationTrack and
   * create/show the one corresponding to this
   * Automatable.
   */
  Port *       selected_port;
} AutomatableSelectorPopoverWidget;

/**
 * Creates the popover.
 */
AutomatableSelectorPopoverWidget *
automatable_selector_popover_widget_new (
  AutomationTrack * owner);

/**
 * @}
 */

#endif
