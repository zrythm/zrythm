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

#ifndef __GUI_WIDGETS_MIDI_CONTROLLER_MB_H__
#define __GUI_WIDGETS_MIDI_CONTROLLER_MB_H__

#include <gtk/gtk.h>

#define MIDI_CONTROLLER_MB_WIDGET_TYPE \
  (midi_controller_mb_widget_get_type ())
G_DECLARE_FINAL_TYPE (MidiControllerMbWidget,
                      midi_controller_mb_widget,
                      Z,
                      MIDI_CONTROLLER_MB_WIDGET,
                      GtkMenuButton)

typedef struct _MidiControllerPopoverWidget
  MidiControllerPopoverWidget;

typedef struct _MidiControllerMbWidget
{
  GtkMenuButton           parent_instance;
  GtkBox *                box; ///< the box
  GtkImage *              img; ///< img to show next to the label
  GtkLabel                * label; ///< label to show
  MidiControllerPopoverWidget   * popover; ///< the popover to show
  GtkBox                  * content; ///< popover content holder
} MidiControllerMbWidget;

void
midi_controller_mb_widget_setup (
  MidiControllerMbWidget * self);

/**
 * Called from PreferencesWidget to save the
 * settings.
 */
void
midi_controller_mb_widget_save_settings (
  MidiControllerMbWidget * self);

void
midi_controller_mb_widget_refresh (
  MidiControllerMbWidget * self);

#endif

