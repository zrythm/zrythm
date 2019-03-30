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

#ifndef __GUI_WIDGETS_MIDI_CONTROLLER_POPOVER_H__
#define __GUI_WIDGETS_MIDI_CONTROLLER_POPOVER_H__


#include <gtk/gtk.h>

#define MIDI_CONTROLLER_POPOVER_WIDGET_TYPE \
  (midi_controller_popover_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiControllerPopoverWidget,
                      midi_controller_popover_widget,
                      Z,
                      MIDI_CONTROLLER_POPOVER_WIDGET,
                      GtkPopover)

typedef struct _MidiControllerMbWidget
  MidiControllerMbWidget;

typedef struct _MidiControllerPopoverWidget
{
  GtkPopover              parent_instance;
  MidiControllerMbWidget * owner; ///< the owner
  GtkBox *                controllers_box;
  GtkButton *             rescan;
  //GtkCheckButton *        controllers[50];
  //int                     num_controllers;
} MidiControllerPopoverWidget;

/**
 * Creates the popover.
 */
MidiControllerPopoverWidget *
midi_controller_popover_widget_new (
  MidiControllerMbWidget * owner);

#endif
