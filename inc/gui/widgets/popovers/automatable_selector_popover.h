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

/* FIXME should be merged with port selector popover */

#include <gtk/gtk.h>

#define AUTOMATABLE_SELECTOR_POPOVER_WIDGET_TYPE \
  (automatable_selector_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  AutomatableSelectorPopoverWidget,
  automatable_selector_popover_widget,
  Z,
  AUTOMATABLE_SELECTOR_POPOVER_WIDGET,
  GtkPopover)

typedef struct _AutomatableSelectorButtonWidget AutomatableSelectorButtonWidget;

/**
 * @addtogroup widgets
 *
 * @{
 */

/**
 * Automatable type.
 *
 * These are shown on the left side of the popover.
 */
typedef enum AutomatableSelectorType
{
  /** Midi automatable (modwheel etc.). */
  AS_TYPE_MIDI_CH1,
  AS_TYPE_MIDI_CH2,
  AS_TYPE_MIDI_CH3,
  AS_TYPE_MIDI_CH4,
  AS_TYPE_MIDI_CH5,
  AS_TYPE_MIDI_CH6,
  AS_TYPE_MIDI_CH7,
  AS_TYPE_MIDI_CH8,
  AS_TYPE_MIDI_CH9,
  AS_TYPE_MIDI_CH10,
  AS_TYPE_MIDI_CH11,
  AS_TYPE_MIDI_CH12,
  AS_TYPE_MIDI_CH13,
  AS_TYPE_MIDI_CH14,
  AS_TYPE_MIDI_CH15,
  AS_TYPE_MIDI_CH16,

  /** Channel. */
  AS_TYPE_CHANNEL,

  /** Plugin at Track MIDI fx slot. */
  AS_TYPE_MIDI_FX,

  /** Instrument plugin. */
  AS_TYPE_INSTRUMENT,

  /** Insert plugin. */
  AS_TYPE_INSERT,

  /** Modulator plugin. */
  AS_TYPE_MODULATOR,

  /** Tempo track ports. */
  AS_TYPE_TEMPO,

  /** Modulator macros. */
  AS_TYPE_MACRO,
} AutomatableSelectorType;

/**
 * A popover for selecting the automation track
 * to automate.
 */
typedef struct _AutomatableSelectorPopoverWidget
{
  GtkPopover parent_instance;

  /** The owner button. */
  AutomationTrack * owner;

  GtkBox *       type_treeview_box;
  GtkTreeView *  type_treeview;
  GtkTreeModel * type_model;
  GtkBox *       port_treeview_box;
  GtkTreeView *  port_treeview;
  GtkTreeModel * port_model;

  GtkLabel * info;

  /** Flag to set to ignore the selection change
   * callbacks when setting a selection manually */
  int selecting_manually;

  AutomatableSelectorType selected_type;
  int                     selected_slot;

  /**
   * The selected Port will be stored here
   * and passed to the button when closing so that
   * it can hide the current AutomationTrack and
   * create/show the one corresponding to this
   * Automatable.
   */
  Port * selected_port;
} AutomatableSelectorPopoverWidget;

/**
 * Creates the popover.
 */
AutomatableSelectorPopoverWidget *
automatable_selector_popover_widget_new (AutomationTrack * owner);

/**
 * @}
 */

#endif
