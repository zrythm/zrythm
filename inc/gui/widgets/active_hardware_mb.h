// SPDX-FileCopyrightText: © 2019-2021 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

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

#define ACTIVE_HARDWARE_MB_WIDGET_TYPE (active_hardware_mb_widget_get_type ())
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

typedef struct _ActiveHardwarePopoverWidget ActiveHardwarePopoverWidget;

/**
 * A menu button that allows selecting active
 * hardware ports.
 */
typedef struct _ActiveHardwareMbWidget
{
  GtkWidget parent_instance;

  /** The actual menu button. */
  GtkMenuButton * mbutton;

  // GtkBox *          box;

  /** Image to show next to the label. */
  // GtkImage *        img;

  /** Label to show. */
  // GtkLabel *        label;

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
active_hardware_mb_widget_save_settings (ActiveHardwareMbWidget * self);

void
active_hardware_mb_widget_refresh (ActiveHardwareMbWidget * self);

ActiveHardwareMbWidget *
active_hardware_mb_widget_new (void);

/**
 * @}
 */

#endif
