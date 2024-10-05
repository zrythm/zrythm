// SPDX-FileCopyrightText: © 2022 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense

/**
 * @file
 *
 * Velocity settings buttons.
 */

#ifndef __GUI_WIDGETS_VELOCITY_SETTINGS_H__
#define __GUI_WIDGETS_VELOCITY_SETTINGS_H__

#include "gui/cpp/gtk_widgets/gtk_wrapper.h"

/**
 * @addtogroup widgets
 *
 * @{
 */

#define VELOCITY_SETTINGS_WIDGET_TYPE (velocity_settings_widget_get_type ())
G_DECLARE_FINAL_TYPE (
  VelocitySettingsWidget,
  velocity_settings_widget,
  Z,
  VELOCITY_SETTINGS_WIDGET,
  GtkWidget)

/**
 * Velocity settings for toolbars.
 */
typedef struct _VelocitySettingsWidget
{
  GtkWidget parent_instance;

  GtkDropDown * default_velocity_dropdown;
} VelocitySettingsWidget;

/**
 * @}
 */

#endif
