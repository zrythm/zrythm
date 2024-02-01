// clang-format off
// SPDX-FileCopyrightText: © 2019, 2021-2022, 2024 Alexandros Theodotou <alex@zrythm.org>
// SPDX-License-Identifier: LicenseRef-ZrythmLicense
// clang-format on

/**
 * \file
 */

#ifndef __GUI_WIDGETS_TOOLBOX_H__
#define __GUI_WIDGETS_TOOLBOX_H__

#include <gtk/gtk.h>

/**
 * @addtogroup widgets
 *
 * @{
 */

#define TOOLBOX_WIDGET_TYPE (toolbox_widget_get_type ())
G_DECLARE_FINAL_TYPE (ToolboxWidget, toolbox_widget, Z, TOOLBOX_WIDGET, GtkBox)

#define MW_TOOLBOX MAIN_WINDOW->toolbox

typedef struct _ToolboxWidget
{
  GtkBox            parent_instance;
  GtkToggleButton * select_mode;
  GtkToggleButton * stretch_mode;
  GtkToggleButton * edit_mode;
  GtkToggleButton * cut_mode;
  GtkToggleButton * erase_mode;
  GtkToggleButton * ramp_mode;
  GtkToggleButton * audition_mode;
  GtkImage *        select_img;

  gulong select_handler_id;
  gulong stretch_handler_id;
  gulong edit_handler_id;
  gulong cut_handler_id;
  gulong erase_handler_id;
  gulong ramp_handler_id;
  gulong audition_handler_id;
} ToolboxWidget;

/**
 * Sets the toolbox toggled states after deactivating
 * the callbacks.
 */
void
toolbox_widget_refresh (ToolboxWidget * self);

/**
 * @}
 */

#endif
