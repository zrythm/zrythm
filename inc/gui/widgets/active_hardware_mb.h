/*
 * Copyright (C) 2019-2021 Alexandros Theodotou <alex at zrythm dot org>
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

/**
 * @file
 *
 * Active hardware menu button.
 */

#ifndef __GUI_WIDGETS_ACTIVE_HARDWARE_MB_H__
#define __GUI_WIDGETS_ACTIVE_HARDWARE_MB_H__

#include <stdbool.h>

#include "utils/types.h"

#include <gtk/gtk.h>

#define ACTIVE_HARDWARE_MB_WIDGET_TYPE \
  (active_hardware_mb_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  ActiveHardwareMbWidget,
  active_hardware_mb_widget,
  Z,
  ACTIVE_HARDWARE_MB_WIDGET,
  GtkWidget)

/**
 * @addtogroup widgets
 *
 * @{
 */

typedef struct _ActiveHardwarePopoverWidget
  ActiveHardwarePopoverWidget;

/**
 * A menu button that allows selecting active
 * hardware ports.
 */
typedef struct _ActiveHardwareMbWidget
{
  GtkWidget parent_instance;

  /** The actual menu button. */
  GtkMenuButton * mbutton;

  //GtkBox *          box;

  /** Image to show next to the label. */
  //GtkImage *        img;

  /** Label to show. */
  //GtkLabel *        label;

  /** The popover. */
  ActiveHardwarePopoverWidget * popover;

  /** True for MIDI, false for audio. */
  bool is_midi;

  /** True for input, false for output. */
  bool input;

  /** The settings to save to. */
  GSettings * settings;

  /** The key in the settings to save to. */
  const char * key;

  /** Popover content holder. */
  GtkBox * content;

  GenericCallback callback;
  void *          object;
} ActiveHardwareMbWidget;

void
active_hardware_mb_widget_setup (
  ActiveHardwareMbWidget * self,
  bool                     is_input,
  bool                     is_midi,
  GSettings *              settings,
  const char *             key);

/**
 * Called from PreferencesWidget to save the
 * settings.
 */
void
active_hardware_mb_widget_save_settings (
  ActiveHardwareMbWidget * self);

void
active_hardware_mb_widget_refresh (
  ActiveHardwareMbWidget * self);

ActiveHardwareMbWidget *
active_hardware_mb_widget_new (void);

/**
 * @}
 */

#endif
